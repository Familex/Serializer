#include "common.hpp"

auto quantifier_to_props(dynser::regex::Quantifier const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Empty const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::WildCard const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Group const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::NonCapturingGroup const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Backreference const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Lookup const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::CharacterClass const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Disjunction const& value) -> dynser::Properties;
auto regex_to_props_helper(dynser::regex::Regex const& value) -> dynser::Properties;

TEST_CASE("Regex")
{
    using namespace dynser_test;
    using namespace dynser::regex;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(),
        dynser::generate_target_to_property_mapper([&](dynser::Context&, Regex const& target) -> dynser::Properties {
            return regex_to_props_helper(target);
        })
    };

    const auto config = R"##(---
version: ''
tags:
  - name: "regex"
    recurrent-dict:
      key: 'value'
      tag: "token"
  - name: "token"
    branched:
      branching-script: |
        local type = inp['type']:as_string()
        if type == "empty" then
           branch = 0
        elseif type == "group" then
           branch = 1
        elseif type == "non-capturing-group" then
           branch = 2
        elseif type == "backreference" then
           branch = 3
        elseif type == "lookup" then
           branch = 4
        elseif type == "character-class" then
           branch = 5
        elseif type == "disjunction" then
           branch = 6
        elseif type == "regex" then
           branch = 7
        end
      debranching-script: ''
      rules:
        - existing: { tag: "empty" }
        - existing: { tag: "group" }
        - existing: { tag: "non-capturing-group" }
        - existing: { tag: "backreference" }
        - existing: { tag: "lookup" }
        - existing: { tag: "character-class" }
        - existing: { tag: "disjunction" }
        - existing: { tag: "regex", prefix: 'value' }
  - name: "empty"
    continual: []
  - name: "group"
    continual:
      - linear: { pattern: '\(' }
      - existing: { tag: "regex", prefix: 'inner' }
      - linear: { pattern: '\)' }
      - existing: { tag: "quantifier" }
  - name: "non-capturing-group"
    continual:
      - linear: { pattern: '\(\?\:' }
      - existing: { tag: "regex", prefix: 'inner' }
      - linear: { pattern: '\)' }
      - existing: { tag: "quantifier" }
  - name: "backreference"
    continual:
      - linear: { pattern: '\\(\d+)', fields: { 1: 'group-number' } }
      - existing: { tag: "quantifier" }
    serialization-script: |
      out['group-number'] = inp['group-number']:as_u64()
  - name: "lookup"
    continual:
      - linear: { pattern: '\(\?(\<?)([=!])', fields: { 1: 'backward-sign', 2: 'negative-sign' } }
      - existing: { tag: "regex", prefix: 'inner' }
      - linear: { pattern: '\)' }
    serialization-script: |
      out['backward-sign'] = inp['is-forward']:as_bool() and '' or '<'
      out['negative-sign'] = inp['is-negative']:as_bool() and '!' or '='
  - name: "character-class"
    continual:
      - linear: { pattern: '\[?', fields: { 0: 'open-bracket-sign' } }
      - linear: { pattern: '\^?', fields: { 0: 'negative-sign' } }
      - linear: { pattern: '.+', fields: { 0: 'characters' } }
      - linear: { pattern: '\]?', fields: { 0: 'close-bracket-sign' } }
      - existing: { tag: "quantifier" }
    serialization-script: |
      out['characters'] = inp['characters']:as_string()
      local content = inp['characters']:as_string()
      local is_content_one_symbol = content:len() == 1 or content:len() == 2 and content:sub(0, 1) == '\\'
      local is_negative = inp['is-negative']:as_bool()
      if is_negative then
        out['open-bracket-sign'] = '['
        out['close-bracket-sign'] = ']'
        out['negative-sign'] = '^'
      elseif not is_content_one_symbol then
        out['open-bracket-sign'] = '['
        out['close-bracket-sign'] = ']'
        out['negative-sign'] = ''
      else
        out['open-bracket-sign'] = ''
        out['close-bracket-sign'] = ''
        out['negative-sign'] = ''
      end
  - name: "disjunction"
    continual:
      - existing: { tag: "token", prefix: 'left' }
      - linear: { pattern: '\|' }
      - existing: { tag: "token", prefix: 'right' }
  - name: "quantifier"
    branched:
      branching-script: |
        local from = inp['from']:as_u64()
        local to = inp['to'] and inp['to']:as_u64() or nil
        local is_lazy = inp['is-lazy']:as_bool()
        if from == 1 and to and to == 1 then
          branch = 0 -- without quantifier
        elseif from == 0 and to == nil then
          if is_lazy then
            branch = 1 -- *?
          else
            branch = 2 -- *
          end
        -- +
        elseif from == 1 and to == nil then
          if is_lazy then
            branch = 3 -- +?
          else
            branch = 4 -- +
          end
        -- {\d+,}
        else
          branch = 5 -- {\d+,\d*}??
        end
      debranching-script: ''
      rules:
        - linear: { pattern: '' }
        - linear: { pattern: '\*\?' }
        - linear: { pattern: '\*' }
        - linear: { pattern: '\+\?' }
        - linear: { pattern: '\+' }
        - existing: { tag: "range-quantifier" }
  - name: "range-quantifier"
    continual:
      - linear: { pattern: '\{(\d+),(\d*)\}(\??)', fields: { 1: 'from', 2: 'to', 3: 'is-lazy' } }
    serialization-script: |
      out['from'] = tostring(inp['from']:as_u64())
      out['to'] = inp['to'] and tostring(inp['to']:as_u64()) or ''
      out['is-lazy'] = inp['is-lazy']:as_bool() and '?' or ''
...)##";

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    char const* const regexes[]{
        R"(|())",
        R"([abc]([^D])(?:u))",
        R"(|||)",
        R"((?=a|b\[)(?<=a\))(?!test)(?<!wha-\!))",
        R"(|[ab]|\d+|[a-zA-Z]|[^a-z]|.|.+)",
        R"((test)\1(\1)\2)",
        R"(a*b+c+?d*?ef{10,}g{10,20}h{10,}?j{10,20}?)",
    };

    for (std::size_t ind{}; auto const regex_str : regexes) {
        DYNAMIC_SECTION("Regex #" << ind << " (" << regex_str << ")")
        {
            auto const regex = from_string(regex_str);
            REQUIRE(regex);
            dynser::Context dummy_context{};
            INFO("Regex props: " << Printer::properties_to_string(ser.ttpm(dummy_context, *regex)));
            DYNSER_TEST_SERIALIZE(*regex, "regex", regex_str);
        }
        ++ind;
    }
}

auto quantifier_to_props(dynser::regex::Quantifier const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    if (value.to) {
        return map_to_props(
            "from",
            static_cast<std::uint64_t>(value.from),
            "to",
            static_cast<std::uint64_t>(*value.to),
            "is-lazy",
            value.is_lazy
        );
    }
    else {
        return map_to_props("from", static_cast<std::uint64_t>(value.from), "is-lazy", value.is_lazy);
    }
}

auto regex_to_props_helper(dynser::regex::Empty const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "empty");
}

auto regex_to_props_helper(dynser::regex::WildCard const& value) -> dynser::Properties
{
    return regex_to_props_helper(dynser::regex::CharacterClass{
        .characters = ".", .is_negative = false, .quantifier = value.quantifier });
}

auto regex_to_props_helper(dynser::regex::Group const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "group") << add_prefix(regex_to_props_helper(*value.value.get()), "inner")
                                         << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::NonCapturingGroup const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "non-capturing-group")
           << add_prefix(regex_to_props_helper(*value.value.get()), "inner") << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Backreference const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "backreference", "group-number", static_cast<std::uint64_t>(value.group_number))
           << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Lookup const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;

    return map_to_props("type", "lookup", "is-negative", value.is_negative, "is-forward", value.is_forward)
           << add_prefix(regex_to_props_helper(*value.value.get()), "inner");
}

auto regex_to_props_helper(dynser::regex::CharacterClass const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;

    return map_to_props("type", "character-class", "characters", value.characters, "is-negative", value.is_negative)
           << quantifier_to_props(value.quantifier);
}

auto regex_to_props_helper(dynser::regex::Disjunction const& value) -> dynser::Properties
{
    using dynser::util::add_prefix;
    using dynser::util::map_to_props;
    using dynser::util::visit_one;

    return map_to_props("type", "disjunction")
           << add_prefix(
                  visit_one(*value.left.get(), [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }), "left"
              )
           << add_prefix(
                  visit_one(*value.right.get(), [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }),
                  "right"
              );
}

auto regex_to_props_helper(dynser::regex::Regex const& value) -> dynser::Properties
{
    using dynser::util::map_to_props;
    using dynser::util::visit_one;

    using List = dynser::PropertyValue::ListType<dynser::PropertyValue>;

    List result;

    for (auto const& tok : value.value) {
        result.emplace_back(visit_one(tok, [](auto&& tok) { return regex_to_props_helper(std::move(tok)); }));
    }

    return map_to_props("value", result);
}

/*
{
    "value":[
        {
            "from":1,
            "inner@value":[
                {
                    "left@characters":"a",
                    "left@from":1,
                    "left@is-lazy":0,
                    "left@is-negative":0,
                    "left@to":1,
                    "left@type":"character-class",
                    "right@characters":"b",
                    "right@from":1,
                    "right@is-lazy":0,
                    "right@is-negative":0,
                    "right@to":1,
                    "right@type":"character-class",
                    "type":"disjunction"
                },
                {
                    "characters":"\\[",
                    "from":1,
                    "is-lazy":0,
                    "is-negative":0,
                    "to":1,
                    "type":"character-class"
                }
            ],
            "is-lazy":0,
            "to":1,
            "type":"non-capturing-group"
        },
        {
            "inner@value":[
                {
                    "characters":"a",
                    "from":1,
                    "is-lazy":0,
                    "is-negative":0,
                    "to":1,
                    "type":"character-class"
                },
                {
                    "characters":"\\)",
                    "from":1,
                    "is-lazy":0,
                    "is-negative":0,
                    "to":1,
                    "type":"character-class"
                }
            ],
            "is-forward":0,
            "is-negative":0,
            "type":"lookup"
        }
    ]
}
*/
