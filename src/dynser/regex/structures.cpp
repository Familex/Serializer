#include "structures.h"

namespace dynser::regex
{
Group::Group(std::unique_ptr<Regex>&& value, Quantifier&& quantifier, std::regex&& regex, std::size_t number) noexcept
  : value{ std::move(value) }
  , quantifier{ std::move(quantifier) }
  , regex{ std::move(regex) }
  , number{ number }
{ }

Group::Group(const Group& other) noexcept
  : value{ new Regex{ *other.value } }
  , quantifier{ other.quantifier }
  , regex{ other.regex }
  , number{ other.number }
{ }

NonCapturingGroup::NonCapturingGroup(
    std::unique_ptr<Regex>&& value,
    Quantifier&& quantifier,
    std::regex&& regex
) noexcept
  : value{ std::move(value) }
  , quantifier{ std::move(quantifier) }
  , regex{ std::move(regex) }
{ }

NonCapturingGroup::NonCapturingGroup(const NonCapturingGroup& other) noexcept
  : value{ new Regex{ *other.value } }
  , quantifier{ other.quantifier }
  , regex{ other.regex }
{ }

Lookup::Lookup(std::unique_ptr<Regex>&& value, bool is_negative, bool is_forward) noexcept
  : value{ std::move(value) }
  , is_negative{ is_negative }
  , is_forward{ is_forward }
{ }

Lookup::Lookup(const Lookup& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_negative{ other.is_negative }
  , is_forward{ other.is_forward }
{ }

Disjunction::Disjunction(std::unique_ptr<Token>&& left, std::unique_ptr<Token>&& right) noexcept
  : left{ std::move(left) }
  , right{ std::move(right) }
{ }

Disjunction::Disjunction(const Disjunction& other) noexcept
  : left{ new Token{ *other.left } }
  , right{ new Token{ *other.right } }
{ }
}    // namespace dynser::regex
