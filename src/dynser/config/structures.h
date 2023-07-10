#pragma once

#include <unordered_map>

#include <concepts>
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

struct ConExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;
};

struct BraExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;
};

struct RecExisting
{
    std::string tag;
    std::optional<std::string> prefix;
    bool required;

    bool wrap;
    std::optional<std::string> default_value;
};

struct ConLinear
{
    details::yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;
};

struct BraLinear
{
    details::yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;
};

struct RecLinear
{
    details::yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;

    bool wrap;
    std::optional<std::string> default_value;
};

struct RecInfix
{
    details::yaml::Regex pattern;
};

using Continual = std::vector<std::variant<ConExisting, ConLinear>>;
using Recurrent = std::vector<std::variant<RecExisting, RecLinear, RecInfix>>;
struct Branched
{
    Script branching_script;
    Script debranching_script;

    using Rules = std::variant<BraExisting, BraLinear>;
    std::vector<Rules> rules;
};

using Nested = std::variant<Continual, Recurrent, Branched>;
template <typename Rule>
concept NestedConcept = std::same_as<Rule, Continual> || std::same_as<Rule, Recurrent> || std::same_as<Rule, Branched>;

struct Tag
{
    std::string name;
    Nested nested;
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
