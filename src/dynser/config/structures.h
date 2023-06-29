#pragma once

#include <unordered_map>

#include <string>

namespace dynser::config
{

struct FileName
{
    std::string config_file_name;
};

struct RawContents
{
    std::string config;
};

namespace details::yaml
{
struct Regex
{
    std::string value;
};

struct DynRegex
{
    std::string value;
};

using GroupValues = std::unordered_map<std::size_t, std::string>;

using DynGroupValues = std::unordered_map<std::size_t, std::string>;

}    // namespace details::yaml

}    // namespace dynser::config
