#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "data/bitset.hpp"

/**
 * Regex grammar
 *
 *  alt     := seq ('|' seq)*
 *  seq     := repeat+
 *  repeat  := atom ['*' '+' '?']*
 *  atom    := '(' alt ')' | literal | range
 *  literal := a for a in alphabet
 *  range   := '[' '^'? a+ for a in alphabet ']'
 */

namespace cta
{
    template<class Alphabet = DefaultAlphabet> struct lit;
    template<class Alphabet = DefaultAlphabet> struct rep;
    template<class Alphabet = DefaultAlphabet> struct seq;
    template<class Alphabet = DefaultAlphabet> struct alt;
    template<class Alphabet = DefaultAlphabet, size_t N = Alphabet::Size()> struct range;

    template<class Alphabet = DefaultAlphabet, size_t N = Alphabet::Size()>
    using node = std::variant<
        std::unique_ptr<lit<Alphabet>>,
        std::unique_ptr<rep<Alphabet>>,
        std::unique_ptr<seq<Alphabet>>,
        std::unique_ptr<alt<Alphabet>>,
        std::unique_ptr<range<Alphabet, N>>
    >;

    template<class Alphabet>
    struct lit
    {
        unsigned char data;
    };

    template<class Alphabet>
    struct rep
    {
        enum class type
        {
            zero_or_more,
            one_or_more,
            zero_or_one
        } const kind;
        node<Alphabet> next;
    };

    template<class Alphabet>
    struct seq
    {
        node<Alphabet> left;
        node<Alphabet> right;
    };

    template<class Alphabet>
    struct alt
    {
        node<Alphabet> left;
        node<Alphabet> right;
    };

    template<class Alphabet, size_t N>
    struct range
    {
        Bitset<N> choices;
        bool inverted;
    };

    template<class Alphabet = DefaultAlphabet>
    struct parse_error
    {
        enum class error_kind
        {
            unexpected_end_of_string,
            missing_closing_parenthesis,
            missing_closing_range,
            empty_range,
            character_not_in_alphabet,
            unexpected_character
        };

        std::string_view rest;
        error_kind kind;
    };

    template<class Alphabet = DefaultAlphabet>
    struct parse_ok
    {
        cta::node<Alphabet> node;
        std::string_view rest;
    };

    template<class Alphabet = DefaultAlphabet>
    using parse_result = std::expected<parse_ok<Alphabet>, parse_error<Alphabet>>;

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_alt(std::string_view string);

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_seq(std::string_view string);

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_repeat(std::string_view string);

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_atom(std::string_view string);

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_literal(std::string_view string);

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_range(std::string_view string);

    template<class Alphabet = DefaultAlphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse(std::string_view string)
    {
        return parse_alt<Alphabet>(string);
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_alt(std::string_view string)
    {
        auto left = parse_seq<Alphabet>(string);
        if (!left.has_value()) { return left; }

        auto [ret, rest] = std::move(left.value());
        while (!rest.empty() && rest[0] == '|')
        {
            rest = rest.substr(1);
            auto right = parse_seq<Alphabet>(rest);
            if (!right.has_value()) { return right; }
            ret = std::make_unique<alt<Alphabet>>(alt<Alphabet>{
                .left = std::move(ret),
                .right = std::move(right->node)
            });
            rest = right->rest;
        }

        return parse_ok<Alphabet>{
            .node = std::move(ret),
            .rest = rest
        };
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_seq(std::string_view string)
    {
        auto left = parse_repeat<Alphabet>(string);
        if (!left.has_value()) { return left; }

        auto [ret, rest] = std::move(left.value());
        while (!rest.empty() && rest[0] != '|' && rest[0] != ')')
        {
            auto right = parse_repeat<Alphabet>(rest);
            if (!right.has_value()) { return right; }
            ret = std::make_unique<seq<Alphabet>>(seq<Alphabet>{
                .left = std::move(ret),
                .right = std::move(right->node)
            });
            rest = right->rest;
        }

        return parse_ok<Alphabet>{
            .node = std::move(ret),
            .rest = rest
        };
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_repeat(std::string_view string)
    {
        auto atom = parse_atom<Alphabet>(string);
        if (!atom.has_value()) { return atom; }

        auto [ret, rest] = std::move(atom.value());
        while (!rest.empty())
        {
            auto kind = [&]() -> std::optional<typename rep<Alphabet>::type>
            {
                switch (rest[0])
                {
                    case '*': return rep<Alphabet>::type::zero_or_more;
                    case '+': return rep<Alphabet>::type::one_or_more;
                    case '?': return rep<Alphabet>::type::zero_or_one;
                    default: return std::nullopt;
                }
            }();
            if (!kind.has_value()) { break; }

            ret = std::make_unique<rep<Alphabet>>(rep<Alphabet>{
                .kind = *kind,
                .next = std::move(ret)
            });
            rest = rest.substr(1);
        }

        return parse_ok<Alphabet>{
            .node = std::move(ret),
            .rest = rest
        };
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_atom(std::string_view string)
    {
        if (string.empty())
        {
            return std::unexpected(parse_error<Alphabet>{
                .rest = string,
                .kind = parse_error<Alphabet>::error_kind::unexpected_end_of_string
            });
        }

        if (string[0] == '(')
        {
            auto parsed = parse_alt<Alphabet>(string.substr(1));
            if (!parsed.has_value()) { return parsed; }
            auto [ret, rest] = std::move(parsed.value());
            if (rest.empty() || rest[0] != ')')
            {
                return std::unexpected(parse_error<Alphabet>{
                    .rest = rest,
                    .kind = parse_error<Alphabet>::error_kind::missing_closing_parenthesis
                });
            }
            return parse_ok<Alphabet>{
                .node = std::move(ret),
                .rest = rest.substr(1)
            };
        }

        if (string[0] == '[') { return parse_range<Alphabet>(string); }
        if (string[0] == '|' || string[0] == ')' ||
            string[0] == '*' || string[0] == '+' || string[0] == '?')
        {
            if (!Alphabet::IsMember(static_cast<unsigned char>(string[0])))
            {
                return std::unexpected(parse_error<Alphabet>{
                    .rest = string,
                    .kind = parse_error<Alphabet>::error_kind::unexpected_character
                });
            }
        }
        return parse_literal<Alphabet>(string);
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_literal(std::string_view string)
    {
        if (string.empty())
        {
            return std::unexpected(parse_error<Alphabet>{
                .rest = string,
                .kind = parse_error<Alphabet>::error_kind::unexpected_end_of_string
            });
        }

        const auto character = static_cast<unsigned char>(string[0]);
        if (!Alphabet::IsMember(character))
        {
            return std::unexpected(parse_error<Alphabet>{
                .rest = string,
                .kind = parse_error<Alphabet>::error_kind::character_not_in_alphabet
            });
        }

        return parse_ok<Alphabet>{
            .node = std::make_unique<lit<Alphabet>>(lit<Alphabet>{ .data = character }),
            .rest = string.substr(1)
        };
    }

    template<class Alphabet>
    [[nodiscard]] constexpr parse_result<Alphabet> parse_range(std::string_view string)
    {
        auto rest = string.substr(1);
        bool inverted = false;
        if (!rest.empty() && rest[0] == '^')
        {
            inverted = true;
            rest = rest.substr(1);
        }

        // !! LEFT OFF HERE !! Use Bitset for range representation
        std::vector<unsigned char> choices;
        while (!rest.empty() && rest[0] != ']')
        {
            const auto character = static_cast<unsigned char>(rest[0]);
            if (character == '-')
            {
                const unsigned char start = choices.back();
                rest = rest.substr(1);
                if (rest.empty())
                {
                    return std::unexpected(parse_error<Alphabet>{
                        .rest = rest,
                        .kind = parse_error<Alphabet>::error_kind::missing_closing_range
                    });
                }
                const unsigned character end = static_cast<unsigned char>(rest[0]);
                if (!Alphabet::IsMember(end))
                {
                    return std::unexpected(parse_error<Alphabet>{
                        .rest = rest,
                        .kind = parse_error<Alphabet>::error_kind::character_not_in_alphabet
                    });
                }
                for (unsigned char c = start + 1; c <= end; ++c)
                {
                    choices.push_back(c);
                }
            }
            if (!Alphabet::IsMember(character))
            {
                return std::unexpected(parse_error<Alphabet>{
                    .rest = rest,
                    .kind = parse_error<Alphabet>::error_kind::character_not_in_alphabet
                });
            }
            choices.push_back(character);
            rest = rest.substr(1);
        }

        if (rest.empty())
        {
            return std::unexpected(parse_error<Alphabet>{
                .rest = rest,
                .kind = parse_error<Alphabet>::error_kind::missing_closing_range
            });
        }
        if (choices.empty())
        {
            return std::unexpected(parse_error<Alphabet>{
                .rest = rest,
                .kind = parse_error<Alphabet>::error_kind::empty_range
            });
        }

        return parse_ok<Alphabet>{
            .node = std::make_unique<range<Alphabet>>(range<Alphabet>{
                .choices = { static_cast<size_t>(choices) },
                .inverted = inverted
            }),
            .rest = rest.substr(1)
        };
    }

    template<class... Ts>
    struct overloads : Ts...
    {
        using Ts::operator()...;
    };

    template<class... Ts>
    overloads(Ts...) -> overloads<Ts...>;

    template<class Alphabet>
    constexpr std::size_t tree_size(const node<Alphabet>& n)
    {
        const auto invocation = overloads{
            [](const std::unique_ptr<lit<Alphabet>>&) { return std::size_t{0}; },
            [](const std::unique_ptr<range<Alphabet>>&) { return std::size_t{0}; },
            [&](const std::unique_ptr<rep<Alphabet>>& repetition)
            {
                return tree_size(repetition->next);
            },
            [&](const std::unique_ptr<seq<Alphabet>>& sequence)
            {
                return tree_size(sequence->left) + tree_size(sequence->right);
            },
            [&](const std::unique_ptr<alt<Alphabet>>& alternation)
            {
                return tree_size(alternation->left) + tree_size(alternation->right);
            }
        };
        return 1 + std::visit(invocation, n);
    }
}
