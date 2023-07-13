#pragma once

#include "config/config.h"
#include "config/keywords.h"
#include "luwra.hpp"
#include "structs/context.hpp"
#include "structs/fields.hpp"
#include "util/prefix.hpp"
#include "util/visit.hpp"
#include <unordered_set>

#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

// debug print
#ifdef _DEBUG
#include <format>
#include <iostream>
#endif

namespace dynser
{
/**
 * \brief receives `void(*)(Context&, Properties&&, Target&)`.
 */
template <typename... Fs>
struct PropertyToTargetMapper : Fs...
{
    using Fs::operator()...;
};
template <typename... Fs>
PropertyToTargetMapper(Fs...) -> PropertyToTargetMapper<Fs...>;

/**
 * \brief receives `Properties(*)(Context&, Target&&)`.
 */
template <typename... Fs>
struct TargetToPropertyMapper : Fs...
{
    using Fs::operator()...;
};
template <typename... Fs>
TargetToPropertyMapper(Fs...) -> TargetToPropertyMapper<Fs...>;

namespace serialize_err
{

struct Unknown
{ };

struct ConfigNotLoaded
{ };

struct BranchNotSet
{ };

struct BranchOutOfBounds
{
    std::uint32_t selected_branch;
    std::size_t max_branch;
};

struct ScriptError
{
    std::string message;
};

struct ScriptVariableNotFound
{
    std::string variable_name;
};

struct ResolveRegexError
{
    config::details::regex::ToStringError error;
};

using Error = std::variant<
    Unknown,
    ScriptError,
    ScriptVariableNotFound,
    ResolveRegexError,
    BranchNotSet,
    BranchOutOfBounds,
    ConfigNotLoaded>;

}    // namespace serialize_err

// FIXME rename
/**
 * \brief Error place context on serialize error to handle nested rules.
 */
struct ErrorRef
{
    std::string tag;
    std::size_t rule_ind{};
};

struct SerializeError
{
    serialize_err::Error error;
    std::vector<ErrorRef> ref_seq{};
};

using SerializeResult = std::expected<std::string, SerializeError>;

/**
 * \brief helper.
 */
constexpr SerializeResult make_serialize_err(serialize_err::Error&& err) noexcept
{
    return std::unexpected{ SerializeError{ std::move(err), {} } };
}

namespace details
{

/**
 * \brief Merge <'a, 'b> map and <'b, 'c> map into <'a, 'c> map.
 * Uses ::key_type and ::mapped_type to get keys and values types.
 */
template <
    typename Lhs,
    typename Rhs,
    typename LhsKey = typename Lhs::key_type,
    typename LhsValue = typename Lhs::mapped_type,
    typename RhsKey = typename Rhs::key_type,
    typename RhsValue = typename Rhs::mapped_type,
    typename Result = std::unordered_map<LhsKey, RhsValue>>
    requires std::same_as<LhsValue, RhsKey>
std::expected<Result, LhsValue> merge_maps(const Lhs& lhs, const Rhs& rhs) noexcept
{
    Result result{};
    for (auto& [key, inter] : lhs) {
        if (!rhs.contains(inter)) {
            return std::unexpected{ inter };
        }
        result[key] = rhs.at(inter);
    }
    return result;
}

/**
 * \brief Filters string properties.
 */
Fields props_to_fields(const Properties& props) noexcept
{
    Fields result;
    for (const auto& [key, val] : props) {
        if (val.is_string()) {
            result[key] = val.as_const_string();
        }
    }
    return result;
}

}    // namespace details

/**
 * \brief string <=> target convertion based on Mappers and config file.
 * \tparam PropertyToTargetMapper functor what receives properties struct (and context) and returns target.
 * \tparam TargetToPropertyMapper functor what receives target (and context) and returns properties struct.
 */
template <typename PropertyToTargetMapper, typename TargetToPropertyMapper>
class DynSer
{
    std::optional<config::Config> config_{};

    std::optional<config::Config> from_file(const config::RawContents& wrapper) noexcept
    {
        return config::from_string(wrapper.config);
    }

    std::optional<config::Config> from_file(const config::FileName& wrapper) noexcept
    {
        std::ifstream file{ wrapper.config_file_name };
        std::stringstream buffer;
        buffer << file.rdbuf();
        return from_file(config::RawContents(buffer.str()));
    }

    // share 'existing' serialize between continual and branched
    template <typename Existing>
    auto gen_existing_process_helper(const auto& props, const auto& after_script_fields) noexcept
    {
        return [&](const Existing& nested) noexcept -> dynser::SerializeResult {
            const auto inp = nested.prefix ? util::remove_prefix(props, *nested.prefix) : props;
            const auto serialize_result = this->serialize_props(inp, nested.tag);
            if (!serialize_result &&
                std::holds_alternative<serialize_err::ScriptVariableNotFound>(serialize_result.error().error) &&
                !nested.required)
            {
                return "";    // can be used as recursion exit
            }

            return serialize_result;    // pass through
        };
    }

    // share 'linear' serialize between continual and branched
    template <typename Linear>
    auto gen_linear_process_helper(const auto& props, const auto& after_script_fields) noexcept
    {
        return [&](const Linear& nested) noexcept -> dynser::SerializeResult {
            using config::details::yaml::GroupValues;

            const auto pattern =
                nested.dyn_groups
                    ? config::details::resolve_dyn_regex(
                          nested.pattern,
                          *dynser::details::merge_maps(*nested.dyn_groups, dynser::details::props_to_fields(context))
                      )
                    : nested.pattern;
            const auto regex_fields_sus = nested.fields
                                              ? dynser::details::merge_maps(*nested.fields, after_script_fields)
                                              : std::expected<GroupValues, std::string>{ GroupValues{} };
            if (!regex_fields_sus) {
                // failed to merge script variables (script not set all variables or failed to execute)
                return make_serialize_err(serialize_err::ScriptVariableNotFound{ regex_fields_sus.error() });
            }
            const auto to_string_result = config::details::resolve_regex(pattern, *regex_fields_sus);

            if (!to_string_result) {
                return make_serialize_err(serialize_err::ResolveRegexError{ to_string_result.error() });
            }
            return *to_string_result;
        };
    }

public:
    const PropertyToTargetMapper pttm;
    const TargetToPropertyMapper ttpm;
    Context context;

    DynSer(PropertyToTargetMapper&& pttm, TargetToPropertyMapper&& ttpm) noexcept
      : pttm{ std::move(pttm) }
      , ttpm{ std::move(ttpm) }
    { }

    template <typename ConfigFile>
    bool load_config(ConfigFile&& wrapper) noexcept
    {
        if (const auto config = from_file(std::forward<ConfigFile>(wrapper))) {
            config_ = *config;
            return true;
        }
        return false;
    }

    SerializeResult serialize_props(const Properties& props, const std::string_view tag) noexcept
    {
        if (!config_) {
            return make_serialize_err(serialize_err::ConfigNotLoaded{});
        }

        using namespace config::details::yaml;
        using namespace config;

        // properies to fields
        luwra::StateWrapper state;
        state.loadStandardLibrary();
        register_userdata_property_value(state);
        const auto& tag_config = config_->tags.at(std::string{ tag });

        state[keywords::CONTEXT] = context;
        state[keywords::INPUT_TABLE] = props;
        state[keywords::OUTPUT_TABLE] = Fields{};
        // base script
        if (const auto& script = tag_config.serialization_script) {
            const auto script_run_result = state.runString(script->c_str());
            if (script_run_result != LUA_OK) {
                const auto error = state.read<std::string>(-1);
                return make_serialize_err(serialize_err::ScriptError{ error });
            }
        }
        const auto fields = state[keywords::OUTPUT_TABLE].read<dynser::Fields>();

        // nested
        return util::visit_one(
            tag_config.nested,
            [&](const Continual& continual) -> SerializeResult {
                using namespace config::details;
                std::string result;

                for (std::size_t rule_ind{}; rule_ind < continual.size(); ++rule_ind) {
                    const auto& rule = continual[rule_ind];

                    auto serialized_continual = util::visit_one(
                        rule,
                        gen_existing_process_helper<ConExisting>(props, fields),
                        gen_linear_process_helper<ConLinear>(props, fields)
                    );
                    if (!serialized_continual) {
                        serialized_continual.error().ref_seq.emplace_back(
                            std::string{ tag }, rule_ind
                        );    // add ref to outside rule
                        return serialized_continual;
                    }
                    result += *serialized_continual;
                }

                return result;
            },
            [&](const Branched& branched) -> SerializeResult {
                luwra::StateWrapper branched_script_state;
                branched_script_state.loadStandardLibrary();
                register_userdata_property_value(branched_script_state);

                branched_script_state[keywords::CONTEXT] = context;
                branched_script_state[keywords::INPUT_TABLE] = props;
                using keywords::BRANCHED_RULE_IND_ERRVAL;
                branched_script_state[keywords::BRANCHED_RULE_IND_VARIABLE] = BRANCHED_RULE_IND_ERRVAL;
                const auto branched_script_run_result =
                    branched_script_state.runString(branched.branching_script.c_str());
                if (branched_script_run_result != LUA_OK) {
                    const auto error = branched_script_state.read<std::string>(-1);
                    return make_serialize_err(serialize_err::ScriptError{ error });
                }
                const auto branched_rule_ind = branched_script_state[keywords::BRANCHED_RULE_IND_VARIABLE].read<int>();
                if (branched_rule_ind == BRANCHED_RULE_IND_ERRVAL) {
                    return make_serialize_err(serialize_err::BranchNotSet{});
                }
                if (branched_rule_ind >= branched.rules.size()) {
                    return make_serialize_err(serialize_err::BranchOutOfBounds{
                        .selected_branch = static_cast<std::uint32_t>(branched_rule_ind),
                        .max_branch = branched.rules.size() - 1 });
                }

                auto serialized_branched = util::visit_one(
                    branched.rules[branched_rule_ind],
                    gen_existing_process_helper<BraExisting>(props, fields),
                    gen_linear_process_helper<BraLinear>(props, fields)
                );

                if (!serialized_branched) {
                    serialized_branched.error().ref_seq.emplace_back(
                        std::string{ tag }, branched_rule_ind
                    );    // add ref to outside rule
                }
                return serialized_branched;
            },
            [&](const Recurrent& recurrent) -> SerializeResult {
                std::string result;

                // FIXME

                return result;
            }
        );
    }

    template <typename Target>
    SerializeResult serialize(const Target& target, const std::string_view tag) noexcept
    {
        if (!config_) {
            return make_serialize_err(serialize_err::ConfigNotLoaded{});
        }

        return serialize_props(ttpm(context, target), tag);
    }

    template <typename Target>
    std::optional<Target> deserialize(const std::string_view sv, const std::string_view tag) noexcept
    {
        // string to fields

        // fields to properites

        // properties to target

        return {};
    }
};

}    // namespace dynser