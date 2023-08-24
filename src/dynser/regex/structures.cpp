#include "structures.h"

namespace dynser::regex
{
Group::Group(
    std::unique_ptr<Regex const>&& value,
    Quantifier&& quantifier,
    std::regex&& regex,
    std::size_t number
) noexcept
  : value{ std::move(value) }
  , quantifier{ std::move(quantifier) }
  , regex{ std::move(regex) }
  , number{ number }
{ }

Group::Group(Group const& other) noexcept
  : value{ new Regex{ *other.value } }
  , quantifier{ other.quantifier }
  , regex{ other.regex }
  , number{ other.number }
{ }

NonCapturingGroup::NonCapturingGroup(
    std::unique_ptr<Regex const>&& value,
    Quantifier&& quantifier,
    std::regex&& regex
) noexcept
  : value{ std::move(value) }
  , quantifier{ std::move(quantifier) }
  , regex{ std::move(regex) }
{ }

NonCapturingGroup::NonCapturingGroup(NonCapturingGroup const& other) noexcept
  : value{ new Regex{ *other.value } }
  , quantifier{ other.quantifier }
  , regex{ other.regex }
{ }

Lookup::Lookup(std::unique_ptr<Regex const>&& value, bool is_negative, bool is_forward) noexcept
  : value{ std::move(value) }
  , is_negative{ is_negative }
  , is_forward{ is_forward }
{ }

Lookup::Lookup(Lookup const& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_negative{ other.is_negative }
  , is_forward{ other.is_forward }
{ }

Disjunction::Disjunction(std::unique_ptr<Token const>&& left, std::unique_ptr<Token const>&& right) noexcept
  : left{ std::move(left) }
  , right{ std::move(right) }
{ }

Disjunction::Disjunction(Disjunction const& other) noexcept
  : left{ new Token{ *other.left } }
  , right{ new Token{ *other.right } }
{ }

}    // namespace dynser::regex
