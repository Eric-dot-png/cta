/**
 * @file regular/alphabet.h
 * @brief File containing alphabet type used in regular automaton 
 */

#pragma once

#include <cstddef>
#include <algorithm>
#include <limits>
#include <array>
#include <span>
#include <utility>
#include <ranges>

namespace cta 
{
    namespace __detail
    {
        /** 
         * @breif Function to determine if a template pack of 
         *        unsigned characters are all unique.
         *
         * @tparam Chars... the characters to check
         * 
         * @returns true if Chars... are unique, false otherwise
         */
        template < unsigned char ... Chars >
        consteval bool AreUnique() 
        {
            if constexpr (sizeof...(Chars) < 1) return true;
            
            std::array<unsigned char, sizeof...(Chars)> AsArray = { Chars... };
            std::sort(std::begin(AsArray), std::end(AsArray));
            auto it = std::unique(std::begin(AsArray), std::end(AsArray));
            // if it == end, then no elements were shifted to the end of the 
            // array by std::unique, so all unique elements.
            return it == std::end(AsArray);
        }

        template < template<unsigned char...> class T, size_t... I>
        consteval auto PopulateWith(std::index_sequence<I...>)
        {
            return T<static_cast<unsigned char>(I)...>{ };
        }
        

    }; // namespace __detail


    /** 
     * @brief Alphabet type for regular languages
     */
    template < unsigned char... Elements >
        requires (sizeof...(Elements) > 0 && __detail::AreUnique<Elements...>())
    struct Alphabet 
    {
        unsigned char Members[sizeof...(Elements)]; ///< Members of the alphabet

        constexpr Alphabet()
            : Members{ Elements... }
        {
            std::ranges::sort(Members);
        }
        
        [[nodiscard]] constexpr size_t Size() const noexcept
        { 
            return sizeof...(Elements); 
        }

    }; // Alphabet
    
    using DefaultAlphabet = decltype(
        __detail::PopulateWith<Alphabet>(
            std::make_index_sequence<
                std::numeric_limits<unsigned char>::max()+1
            >{}
        )
    );


} // namespace cta
