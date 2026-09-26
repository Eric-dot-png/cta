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
#include "../data/bitset.hpp"

namespace cta 
{
    namespace __detail
    {
        /** 
         * @breif Function to determine if a template pack of 
         *        unsigned characters are distinct from eachother.
         *        Used in this file to make sure that the pack argument
         *        in Alphabet contains only unique characters.
         *
         * @tparam Chars the characters to check.
         * 
         * @returns true if Chars are distinct, false otherwise.
         */
        template < unsigned char ... Chars >
        consteval bool AreDistinct() noexcept
        {
            if constexpr (sizeof...(Chars) < 2) return true;
            
            std::array<unsigned char, sizeof...(Chars)> AsArray = { Chars... };
            std::sort(std::begin(AsArray), std::end(AsArray));
            auto it = std::unique(std::begin(AsArray), std::end(AsArray));
            // if it == end, then no elements were shifted to the end of the 
            // array by std::unique, so all unique elements.
            return it == std::end(AsArray);
        }

        /**
         * @brief Function to populate a templated class T with a pack
         *        of unsigned characters casted from an integer sequence.
         *        Used in this file to facilitate the creation of the default
         *        alphabet. (possibly not needed here anymore, but left in for now)
         *
         * @tparam T The class to populate.
         * @tparam I The template parameter pack of idices in index_sequence
         *           paramater.
         *
         * @param index_sequence Unused instance of an index sequence
         *
         * @return Instantiated class of type T packed with the unsinged chars.
         */
        template < template<unsigned char...> class T, size_t... I>
        consteval auto PopulateWith(std::index_sequence<I...>) noexcept
        {
            return T<static_cast<unsigned char>(I)...>{ };
        }
        

    }; // namespace __detail


    /**
     * @brief Alphabet type for regular languages.
     *
     * @tparam Elements Characters to include in the alphabet.
     *                  Constrained by requiring more than 0 characters, and
     *                  also that the characters are distinct (See
     *                  __detail::AreDistinct).
     */
    template < unsigned char... Elems >
        requires (sizeof...(Elems) > 0 && __detail::AreDistinct<Elems...>())
    struct Alphabet
    {
        static constexpr Bitset< std::numeric_limits<unsigned char>::max() + 1 > 
            Members = { (static_cast<size_t>(Elems), ...) }; ///< Members of the alphabet
        
        /**
         * @brief Method to get the size of this alphabet.
         *
         * @return the size of this alphabet.
         */
        [[nodiscard]] static constexpr size_t Size() noexcept
        { 
            return sizeof...(Elems); 
        }

        // No longer needed
        // [[nodiscard]] staticconstexpr std::span<const unsigned char> GetMembers() const noexcept
        // {
        //     return std::span<const unsigned char>(Members, sizeof...(Elems));
        // }

        [[nodiscard]] static constexpr bool IsMember(unsigned char c) noexcept
        {
            return Members.mem(static_cast<size_t>(c));
        }

    }; // Alphabet
 

    using DefaultAlphabet = decltype(__detail::PopulateWith<Alphabet>(std::make_index_sequence<256>{}));


} // namespace cta
