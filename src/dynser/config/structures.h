#pragma once

#include <unordered_map>

#include <optional>
#include <string>
#include <variant>

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
using Regex = std::string;

using DynRegex = std::string;

using GroupValues = std::unordered_map<std::size_t, std::string>;

using DynGroupValues = std::unordered_map<std::size_t, std::string>;

using Script = std::string;

struct Existing
{
    std::string tag;
    std::optional<std::string> prefix;
};

struct Linear
{
    details::yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;
};

struct BranchedMatchSuccessfulness
{
    std::vector<details::yaml::Regex> patterns;
    std::optional<GroupValues> fields;
};

struct BranchedScriptVariable
{
    using Patterns = std::unordered_map<std::string, details::yaml::Regex>;

    std::string variable;
    Script script;
    Patterns patterns;
    std::optional<GroupValues> fields;
};

using Branched = std::variant<BranchedMatchSuccessfulness, BranchedScriptVariable>;

using Continual = std::variant<Existing, Linear, Branched>;

struct Recurrent {

};

using Nested = std::variant<Continual, Recurrent>;

struct Tag
{
    std::string name;
    std::vector<Nested> nested;
    std::optional<Script> serialization_script;
    std::optional<Script> deserialization_script;
};

using Tags = std::unordered_map<std::string, Tag>;

}    // namespace details::yaml

struct Config
{
    std::string version;
    details::yaml::Tags tags;
};

}    // namespace dynser::config
