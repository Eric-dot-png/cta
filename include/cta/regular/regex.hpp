#pragma once 

#include <string_view>
#include <memory>
#include <variant>
#include <vector>
#include <expected>

/**
 * Regex grammar 
 *
 *  alt     := seq ('|' seq)*
 *  seq     := repeat+
 *  repeat  := atom ['*' '+' '?']?
 *  atom    := ( '(' alt ')' ) | literal | range 
 *  literal := a+ for a in alpabet
 *  range   := '[' '^'? a+ for a in alphabet ']'
 */

namespace cta 
{
    struct lit;
    struct rep; 
    struct seq;
    struct alt;
    struct range;

    using node = std::variant<
        std::unique_ptr<lit>,
        std::unique_ptr<rep>,
        std::unique_ptr<seq>,
        std::unique_ptr<alt>,
        std::unique_ptr<range>
    >;

    struct lit { unsigned char data; };
    struct rep { node next; };
    struct seq { node left, right; };
    struct alt { node left, right; };
    struct range { std::vector<unsigned char> choices; bool inverted; };

    struct parse_error 
    {
        enum class error_kind 
        {
            // error kinds here
        };

        std::string_view rest;
        error_kind kind;
    };
    
    struct parse_ok 
    {
        node node;
        std::string_view rest;
    };


    using parse_result = std::expected<parse_ok, parse_error>;
        
    [[nodiscard]] constexpr parse_result parse_alt(std::string_view string);
    [[nodiscard]] constexpr parse_result parse_seq(std::string_view string);
    [[nodiscard]] constexpr parse_result parse_repeat(std::string_view string);
    [[nodiscard]] constexpr parse_result parse_atom(std::string_view string);
    [[nodiscard]] constexpr parse_result parse_literal(std::string_view string);
    [[nodiscard]] constexpr parse_result parse_range(std::string_view string);


    [[nodiscard]]
    constexpr parse_result parse(std::string_view string)
    {
        return parse_alt(string);
    }
    
    [[nodiscard]]
    constexpr parse_result parse_alt(std::string_view string) 
    {
        auto left = parse_seq(string);
        if (!left.has_value()) { return left; }

        auto [ret, rest] = std::move(left.value());
        while (!rest.empty() && rest[0] == '|') 
        {
            rest = rest.substr(1);
            auto right = parse_seq(rest);
            if (!right.has_value()) { return right; }
            ret = std::make_unique<alt>(std::move(ret), std::move(right->node));
            rest = right->rest;
        }

        return parse_ok { 
            .node = std::move(ret),
            .rest = rest 
        };
    }
    
    template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
    template<class... Ts> overloads(Ts...) -> overloads<Ts...>; // Deducted guide (C++17)

    constexpr std::size_t tree_size(const node& n) {
        constexpr auto invocation = overloads {
            [](const std::unique_ptr<lit>&) { return std::size_t{ 0 }; },
            [](const std::unique_ptr<range>&) { return std::size_t{ 0 }; },
            [&](const std::unique_ptr<rep>& rep) { return tree_size(rep->next); },
            [&](const std::unique_ptr<seq>& seq) { return tree_size(seq->left) + tree_size(seq->right); },
            [&](const std::unique_ptr<alt>& alt) { return tree_size(alt->left) + tree_size(alt->right); },
        };
        return 1 + std::visit(invocation, n);
    }


    /**
     *    template < ct_string UserRegexString >
     *    constexpr auto parse_to_stack() {
     *          static constexpr std::string_view view { UserRegexString };
     *          static constexpr parse_result result = parse(view);
     *          // assume its ok 
     *          static constexpr result_size = tree_size(result->node);
     *          std::array<stack_node, result_size> ret { };
     *          ...
     *          return ret;
     *    }
     *
     *  
     *    template < ct_string Regex > 
     *    constexpr bool matches(std::string_view test) {
     *          static constexpr regex_ast = parse(Regex);
     *    }
     *  
     */
}
