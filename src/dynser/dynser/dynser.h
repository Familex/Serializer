#pragma once

#include "structs/context.hpp"
#include <type_traits>

#include <string>
#include <variant>
#include <vector>

namespace dynser
{

template <typename Target, typename PropertiesToTargetMapper, typename TargetToPropertiesMapper>
    requires requires(
        Context& ctx,
        Target&& target,
        Properties&& props,
        PropertiesToTargetMapper prop_to_targ,
        TargetToPropertiesMapper targ_to_prop
    ) {
        // clang-format off
        { targ_to_prop(ctx, std::move(target)) } -> std::same_as<Properties>;
        { prop_to_targ(ctx, std::move(props)) } -> std::same_as<Target>;
        // clang-format on
    }
struct CDStrategy
{
    using Targ = Target;
    using PTTM = PropertiesToTargetMapper;
    using TTPM = TargetToPropertiesMapper;

    std::string tag;
    PropertiesToTargetMapper prop_to_targ;
    TargetToPropertiesMapper targ_to_prop;
};

template <typename PTTM, typename TTPM>
explicit CDStrategy(std::string, PTTM, TTPM)
    -> CDStrategy<std::invoke_result_t<PTTM, Context&, Properties&&>, PTTM, TTPM>;

template <typename... Strategies>
class DynSer
{
    std::vector<std::variant<Strategies...>> strategies_;
    Context context_{};

public:
    DynSer(Strategies&&... strategies) noexcept
    {
        (
            [&] {
                strategies_.emplace_back(std::move(strategies));
            }(),
            ...
        );
    }
};
}    // namespace dynser
