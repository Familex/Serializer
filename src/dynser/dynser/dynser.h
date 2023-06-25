#pragma once

#include "config/config.h"
#include "structs/context.hpp"

#include <fstream>
#include <sstream>
#include <string>

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

/**
 * \brief string <=> target convertion based on Mappers and config file.
 * \tparam PropertyToTargetMapper functor what receives properties struct (and context) and returns target.
 * \tparam TargetToPropertyMapper functor what receives target (and context) and returns properties struct.
 */
template <typename PropertyToTargetMapper, typename TargetToPropertyMapper>
class DynSer
{
    config::Config config_;

public:
    const PropertyToTargetMapper pttm;
    const TargetToPropertyMapper ttpm;
    Context context;

    DynSer(PropertyToTargetMapper&& pttm, TargetToPropertyMapper&& ttpm, config::FileName wrapper) noexcept
      : pttm{ std::move(pttm) }
      , ttpm{ std::move(ttpm) }
    {
        std::ifstream file{ wrapper.config_file_name };
        std::stringstream buffer;
        buffer << file.rdbuf();
        config_ = config::Config{ buffer.str() };
    }

    DynSer(PropertyToTargetMapper&& pttm, TargetToPropertyMapper&& ttpm, config::RawContents wrapper) noexcept
      : pttm{ std::move(pttm) }
      , ttpm{ std::move(ttpm) }
      , config_{ wrapper.config }
    { }

    template <typename Target>
    std::string serialize(const Target& target, const std::string_view tag) noexcept
    {
        std::unreachable();
    }

    template <typename Target>
    Target deserialize(const std::string_view sv, const std::string_view tag) noexcept
    {
        std::unreachable();
    }
};

}    // namespace dynser
