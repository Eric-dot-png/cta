/** 
 * @brief file containing bitset implementation that is NTTP friendly,
 *        as c++ std::bitset is not 
 * @
 */

#pragma once 

#include <array>
#include <cstddef>
#include <bit>
#include <stdexcept>
#include <ranges>
#include <utility>

namespace cta 
{
    template < size_t N >
    struct bitset 
    {
        /// @brief Constant for how large words are (in bits)
        static constexpr size_t WORD_SIZE = sizeof(size_t) * 8U;


        /// @brief Constant for how many words needed in the bitset
        static constexpr size_t NUM_WORDS = (N + WORD_SIZE - 1) / WORD_SIZE;


        std::array<size_t, NUM_WORDS> words; ///< Words that make up bits


        constexpr bitset() noexcept
            : words { 0 }
        { }


        template < size_t... Values >
            requires ((Values < N) && ...)
        explicit constexpr bitset(std::index_sequence<Values...>) noexcept 
            : bitset()
        {
            (set(Values), ...);
        }


        constexpr bitset(const bitset&) = default;


        [[nodiscard]] constexpr bitset& operator=(const bitset&) = default;


        [[nodiscard]] constexpr bool operator==(const bitset&) const = default;


        constexpr void set(size_t val) const
        {
            if (val >= N) 
            { 
                throw std::out_of_range("bitset::set out of range"); 
            }
            
            size_t word_idx = val / WORD_SIZE;
            size_t offset   = val % WORD_SIZE;
            words[word_idx] |= (size_t{1} << offset);
        }


        [[nodiscard]] constexpr bool mem(size_t test) const noexcept
        {
            if (test > N) {  return false; }

            size_t word_idx = test / WORD_SIZE;
            size_t offset   = test % WORD_SIZE;
            return words[word_idx] >> offset & 1;
        }


        [[nodiscard]] bitset& operator|=(const bitset& other) noexcept 
        {
            namespace stdv = std::ranges::views;
            for (auto [idx, word] : stdv::enumerate(other.words))
            {
                words[idx] |= word;
            }
            return *this;
        }


        [[nodiscard]] bitset& operator&=(const bitset& other) noexcept 
        {
            namespace stdv = std::ranges::views;
            for (auto [idx, word] : stdv::enumerate(other.words))
            {
                words[idx] &= word;
            }
            return *this;
        }


        [[nodiscard]] 
            constexpr bitset operator|(const bitset& other) const noexcept 
        {
            return bitset(*this) |= other;
        }


        [[nodiscard]] 
            constexpr bitset operator&(const bitset& other) const noexcept 
        {
            return bitset(*this) &= other;
        }

        /// @brief iterator type
        class iterator {
            friend class bitset<N>;
        private:
            const bitset& parent;   //< reference to the parent bitset object
            size_t bit_idx;         //< this iterator's bit index
            
            /**
             * @brief Static utility method to find the next set bit
             *
             * @param parent parent bitset to populate iterator with
             * @param bit_idx the bit index to start from. *Checks this index*.
             *
             * @returns iterator instance containing the bit_idx of the 
             *          next set bit, or the end iterator.
             */
            [[nodiscard]] static constexpr iterator find_next(
                    const bitset& parent, size_t bit_idx) noexcept
            {
                if (bit_idx >= N) { return {parent, N}; }
                
                size_t word_idx = bit_idx / WORD_SIZE;
                size_t word = parent.words[word_idx] >> (bit_idx % WORD_SIZE);
                while ( word == 0 )
                {
                    ++word_idx;
                    if (word_idx >= NUM_WORDS) 
                    {
                        return {parent, N};
                    }
                    word = parent.words[word_idx];
                }
                bit_idx = word_idx * WORD_SIZE + std::countr_zero(word);
                return {parent, bit_idx};
           }

        public:

            /** 
             * @brief dereference operator.
             *
             * @return bit index of this iterator.
             */
            [[nodiscard]] constexpr size_t operator*() const noexcept
            { 
                return bit_idx; 
            }


            /**
             * @brief pre-increment operator.
             */
            [[nodiscard]] constexpr iterator& operator++() noexcept
            {
                ++bit_idx;
                return (*this = iterator::find_next(parent, bit_idx));
            }
        

            /** 
             * @brief not equal operator.
             */
            [[nodiscard]] constexpr bool operator!=(const iterator& other) 
                const noexcept
            {
                return other.bit_idx != bit_idx;
            }

        }; // struct bitset::iterator
        

        /** 
         * @brief method to return the beginning iterator of this set.
         *        required by compiler for range-based for loops and STL
         *        compatibility.
         */ 
        [[nodiscard]] constexpr iterator begin() const noexcept
        { 
            return iterator::find_next(*this, 0); 
        }
        

        /**
         * @brief method to retunr ending iterator of this set.
         *        required by compiler for range-based for loops and STL
         *        compatibility.
         */      
        [[nodiscard]] constexpr iterator end() const noexcept
        { 
            return { *this, N }; 
        }
    

    }; // struct bitset

}; //namespace cta
