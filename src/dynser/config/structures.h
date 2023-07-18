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
using PriorityType = std::int32_t;

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
    PriorityType priority;
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
    PriorityType priority;
};

struct RecInfix
{
    details::yaml::Regex pattern;
    std::optional<DynGroupValues> dyn_groups;
    std::optional<GroupValues> fields;

    bool wrap;
    std::optional<std::string> default_value;
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

template <typename Rule>
concept LikeExisting = requires(Rule rule) {
    {
        rule.tag
    } -> std::same_as<std::string&>;
};

template <typename Rule>
concept LikeLinear = requires(Rule rule) {
    {
        rule.pattern
    } -> std::same_as<details::yaml::Regex&>;
    {
        rule.fields
    } -> std::same_as<std::optional<GroupValues>&>;
};

}    // namespace details::yaml

struct Config
{
    std::string version;
    details::yaml::Tags tags;
};

}    // namespace dynser::config
