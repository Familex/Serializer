#pragma once

#include "luwra.hpp"

#include <iostream>
#include <string>

namespace lua_debug
{
int io_loop(luwra::StateWrapper& state) noexcept
{
    std::string request;

    do {
        std::cout << "λ> ";
        std::getline(std::cin, request);

        if (request == "exit()") {
            break;
        }

        const auto result = state.runString(request.c_str());

        if (result != LUA_OK) {
            std::cerr << "An error occured: " << state.read<std::string>(-1) << std::endl;
        }
    } while (request != "exit()");

    return 0;
}

void print_table(const luwra::StateWrapper& state, const char* const key) noexcept
{
    std::map<std::string, luwra::Reference> table = state[key];
    for (auto pair : table) {
        std::cout << pair.first << ": ";
        luwra::LuaType typ = pair.second;
        switch (typ) {
            case luwra::LuaType::Number:
                std::cout << pair.second.read<int>();
                break;

            case luwra::LuaType::String:
                std::cout << pair.second.read<std::string>();
                break;

            case luwra::LuaType::Table:
                std::cout << "table";
                break;

            case luwra::LuaType::Userdata:
                std::cout << "userdata";
                break;

            default:
                std::cout << "Unknown";
                break;
        }
        std::cout << std::endl;
    }
}
}    // namespace lua_debug
