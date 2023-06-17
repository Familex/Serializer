#define LUWRA_REGISTRY_PREFIX "DynSer#"

#include "structs/properties.hpp"
#include "utils/lua_manual_debug.hpp"

int main()
{
    luwra::StateWrapper state;
    state.loadStandardLibrary();
    dyn_ser::register_userdata_property_value(state);

    auto props = dyn_ser::Properties{ { "value", { 123ll } } };
    state["props"] = props;

    lua_debug::io_loop(state);
    // λ> props['value-str'] = tostring(props['value']:as_int())
    // λ> exit()

    std::cout << "lua state props table: \n";
    lua_debug::print_table(state, "props");
    // value: userdata
    // value-str: 123
}
