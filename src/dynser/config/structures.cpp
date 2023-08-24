#include "structures.h"

void dynser::config::Config::merge(Config&& other) noexcept
{
    version += " + " + other.version;
    tags.merge(std::move(other.tags));
}
