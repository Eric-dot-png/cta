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
    struct Bitset 
    {
        /// @brief Constant for how large words are (in bits)
        static constexpr size_t WORD_SIZE = sizeof(size_t) * 8U;


        /// @brief Constant for how many words needed in the bitset
        static constexpr size_t NUM_WORDS = (N + WORD_SIZE - 1) / WORD_SIZE;


        std::array<size_t, NUM_WORDS> words; ///< Words that make up bits


        constexpr Bitset() noexcept
            : words { 0 }
        { }


        template < size_t... Values >
            requires ((Values < N) && ...)
        explicit constexpr Bitset(std::index_sequence<Values...>) noexcept 
            : Bitset()
        {
            (set(Values), ...);
        }


        constexpr Bitset(const Bitset&) = default;


        [[nodiscard]] constexpr Bitset& operator=(const Bitset&) = default;


        [[nodiscard]] constexpr bool operator==(const Bitset&) const = default;

        
        /** 
         * @brief Sets the specified bit. 
         * @param val bit index to set. 
         *
         * @throws std::out_of_range if @p val is greater than or equal to N. 
         */
        constexpr void set(size_t val)
        {
            if (val >= N) 
            { 
                throw std::out_of_range("bitset::set out of range"); 
            }
            
            size_t word_idx = val / WORD_SIZE;
            size_t offset   = val % WORD_SIZE;
            words[word_idx] |= (size_t{1} << offset);
        }


        /** 
         * @brief Tests whether a bit is set. 
         *
         * @param test bit index to test.
         *
         * @return true if the specified bit is set, otherwise false. 
         *
         * @note An index outside the bitset's range returns false. 
         */
        [[nodiscard]] constexpr bool mem(size_t test) const noexcept
        {
            if (test >= N) {  return false; }

            size_t word_idx = test / WORD_SIZE;
            size_t offset   = test % WORD_SIZE;
            return words[word_idx] >> offset & 1;
        }

        /**
         * @brief Performs a set union operation with another bitset. 
         *
         * Each word in @p other is ORed with the corresponding word in 
         * this bitset. 
         *
         * @param other bitset to OR with this bitset. 
         *
         * @return reference to this bitset after the operation. 
         */
        [[nodiscard]] constexpr Bitset& operator|=(const Bitset& other) noexcept 
        {
            namespace stdv = std::ranges::views;
            for (auto [idx, word] : stdv::enumerate(other.words))
            {
                words[idx] |= word;
            }
            return *this;
        }

        /**
         * @brief Performs a bitwise AND operation with another bitset. 
         *
         * Each word in @p other is ANDed with the corresponding word in 
         * this bitset. 
         *
         * @param other bitset to AND with this bitset.
         *
         * @return reference to this bitset after the operation. 
         */
        [[nodiscard]] constexpr Bitset& operator&=(const Bitset& other) noexcept 
        {
            namespace stdv = std::ranges::views;
            for (auto [idx, word] : stdv::enumerate(other.words))
            {
                words[idx] &= word;
            }
            return *this;
        }

        
        /**
         * @brief Performs a bitwise OR operation between two bitsets. 
         *
         * @param other bitset to OR with this bitset. 
         *
         * @return a new bitset containing the result of the OR operation. 
         */
        [[nodiscard]] 
            constexpr Bitset operator|(const Bitset& other) const noexcept 
        {
            return Bitset(*this) |= other;
        }

        /** 
         * @brief Performs a bitwise AND operation between two bitsets. 
         *
         * @param other bitset to AND with this bitset. 
         *
         * @return a new bitset containing the result of the AND operation. 
         */
        [[nodiscard]] 
            constexpr Bitset operator&(const Bitset& other) const noexcept 
        {
            return Bitset(*this) &= other;
        }

        /// @brief Iterator type over the set bits in the bitset
        class iterator {
            friend class Bitset<N>;
        private:
            const Bitset& parent_;   //< reference to the parent bitset object
            size_t bitIdx_;          //< this iterator's bit index
            

            /** @brief constructor
             *
             *  @param parent parent bitset to iterate on 
             *  @param bitIdx bit index of this iterator 
             */
            constexpr iterator(const Bitset& parent, size_t bitIdx)
                : parent_(parent), bitIdx_(bitIdx)
            { }


            /**
             * @brief Static utility method to find the next set bit
             *
             * @param parent parent bitset to populate iterator with
             * @param bit_idx the bit index to start from. *Checks this index*.
             *
             * @returns iterator instance containing the bit_idx of the 
             *          next set bit, or the end iterator.
             */
            [[nodiscard]] static constexpr iterator findNext(
                    const Bitset& parent, size_t bit_idx) noexcept
            {
                if (bit_idx >= N) { return {parent, N}; }
                
                size_t word_idx = bit_idx / WORD_SIZE;
                size_t offset = bit_idx % WORD_SIZE;
                size_t word = parent.words[word_idx] >> offset;
                while ( word == 0 )
                {
                    ++word_idx;
                    if (word_idx >= NUM_WORDS) 
                    {
                        return {parent, N};
                    }
                    word = parent.words[word_idx];
                    offset = 0;
                }
                bit_idx = 
                    word_idx * WORD_SIZE + std::countr_zero(word) + offset;
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
                return bitIdx_; 
            }


            /**
             * @brief pre-increment operator.
             */
            constexpr iterator& operator++() noexcept
            {
                bitIdx_ = iterator::findNext(parent_, ++bitIdx_).bitIdx_;
                return *this;
            }
        

            /** 
             * @brief not equal operator.
             */
            [[nodiscard]] constexpr bool operator!=(const iterator& other) 
                const noexcept
            {
                return other.bitIdx_ != bitIdx_;
            }

        }; // struct bitset::iterator
        

        /** 
         * @brief method to return the beginning iterator of this set.
         *        required by compiler for range-based for loops and STL
         *        compatibility.
         */ 
        [[nodiscard]] constexpr iterator begin() const noexcept
        { 
            return iterator::findNext(*this, 0); 
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
