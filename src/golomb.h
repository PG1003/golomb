// MIT License
//
// Copyright (c) 2022 PG1003
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <concepts>
#include <algorithm>
#include <limits>
#include <iterator>
#include <bit>
#include <type_traits>


namespace pg::golomb
{

namespace detail
{

template< typename SignedT >
requires std::signed_integral< SignedT >
auto to_unsigned( SignedT s )
{
    using UnsignedT = typename std::make_unsigned< SignedT >::type;

    if( s < 0 )
    {
        return static_cast< UnsignedT >( ( ~s << 1 ) + 1 );
    }

    return static_cast< UnsignedT >( s << 1 );
}

template< typename UnsignedT >
requires std::unsigned_integral< UnsignedT >
auto to_signed( UnsignedT u )
{
    using SignedT = typename std::make_signed< UnsignedT >::type;

    if( u & 0x01u )
    {
        return static_cast< SignedT >( ~( u >> 1 ) );
    }

    return static_cast< SignedT >( u >> 1 );
}

template< typename IteratorT >
requires std::signed_integral< typename std::iterator_traits< IteratorT >::value_type >
struct integer_input_adapter
{
    using iterator_category = std::input_iterator_tag;
    using value_type        = typename std::make_unsigned< typename std::iterator_traits< IteratorT >::value_type >::type;
    using difference_type   = ptrdiff_t;
    using pointer           = value_type *;
    using const_pointer     = const value_type *;
    using reference         = value_type &;
    using const_reference   = const value_type &;

    IteratorT it;

    integer_input_adapter( const IteratorT & it ) noexcept
        : it( it )
    {}

    integer_input_adapter & operator =( value_type u )
    {
        *it = to_signed( u );
        return *this;
    }

    operator value_type() const
    {
        return to_unsigned( *it );
    }

    [[nodiscard]] const integer_input_adapter & operator *() const noexcept { return *this; }
    [[nodiscard]] const integer_input_adapter * operator ->() const noexcept { return this; }

                  integer_input_adapter & operator ++() { ++it; return *this; }
                  integer_input_adapter   operator ++( int ) { auto self = *this; it++; return self; }
    [[nodiscard]] bool                    operator ==( const integer_input_adapter &other ) const { return it == other.it; }
    [[nodiscard]] bool                    operator !=( const integer_input_adapter &other ) const { return it != other.it; }
};

template< typename IteratorT >
integer_input_adapter( const IteratorT& ) noexcept
-> integer_input_adapter< IteratorT >;


template< typename IteratorT >
struct integer_output_adapter
{
    using iterator_category = std::output_iterator_tag;
    using value_type        = void;
    using difference_type   = ptrdiff_t;
    using pointer           = void;
    using reference         = void;

    IteratorT it;

    integer_output_adapter( const IteratorT & it ) noexcept
        : it( it )
    {}

    template< typename UnsignedT >
    requires std::unsigned_integral< UnsignedT >
    integer_output_adapter & operator =( UnsignedT u )
    {
        *it = to_signed( u );
        return *this;
    }

    [[nodiscard]] integer_output_adapter & operator *() noexcept { return *this; }
    [[nodiscard]] integer_output_adapter * operator ->() noexcept { return this; }
                  integer_output_adapter & operator ++() { ++it; return *this; }
                  integer_output_adapter   operator ++( int ) { auto self = *this; it++; return self; }
    [[nodiscard]] bool                     operator ==( const integer_output_adapter &other ) const { return it == other.it; }
    [[nodiscard]] bool                     operator !=( const integer_output_adapter &other ) const { return it != other.it; }
};

template< typename IteratorT >
integer_output_adapter( const IteratorT& ) noexcept
-> integer_output_adapter< IteratorT >;

}

template< typename OutputDataT = uint8_t,
          typename InputIt,
          typename OutputIt >
requires std::unsigned_integral< typename std::iterator_traits< InputIt >::value_type > &&
         std::unsigned_integral< OutputDataT >
constexpr auto encode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    using InputValueT = typename std::iterator_traits< InputIt >::value_type;

    constexpr auto input_digits  = std::numeric_limits< InputValueT >::digits;
    constexpr auto input_mask    = std::numeric_limits< InputValueT >::max();
    constexpr auto output_digits = std::numeric_limits< OutputDataT >::digits;

    const auto overflow_threshold = std::numeric_limits< InputValueT >::max() - ( static_cast< InputValueT >( 1 ) << k );
    const auto base               = 1u << k;
    const auto max_zeros          = input_digits - k;
    const auto zeros_threshold    = 1u + k;

    OutputDataT encoded       = 0u;
    auto        encoded_count = 0;

    while( input != last )
    {
        const InputValueT x         = *input++;
        const auto        overflow  = x > overflow_threshold;
        const auto        value     = x + base;
        const auto        bit_width = static_cast< int >( std::bit_width( value ) ); // cast is a workaround for unresolved defect report in GCC/libc++
        const auto        width     = overflow ? input_digits : bit_width;
        const auto        zeros     = overflow ? max_zeros : width - zeros_threshold;

        encoded_count += zeros;
        for( ; encoded_count >= output_digits ; encoded_count -= output_digits )
        {
            *output++ = encoded;
            encoded   = 0;
        }

        if( overflow )
        {
            const auto shift = output_digits - ( 1 + encoded_count );

            encoded |= 1u << shift;
            ++encoded_count;
        }

        for( auto remaining = width ; remaining > 0 ; )
        {
            const auto mask = input_mask >> ( input_digits - remaining );
            const auto data = value & mask;
            const auto free = output_digits - encoded_count;

            if( remaining >= free )
            {
                const auto shift = remaining - free;

                *output++     = encoded | static_cast< OutputDataT >( data >> shift );
                encoded       = 0u;
                encoded_count = 0;
                remaining     = shift;
            }
            else
            {
                const auto shift = free - remaining;

                encoded       |= static_cast< OutputDataT >( data ) << shift;
                encoded_count += remaining;
                remaining      = 0;
            }
        }
    }

    if( encoded_count )
    {
        *output++ = encoded;
    }

    return output;
}

template< typename OutputDataT = uint8_t,
          typename InputIt,
          typename OutputIt >
requires std::signed_integral< typename std::iterator_traits< InputIt >::value_type > &&
         std::unsigned_integral< OutputDataT >
constexpr auto encode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    return encode< OutputDataT >( detail::integer_input_adapter( input ), detail::integer_input_adapter( last ), output, k );
}

template< typename OutputValueT,
          typename InputIt,
          typename OutputIt >
requires std::unsigned_integral< typename std::iterator_traits< InputIt >::value_type > &&
         std::unsigned_integral< OutputValueT >
constexpr auto decode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    using InputDataT = typename std::iterator_traits< InputIt >::value_type;

    constexpr auto input_digits = std::numeric_limits< InputDataT >::digits;
    constexpr auto input_mask   = std::numeric_limits< InputDataT >::max();

    const auto base            = static_cast< OutputValueT >( 1 ) << k;
    const auto initital_digits = 1 + static_cast< int >( k );

    InputDataT   input_buffer          = 0u;
    int          input_buffer_consumed = input_digits;
    OutputValueT output_buffer         = 0u;
    int          digits                = initital_digits;

    enum { scan_zeros, decode } state = scan_zeros;

    while( input_buffer_consumed != input_digits || input != last )
    {
        if( input_buffer_consumed == input_digits && input != last )
        {
            input_buffer          = *input++;
            input_buffer_consumed = 0;
        }

        if( state == scan_zeros )
        {
            const auto n = std::countl_zero( input_buffer );

            digits                += n - input_buffer_consumed;
            input_buffer_consumed  = n;

            if( input_buffer )
            {
                state = decode;
            }
        }
        else    // decode
        {
            const auto input_buffer_remaining = input_digits - input_buffer_consumed;

            if( digits >= input_buffer_remaining )
            {
                const auto shift = digits - input_buffer_remaining;

                output_buffer         |= input_buffer << shift;
                input_buffer_consumed  = input_digits;
                digits                -= input_buffer_remaining;
            }
            else
            {
                const auto shift = input_buffer_remaining - digits;

                output_buffer         |= input_buffer >> shift;
                input_buffer_consumed += digits;
                input_buffer          &= input_mask >> input_buffer_consumed;
                digits                 = 0u;
            }

            if( digits == 0u )
            {
                *output++     = static_cast< OutputValueT >( output_buffer - base );
                output_buffer = 0u;
                digits        = initital_digits;
                state         = scan_zeros;
            }
        }
    }

    return output;
}

template< typename OutputValueT,
          typename InputIt,
          typename OutputIt >
requires std::unsigned_integral< typename std::iterator_traits< InputIt >::value_type > &&
         std::signed_integral< OutputValueT >
constexpr auto decode( InputIt input, InputIt last, OutputIt output, size_t k = 0u ) -> OutputIt
{
    using UnsignedOutputValueT = typename std::make_unsigned< OutputValueT >::type;
    return decode< UnsignedOutputValueT >( input, last, detail::integer_output_adapter( output ), k ).it;
}

}
