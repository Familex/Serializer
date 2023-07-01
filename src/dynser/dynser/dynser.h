#pragma once

#include "config/config.h"
#include "luwra.hpp"
#include "structs/context.hpp"
#include "structs/fields.hpp"
#include "util/prefix.hpp"
#include "util/visit.hpp"

#include <fstream>
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

namespace details
{

template <
    typename Lhs,
    typename Rhs,
    typename Result = std::unordered_map<typename Lhs::key_type, typename Rhs::mapped_type>>
std::optional<Result> merge_maps(const Lhs& lhs, const Rhs& rhs) noexcept
{
    Result result{};
    for (auto& [key, inter] : lhs) {
        if (!rhs.contains(inter)) {
            return std::nullopt;
        }
        result[key] = rhs.at(inter);
    }
    return result;
}

std::optional<Fields> props_to_fields(const Properties& props) noexcept
{
    Fields result;
    for (const auto& [key, val] : props) {
        result[key] = val.as_const_string();
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

    using SerializeResult = std::optional<std::string>;

    // FIXME proper error handling
    SerializeResult serialize_props(const Properties& props, const std::string_view tag) noexcept
    {
        // properies to fields
        luwra::StateWrapper state;
        state.loadStandardLibrary();
        register_userdata_property_value(state);
        const auto& tag_config = config_->tags.at(std::string{ tag });

        state["inp"] = props;
        state["out"] = Fields{};
        // base script
        if (const auto& script = tag_config.serialization_script) {
            const auto script_run_result = state.runString(script->c_str());
            if (script_run_result != LUA_OK) {
                const auto error = state.read<std::string>(-1);
                return std::nullopt;
            }
        }
        const auto fields = state["out"].read<dynser::Fields>();

        // nested
        std::string result;
        for (const auto& nested_v : tag_config.nested) {
            using namespace config::details::yaml;

            const auto serialized_nested = util::visit_one(
                nested_v,
                [&](const Existing& nested) -> SerializeResult {
                    const auto inp = util::remove_prefix(props, nested.prefix ? *nested.prefix : "");
                    const auto serialize_result = serialize_props(inp, nested.tag);
                    if (!serialize_result) {
                        return std::nullopt;
                    }
                    return *serialize_result;
                },
                [&](const Linear& nested) -> SerializeResult {
                    const auto to_string_result = config::details::resolve_regex(
                        nested.dyn_groups
                            ? config::details::resolve_dyn_regex(
                                  nested.pattern,
                                  *details::merge_maps(*nested.dyn_groups, *details::props_to_fields(context))
                              )
                            : nested.pattern,
                        (nested.fields ? static_cast<GroupValues>(*details::merge_maps(*nested.fields, fields))
                                       : GroupValues{})
                    );
                    if (!to_string_result) {
                        return std::nullopt;
                    }
                    return *to_string_result;
                },
                [&](const Branched& nested) -> SerializeResult {
                    return {};
                },
                [&](const Recurrent& nested) -> SerializeResult {
                    return {};
                }
            );
            if (!serialized_nested) {
                return std::nullopt;
            }
            result += *serialized_nested;
        }

        return result;
    }

    template <typename Target>
    std::optional<std::string> serialize(const Target& target, const std::string_view tag) noexcept
    {
        if (!config_) {
            return std::nullopt;
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

/* trash bin (remove before push)
// WRONG (must be recursive)
if (std::holds_alternative<Existing>(existing_sus)) {
    const auto& existing = std::get<Existing>(existing_sus);
    const auto& existing_tag_script = config_->tags.at(existing.tag).serialization_script;
    luwra::StateWrapper state_inner;
    state_inner.loadStandardLibrary();
    register_userdata_property_value(state_inner);

    // nullopt prefix handle
    const auto inp = util::remove_prefix(props, *existing.prefix);
    const auto serialize_result = serialize_props(inp, existing.tag);
    state_inner["inp"] = inp;
    state_inner["out"] = Fields{};

    const auto script_run_result = state_inner.runString(existing_tag_script->c_str());
    if (script_run_result != LUA_OK) {
        const auto error = state_inner.read<std::string>(-1);
        return std::nullopt;
    }
    // FIXME some merge method?
    const auto add = util::add_prefix(state_inner["out"].read<Fields>(), *existing.prefix);
    for (const auto& [key, val] : add) {
        state["out"][key] = val;
    }
}
*/
