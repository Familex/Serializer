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
    regex::ToStringError error;
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

using PrioritizedListLen = std::pair<config::yaml::PriorityType, std::size_t>;

// forward declaration
std::optional<PrioritizedListLen>
calc_max_property_lists_len(config::Config const&, Properties const&, config::yaml::Nested const&) noexcept;

// for existing and linear rules
std::optional<PrioritizedListLen> calc_max_property_lists_len_helper(
    config::Config const& config,
    Properties const& props,
    config::yaml::LikeExisting auto const& rule
) noexcept
{
    using namespace config::yaml;

    auto result = calc_max_property_lists_len(config, props, config.tags.at(rule.tag).nested);

    if constexpr (HasPriority<decltype(rule)>) {
        if (result) {
            result->first = rule.priority;
        }
    }
    return result;
}

// for existing and linear rules (
std::optional<PrioritizedListLen> calc_max_property_lists_len_helper(
    config::Config const& config,
    Properties const& props,
    config::yaml::LikeLinear auto const& rule
) noexcept
{
    using namespace config::yaml;

    std::optional<std::size_t> result{ std::nullopt };

    if (!rule.fields) {
        return result ? std::optional<PrioritizedListLen>{ { 0, *result } } : std::optional<PrioritizedListLen>{};
    }

    for (auto const& [group_num, field_name] : *rule.fields) {
        if (!props.contains(field_name)) {
            continue;
        }

        const auto& list_prop_sus = props.at(field_name);

        if (!list_prop_sus.is_list()) {
            continue;
        }

        const auto list_len = list_prop_sus.as_const_list().size();

        if (!result || *result < list_len) {
            result = list_len;
        }
    }

    std::optional<PrioritizedListLen> result_prioritized                  //
        { result ? std::optional<PrioritizedListLen>{ { 0, *result } }    //
                 : std::optional<PrioritizedListLen>{} };

    if constexpr (HasPriority<decltype(rule)>) {
        if (result_prioritized) {
            result_prioritized->first = rule.priority;
        }
    }

    return result_prioritized;
}

std::optional<PrioritizedListLen> calc_max_property_lists_len(
    config::Config const& config,
    Properties const& props,
    config::yaml::Nested const& rules
) noexcept
{
    using namespace config::yaml;

    std::optional<PrioritizedListLen> result{ std::nullopt };

    const auto visit_vector_of_existing_or_linear_rules =    //
        [&config, &props, &result](auto const& vector_of_existing_or_linear_rules) {
            for (const auto& rule_v : vector_of_existing_or_linear_rules) {
                const auto rule_result = std::visit(
                    [&config, &props](auto const& rule) {    //
                        return calc_max_property_lists_len_helper(config, props, rule);
                    },
                    rule_v
                );

                if (rule_result) {
                    if (!result ||                               //
                        rule_result->first > result->first ||    //
                        rule_result->first == result->first && rule_result->second > result->second)
                    {
                        result = *rule_result;
                    }
                }
            }
        };

    util::visit_one(
        rules,
        [&](Branched const& branched) {    //
            visit_vector_of_existing_or_linear_rules(branched.rules);
        },
        [&](auto const& vector_of_existing_or_linear_rules) {
            visit_vector_of_existing_or_linear_rules(vector_of_existing_or_linear_rules);
        }
    );

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

    // share 'existing' serialize between continual, branched and recurrent
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

    // share 'linear' serialize between continual, branched and recurrent
    template <typename Linear>
    auto gen_linear_process_helper(const auto& props, const auto& after_script_fields) noexcept
    {
        return [&](const Linear& nested) noexcept -> dynser::SerializeResult {
            using config::yaml::GroupValues;

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

        using namespace config::yaml;
        using namespace config;

        // input: { 'a': 0, 'b': [ 1, 2, 3 ] }
        // output: ( { 'a': 0 }, [ { 'b': 1 }, { 'b': 2 }, { 'b': 3 } ] )
        const auto [non_list_props, unflattened_props_vector] =
            [](Properties const& props) -> std::pair<Properties, std::vector<Properties>> {    // iife
            std::vector<Properties> unflattened_props;
            Properties non_list_props;

            for (auto const& [key, prop_val] : props) {
                if (prop_val.is_list()) {
                    const auto size = prop_val.as_const_list().size();

                    if (unflattened_props.size() < size) {
                        unflattened_props.resize(size);
                    }
                    for (std::size_t ind{}; auto const& el : prop_val.as_const_list()) {
                        unflattened_props[ind][key] = el;
                        ++ind;
                    }
                }
                else {
                    non_list_props[key] = prop_val;
                }
            }

            return { non_list_props, unflattened_props };
        }(props);

        luwra::StateWrapper state;
        state.loadStandardLibrary();
        register_userdata_property_value(state);
        state[keywords::CONTEXT] = context;
        const auto& tag_config = config_->tags.at(std::string{ tag });

        const auto props_to_fields =    //
            [](luwra::StateWrapper& state, Properties const& props, Tag const& tag_config
            ) -> std::expected<dynser::Fields, dynser::SerializeError> {
            state[keywords::INPUT_TABLE] = props;
            state[keywords::OUTPUT_TABLE] = Fields{};
            // run script
            if (const auto& script = tag_config.serialization_script) {
                const auto script_run_result = state.runString(script->c_str());
                if (script_run_result != LUA_OK) {
                    const auto error = state.read<std::string>(-1);
                    return std::unexpected{ dynser::SerializeError{ serialize_err::ScriptError{ error }, {} } };
                }
            }
            return state[keywords::OUTPUT_TABLE].read<dynser::Fields>();
        };

        Fields fields;
        if (!non_list_props.empty()) {
            auto non_list_fields_sus = props_to_fields(state, non_list_props, tag_config);
            if (!non_list_fields_sus) {
                return make_serialize_err(std::move(non_list_fields_sus.error().error));
            }
            fields = *non_list_fields_sus;
        }    // else only list-fields

        // input (unflattened_props_vector): [ { 'b': 1 }, { 'b': 2 }, { 'b': 3 } ]
        // output (unflattened_fields): [ { 'b': '1' }, { 'b': '2' }, { 'b': '3' } ]
        std::vector<Fields> unflattened_fields;    // for recurrent
        if (!unflattened_props_vector.empty()) {
            for (auto const& unflattened_props : unflattened_props_vector) {
                auto unflattened_fields_sus = props_to_fields(state, unflattened_props, tag_config);
                if (!unflattened_fields_sus) {
                    return make_serialize_err(std::move(unflattened_fields_sus.error().error));
                }
                unflattened_fields.push_back(*unflattened_fields_sus);
            }
        }

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

                // max length of property lists
                // FIXME remove unwrap
                // FIXME is priority unused?
                const auto max_len = dynser::details::calc_max_property_lists_len(*config_, props, recurrent)->second;

                for (std::size_t ind{}; ind < max_len; ++ind) {
                    for (const auto& recurrent_rule : recurrent) {
                        // split lists in props and fields into current element
                        auto curr_fields = fields;
                        if (unflattened_fields.size() > ind) {
                            curr_fields.merge(unflattened_fields[ind]);
                        }
                        auto curr_props = props;
                        if (unflattened_props_vector.size() > ind) {
                            for (auto const& [key, val] : unflattened_props_vector[ind]) {
                                curr_props[key] = val;
                            }
                        }
                        const auto serialized_recurrent = util::visit_one(
                            recurrent_rule,
                            gen_existing_process_helper<RecExisting>(curr_props, curr_fields),
                            gen_linear_process_helper<RecLinear>(curr_props, curr_fields),
                            [&](const RecInfix& rule) -> SerializeResult {
                                if (ind == max_len - 1) {
                                    return "";    // infix rule -> return empty string on last element
                                }

                                return gen_linear_process_helper<RecInfix>(curr_props, curr_fields)(rule);
                            }
                        );
                        if (!serialized_recurrent) {
                            return serialized_recurrent;
                        }
                        result += *serialized_recurrent;
                    }
                }

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
