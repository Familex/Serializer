#define LUWRA_REGISTRY_PREFIX "DynSer#"

#include "dynser.h"

struct Foo
{
    int quux;
};

struct Bar
{
    Foo foo;
    int baz;
};

int main()
{
    // clang-format off
    dynser::DynSer ser{
        dynser::CDStrategy{
            "foo",
            [](dynser::Context&, dynser::Properties&& props) {
                return Foo{ std::any_cast<int>(props["foo-quux"].data) };
            },
            [](dynser::Context&, Foo&& target) {
                return dynser::Properties{
                    { "foo-quux-field", { target.quux } }
                };
            }
        },
        dynser::CDStrategy{
            "bar",
            [](dynser::Context&, dynser::Properties&& props) {
                return Bar
                {
                    Foo{ std::any_cast<int>(props["bar-foo-quux"].data) },
                    std::any_cast<int>(props["bar-baz"])
                };
            },
            [](dynser::Context&, Bar&& target) {
                return dynser::Properties{
                    { "bar-foo-field", { target.foo.quux } },
                    { "bar-baz-field", { target.baz } }
                };
            }
        }
    };
    // clang-format on
}
