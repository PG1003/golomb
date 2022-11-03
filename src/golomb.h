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
#include <limits>
#include <iterator>
#include <bit>
#include <type_traits>


namespace pg::golomb
{

template<typename InputIt>
concept integral_iterator = std::integral< typename std::iterator_traits< InputIt >::value_type > && std::input_iterator<InputIt>;

template<typename InputIt>
concept unsigned_integral_iterator = std::unsigned_integral< typename std::iterator_traits< InputIt >::value_type > && std::input_iterator<InputIt>;

namespace detail
{

template< std::signed_integral SignedT >
auto to_unsigned( SignedT s )
{
    using UnsignedT = typename std::make_unsigned< SignedT >::type;

    if( s < 0 )
    {
        return static_cast< UnsignedT >( ( ~s << 1 ) + 1 );
    }

    return static_cast< UnsignedT >( s << 1 );
}

template< std::unsigned_integral UnsignedT >
auto to_unsigned( UnsignedT u )
{
    return u;
}

template< std::signed_integral SignedT >
auto to_integral( std::unsigned_integral auto u )
{
    using UnsignedT = typename std::make_unsigned< SignedT >::type;

    const auto U = static_cast< UnsignedT >( u );
    if( U & 0x01u )
    {
        return static_cast< SignedT >( ~( U >> 1 ) );
    }

    return static_cast< SignedT >( U >> 1 );
}

template< std::unsigned_integral UnsignedT >
auto to_integral( std::unsigned_integral auto u )
{
    return static_cast< UnsignedT >( u );
}

}

template< typename OutputIt, std::unsigned_integral OutputDataT = uint8_t >
requires std::output_iterator<OutputIt, OutputDataT>
class encoder
{
    static constexpr auto output_digits = std::numeric_limits< OutputDataT >::digits;

    OutputIt     output;
    const size_t k;
    const size_t base;
    const int    zeros_threshold;

    OutputDataT encoded;
    int         encoded_count;

public:
    encoder( OutputIt output_in, size_t k_in = 0u ) noexcept
        : output( output_in )
        , k( k_in)
        , base( 1u << k_in)
        , zeros_threshold( 1 + k_in)
        , encoded( static_cast< OutputDataT>( 0u ) )
        , encoded_count( 0 )
    {}

    template< std::integral InputValueT >
    OutputIt push( InputValueT x )
    {
        using UnsignedInputValueT = typename std::make_unsigned< InputValueT >::type;
        using CommonT             = typename std::common_type< UnsignedInputValueT, OutputDataT >::type;

        static constexpr auto input_digits  = std::numeric_limits< UnsignedInputValueT >::digits;
        static constexpr auto input_mask    = std::numeric_limits< UnsignedInputValueT >::max();

        const auto overflow_threshold = input_mask - ( static_cast< OutputDataT >( base ) );
        const auto max_zeros          = input_digits - k;

        const auto u         = detail::to_unsigned( x );
        const bool overflow  = u > overflow_threshold;
        const auto value     = static_cast< UnsignedInputValueT >( u + base );
        const int  bit_width = static_cast< int >( std::bit_width( value ) ); // cast is a workaround for unresolved defect report in GCC/libc++
        const int  width     = overflow ? input_digits : bit_width;
        const int  zeros     = overflow ? max_zeros : (width - zeros_threshold);

        encoded_count += zeros;
        for( ; encoded_count >= output_digits ; encoded_count -= output_digits )
        {
            *output++ = encoded;
            encoded   = 0u;
        }

        if( overflow )
        {
            const auto shift = output_digits - ( 1 + encoded_count );

            if( shift < output_digits )
            {

                encoded |= static_cast< CommonT >( 1u ) << shift;
                if( ++encoded_count == output_digits )
                {
                    *output++     = encoded;
                    encoded       = 0u;
                    encoded_count = 0;
                }
            }
        }

        for( auto remaining = width ; remaining > 0 ; )
        {
            const auto mask = input_mask >> ( input_digits - remaining );
            const auto data = static_cast< UnsignedInputValueT >( value & mask );
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

                encoded       |= data << shift;
                encoded_count += remaining;
                remaining      = 0;
            }
        }

        return output;
    }

    OutputIt flush()
    {
        if( encoded_count )
        {
            *output++ = encoded;
            encoded   = 0u;
        }

        return output;
    }
};

template< std::unsigned_integral OutputDataT = uint8_t,
          integral_iterator InputIt,
          typename OutputIt >
requires std::output_iterator<OutputIt, OutputDataT>
constexpr auto encode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    using ValueT = typename std::iterator_traits< InputIt >::value_type;

    encoder< OutputIt, OutputDataT > e( output, k );

    while( input != last )
    {
        e.push( static_cast< ValueT >( *input++ ) );
    }

    return e.flush();
}

template< typename OutputIt, std::integral OutputValueT >
class decoder
{
    using UnsignedOutputValueT = typename std::make_unsigned< OutputValueT >::type;

    static constexpr auto output_digits = std::numeric_limits< UnsignedOutputValueT >::digits;

    OutputIt     output;
    const size_t k;
    const size_t base;
    const int    initial_digits;
    
    UnsignedOutputValueT output_buffer;
    int                  digits;
    
    enum class scan_state { scan_zeros, decode } state = scan_zeros;

public:
    decoder( OutputIt output, size_t k = 0u )
        : output( output )
        , k( k )
        , base( static_cast< UnsignedOutputValueT >( 1 ) << k )
        , initial_digits( 1 + static_cast< int >( k ) )
        , output_buffer( 0u )
        , digits( initial_digits )
        , state(scan_state::scan_zeros )
    {}

    template< std::unsigned_integral InputDataT >
    OutputIt push( InputDataT input )
    {
        using CommonT = typename std::common_type< InputDataT, UnsignedOutputValueT >::type;

        constexpr auto input_digits  = std::numeric_limits< InputDataT >::digits;
        constexpr auto input_mask    = std::numeric_limits< InputDataT >::max();

        int input_consumed = 0;
        while( input_consumed != input_digits )
        {
            if( state == scan_state::scan_zeros )
            {
                const auto n = std::countl_zero( input );

                digits         += n - input_consumed;
                input_consumed  = n;

                if( input )
                {
                    state = scan_state::decode;
                }
            }
            else    // decode
            {
                const auto input_remaining = input_digits - input_consumed;

                if( digits >= input_remaining )
                {
                    const auto shift = digits - input_remaining;

                    if( shift < output_digits )
                    {
                        output_buffer |= static_cast< CommonT >( input ) << shift;
                    }

                    input_consumed  = input_digits;
                    digits         -= input_remaining;
                }
                else
                {
                    const auto shift = input_remaining - digits;

                    output_buffer  |= input >> shift;
                    input_consumed += digits;
                    input          &= input_mask >> input_consumed;
                    digits          = 0u;
                }

                if( digits == 0u )
                {
                    *output++     = detail::to_integral< OutputValueT >( output_buffer - base );
                    output_buffer = 0u;
                    digits        = initial_digits;
                    state         = scan_state::scan_zeros;
                }
            }
        }

        return output;
    }

    OutputIt flush()
    {
        output_buffer = 0u;
        digits        = initial_digits;
        state         = scan_state::scan_zeros;
        
        return output;
    }
};

template< std::integral OutputValueT,
          unsigned_integral_iterator InputIt,
          typename OutputIt >
requires std::output_iterator<OutputIt, OutputValueT>
constexpr auto decode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    decoder< OutputIt, OutputValueT > d( output, k );

    while( input != last )
    {
        d.push( *input++ );
    }

    return d.flush();
}

}
