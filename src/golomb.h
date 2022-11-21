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
#include <ranges>
#include <limits>
#include <iterator>
#include <bit>
#include <type_traits>


namespace pg::golomb
{

namespace detail
{

template< typename InputIt >
concept integral_input_iterator = std::integral< typename std::iterator_traits< InputIt >::value_type > &&
                                  std::input_iterator< InputIt >;

template< typename InputIt >
concept unsigned_integral_input_iterator = std::unsigned_integral< typename std::iterator_traits< InputIt >::value_type > &&
                                           std::input_iterator< InputIt >;

template< typename InputRange >
concept integral_input_range = std::integral< std::ranges::range_value_t< InputRange > > &&
                               std::ranges::input_range< InputRange >;

template< typename InputRange >
concept unsigned_integral_input_range = std::unsigned_integral< std::ranges::range_value_t< InputRange > > &&
                                        std::ranges::input_range< InputRange >;


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
    return u;
}

}

/**
 * \brief Golomb encoder object that encodes integral values and write them to an output
 *
 * The encoded values have a variable number of bits that are buffered and packed.
 * The buffer size is equal to the \em OutputDataT and written to the output when all bits are set by the encoder.
 * 
 * \tparam OutputIt    The output iterator type that is used to write the encoded values to
 * \tparam OutputDataT The unsigned integral type used as value type that is written to output
 */
template< typename OutputIt, std::unsigned_integral OutputDataT = uint8_t >
requires std::output_iterator< OutputIt, OutputDataT >
class encoder
{
    static constexpr auto output_digits = std::numeric_limits< OutputDataT >::digits;

    OutputIt          output;
    const size_t      k;
    const OutputDataT base;
    const size_t      zeros_threshold;

    OutputDataT encoded;
    size_t      encoded_count;

public:
    /**
     * \brief Construct the encoder
     *
     * \param output Output iterator to which the encoded values are written
     * \param k      Order of in which the values are encoded
     * 
     * \note Be sure that `output` can buffer the resulting encoded bitstream.
     */
    encoder( OutputIt output, size_t k = 0u ) noexcept
        : output( output )
        , k( k )
        , base( 1u << k )
        , zeros_threshold( 1 + k )
        , encoded( static_cast< OutputDataT>( 0u ) )
        , encoded_count( 0 )
    {}

    ~encoder()
    {
        flush();
    }
    
    /**
     * \brief Encodes the given value and writes the resulting bitstream to output
     *
     * \param x The value to encode
     * 
     * \return The output iterator one past the data that has been written to the output
     */
    template< std::integral InputValueT >
    OutputIt push( InputValueT x )
    {
        using UnsignedInputValueT = typename std::make_unsigned< InputValueT >::type;
        using CommonT             = typename std::common_type< UnsignedInputValueT, OutputDataT >::type;

        static constexpr auto input_digits  = std::numeric_limits< UnsignedInputValueT >::digits;
        static constexpr auto input_mask    = std::numeric_limits< UnsignedInputValueT >::max();

        const auto overflow_threshold = input_mask - static_cast< OutputDataT >( base );
        const auto max_zeros          = input_digits - k;

        const auto   u         = detail::to_unsigned( x );
        const bool   overflow  = u > overflow_threshold;
        const auto   value     = static_cast< UnsignedInputValueT >( u + base );
        const size_t bit_width = static_cast< int >( std::bit_width( value ) ); // cast is a workaround for unresolved defect report in GCC/libc++
        const size_t width     = overflow ? input_digits : bit_width;
        const size_t zeros     = overflow ? max_zeros : width - zeros_threshold;

        encoded_count += zeros;
        for( ; encoded_count >= output_digits ; encoded_count -= output_digits )
        {
            *output++ = encoded;
            encoded   = 0u;
        }

        if( overflow )
        {
            const auto shift = output_digits - ( 1u + encoded_count );

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
                encoded_count = 0u;
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
    
    /**
     * \brief Flushes the internal bitbuffer to output
     *
     * \return The output iterator one past the data that has been flushed to the output
     */
    OutputIt flush()
    {
        if( encoded_count )
        {
            *output++     = encoded;
            encoded_count = 0u;
            encoded       = 0u;
        }

        return output;
    }
};

/**
 * \brief Encodes integeral values from an input with a specified golomb order 
 *
 * \param input  An input iterator to read integral values from
 * \param last   The iterator that marks the end of input range
 * \param output The output iterator to which the ecoded values are written to
 * \param k      Order in which the values from input will be encoded
 * 
 * \note Be sure that `out` can buffer all the data that is decoded from the input.
 */
template< std::unsigned_integral OutputDataT = uint8_t,
          detail::integral_input_iterator InputIt,
          typename OutputIt >
requires std::output_iterator< OutputIt, OutputDataT >
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

/**
 * \overload encode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
 * 
 * \param input  A range to read integral values from that are encoded
 * \param output The output iterator to which the ecoded values are written to
 * \param k      Order in which the values from input will be encoded
 */
template< std::unsigned_integral OutputDataT = uint8_t,
          detail::integral_input_range InputRangeT,
          typename OutputIt >
requires std::output_iterator< OutputIt, OutputDataT >
auto encode( const InputRangeT & input, OutputIt output, size_t k = 0u )
{
    using InputValueT = std::ranges::range_value_t< InputRangeT >;

    encoder< OutputIt, OutputDataT > e( output, k );

    for( const auto& value : input )
    {
        e.push( static_cast< InputValueT >( value ) );
    }

    return e.flush();
}

/**
 * \brief Golomb decoder object that decodes golomb data and write its values to an output
 *
 * All data fed to the decoder is scanned immediately. The values found while scanning are written to the output.
 * Remainig bits are buffered and processed when more data is fed to the decoder.
 *
 * \tparam OutputIt    The output iterator type that is used to write the decoded values to
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \note Be sure the values encoded in the golomb data fits within value range of \em OutputDataT.
 */
template< typename OutputIt, std::integral OutputValueT >
class decoder
{
    using UnsignedOutputValueT = typename std::make_unsigned< OutputValueT >::type;

    static constexpr auto output_digits = std::numeric_limits< UnsignedOutputValueT >::digits;

    OutputIt                   output;
    const size_t               k;
    const UnsignedOutputValueT base;
    const size_t               initial_digits;
    
    UnsignedOutputValueT output_buffer;
    size_t               digits;
    
    enum class scan_state { scan_zeros, decode } state = scan_state::scan_zeros;

public:
    /**
     * \brief Construct the decoder
     *
     * \param output Output iterator to which the decoded values are written
     * \param k      Order of in which the values from the golomb data are encoded
     * 
     * \note Be sure that `output` can buffer the resulting encoded bitstream.
     * 
     * \note You must decode the binary golomb data with the same order as it is encoded.
     */
    decoder( OutputIt output, size_t k = 0u )
        : output( output )
        , k( k )
        , base( static_cast< UnsignedOutputValueT >( 1u ) << k )
        , initial_digits( 1 + static_cast< int >( k ) )
        , output_buffer( 0u )
        , digits( initial_digits )
        , state( scan_state::scan_zeros )
    {}
    
    /**
     * \brief Feeds encoded golomb data and outputs the values the data holds
     *
     * Remaining bits are buffered and processed with the next call of this function.
     *
     * \param data Binary golomb data
     * 
     * \return The output iterator one past the values that has been written to the output
     */
    template< std::unsigned_integral InputDataT >
    OutputIt push( InputDataT data )
    {
        using CommonT = typename std::common_type< InputDataT, UnsignedOutputValueT >::type;

        static constexpr auto data_digits  = std::numeric_limits< InputDataT >::digits;
        static constexpr auto data_mask    = std::numeric_limits< InputDataT >::max();

        size_t data_consumed = 0;
        while( data_consumed != data_digits )
        {
            if( state == scan_state::scan_zeros )
            {
                const auto n = std::countl_zero( data );

                digits        += n - data_consumed;
                data_consumed  = n;

                if( data )
                {
                    state = scan_state::decode;
                }
            }
            else    // decode
            {
                const auto input_remaining = data_digits - data_consumed;

                if( digits >= input_remaining )
                {
                    const auto shift = digits - input_remaining;

                    if( shift < output_digits )
                    {
                        output_buffer |= static_cast< CommonT >( data ) << shift;
                    }

                    data_consumed = data_digits;
                    digits       -= input_remaining;
                }
                else
                {
                    const auto shift = input_remaining - digits;

                    output_buffer |= data >> shift;
                    data_consumed += digits;
                    data          &= data_mask >> data_consumed;
                    digits         = 0u;
                }

                if( digits == 0u )
                {
                    *output++     = detail::to_integral< OutputValueT >( static_cast< UnsignedOutputValueT >( output_buffer - base ) );
                    output_buffer = 0u;
                    digits        = initial_digits;
                    state         = scan_state::scan_zeros;
                }
            }
        }

        return output;
    }
    
    /**
     * \brief Resets the decoder
     *
     * \return The output iterator one past the last written value
     */
    OutputIt flush()
    {
        output_buffer = 0u;
        digits        = initial_digits;
        state         = scan_state::scan_zeros;
        
        return output;
    }
};

/**
 * \brief Decodes binary golomb data from an input with a specified golomb order 
 *
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \param input  An input iterator to read binary golomb data from
 * \param last   The iterator that marks the end of input range
 * \param output The output iterator to which the decoded values are written to
 * \param k      Order in which the data from input is encoded
 *
 * \note Be sure the values encoded in the golomb data fits within value range of \em OutputDataT.
 *
 * \note Be sure that `out` can buffer all the data that is decoded from the input.
 *
 * \note You must decode the binary golomb data with the same order as it is encoded.
 *
 */
template< std::integral OutputValueT,
          detail::unsigned_integral_input_iterator InputIt,
          typename OutputIt >
requires std::output_iterator< OutputIt , OutputValueT >
constexpr auto decode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
{
    decoder< OutputIt, OutputValueT > d( output, k );

    while( input != last )
    {
        d.push( *input++ );
    }

    return d.flush();
}

/**
 * \overload decode( InputIt input, InputIt last, OutputIt output, size_t k = 0u )
 *
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \param input  An input range to read binary golomb data from
 * \param output The output iterator to which the decoded values are written to
 * \param k      Order in which the data from input is encoded
 */
template< std::integral OutputValueT,
          detail::unsigned_integral_input_range InputRangeT,
          typename OutputIt >
requires std::output_iterator< OutputIt , OutputValueT >
constexpr auto decode( InputRangeT && input, OutputIt output, size_t k = 0u )
{
    decoder< OutputIt, OutputValueT > d( output, k );

    for( const auto& value : input )
    {
        d.push( value );
    }

    return d.flush();
}

}
