#pragma once

#include "structs/context.hpp"

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
public:
    const PropertyToTargetMapper pttm;
    const TargetToPropertyMapper ttpm;
    Context context;

    DynSer(PropertyToTargetMapper&& pttm, TargetToPropertyMapper&& ttpm)
      : pttm{ std::move(pttm) }
      , ttpm{ std::move(ttpm) }
    { }
};

}    // namespace dynser
