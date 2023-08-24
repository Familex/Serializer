#include "properties.h"

dynser::Properties dynser::operator<<(dynser::Properties&& lhs, dynser::Properties&& rhs) noexcept
{
    lhs.merge(rhs);
    return lhs;
}

dynser::Properties dynser::operator<<(dynser::Properties&& lhs, dynser::Properties const& rhs) noexcept
{
    lhs.merge(dynser::Properties{ rhs });
    return lhs;
}

void dynser::register_userdata_property_value(luwra::StateWrapper& state) noexcept
{
    state.registerUserType<dynser::PropertyValue>(
        // members
        {
            // as
            LUWRA_MEMBER(dynser::PropertyValue, as_i32),
            LUWRA_MEMBER(dynser::PropertyValue, as_i64),
            LUWRA_MEMBER(dynser::PropertyValue, as_u32),
            LUWRA_MEMBER(dynser::PropertyValue, as_u64),
            LUWRA_MEMBER(dynser::PropertyValue, as_float),
            LUWRA_MEMBER(dynser::PropertyValue, as_string),
            LUWRA_MEMBER(dynser::PropertyValue, as_bool),
            LUWRA_MEMBER(dynser::PropertyValue, as_char),
            LUWRA_MEMBER(dynser::PropertyValue, as_list),
            LUWRA_MEMBER(dynser::PropertyValue, as_map),
            // is
            LUWRA_MEMBER(dynser::PropertyValue, is_i32),
            LUWRA_MEMBER(dynser::PropertyValue, is_i64),
            LUWRA_MEMBER(dynser::PropertyValue, is_u32),
            LUWRA_MEMBER(dynser::PropertyValue, is_u64),
            LUWRA_MEMBER(dynser::PropertyValue, is_float),
            LUWRA_MEMBER(dynser::PropertyValue, is_string),
            LUWRA_MEMBER(dynser::PropertyValue, is_bool),
            LUWRA_MEMBER(dynser::PropertyValue, is_char),
            LUWRA_MEMBER(dynser::PropertyValue, is_list),
            LUWRA_MEMBER(dynser::PropertyValue, is_map),
        },
        // meta-members
        {}
    );
}
