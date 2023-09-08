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
#include <algorithm>
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

template< typename InputValueT, typename OutputDataT >
concept overruns_buffer = std::numeric_limits< typename std::make_unsigned< InputValueT >::type >::digits >=
                          std::numeric_limits< OutputDataT >::digits;

template< std::integral ValueT >
[[nodiscard]] constexpr ValueT make_mask( size_t n_bits )
{
    if( n_bits )
    {
        constexpr auto max_digits = std::numeric_limits< ValueT >::digits;

        return static_cast< ValueT >( -1 ) >> ( max_digits - n_bits );
    }

    return {};
}

template< std::unsigned_integral DataT >
[[nodiscard]] constexpr auto fix_endian( DataT data )
{
    static_assert( std::endian::native == std::endian::big ||
                   std::endian::native == std::endian::little );

    if constexpr( std::endian::native == std::endian::big ||
                  sizeof( DataT ) == 1 )
    {
        return data;
    }
    else
    {
#if defined( __cpp_lib_byteswap )
        return std::byteswap( data );
#else
        using RawDataT = std::array< uint8_t, sizeof( DataT ) >;

        auto raw_data = reinterpret_cast< RawDataT* >( &data );

        std::ranges::reverse( *raw_data );

        return data;
#endif
    }
}

template< typename OutputIt, std::unsigned_integral OutputDataT >
requires std::output_iterator< OutputIt, OutputDataT >
constexpr void write( OutputIt& output, OutputDataT data )
{
    *output++ = fix_endian( data );
}

template< detail::unsigned_integral_input_iterator InputIt >
[[nodiscard]] constexpr auto read( InputIt& input )
{
    return fix_endian( *input++ );
}

}

template< std::signed_integral SignedT >
[[nodiscard]] constexpr auto to_unsigned( SignedT s )
{
    using UnsignedT = typename std::make_unsigned< SignedT >::type;

    if( s < 0 )
    {
        return static_cast< UnsignedT >( ( ~s << 1 ) + 1 );
    }

    return static_cast< UnsignedT >( s << 1 );
}

template< std::unsigned_integral UnsignedT >
[[nodiscard]] constexpr auto to_unsigned( UnsignedT u )
{
    return u;
}

template< std::signed_integral SignedT >
[[nodiscard]] constexpr auto to_integral( std::unsigned_integral auto u )
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
[[nodiscard]] constexpr auto to_integral( std::unsigned_integral auto u )
{
    return u;
}


/**
 * \brief Golomb encoder object that encodes integral values and write them to an output
 *
 * The encoded values have a variable number of bits that are buffered and packed.
 * The buffer size is equal to the \em OutputDataT and written to the output when all bits are set by the encoder.
 * 
 * \tparam OutputIt     The type of the output iterator to which the encoded values are written
 * \tparam OutputDataT  An unsigned integral type which is used for the data that is written to output
 */
template< typename OutputIt, std::unsigned_integral OutputDataT = uint8_t >
requires std::output_iterator< OutputIt, OutputDataT >
class encoder
{
    static constexpr auto output_digits = std::numeric_limits< OutputDataT >::digits;

    OutputIt    output;
    OutputDataT buffer;
    int         buffer_bits_free;

    template< std::integral InputValueT, size_t K >
    constexpr OutputIt push_switch( InputValueT x, size_t k )
    {
        using UnsignedInputValueT = std::make_unsigned< InputValueT >::type;

        constexpr auto max_K = std::numeric_limits< UnsignedInputValueT >::digits - 1;
        if constexpr( K < max_K )
        {
            if( k == K )
            {
                return push< K, InputValueT >( x );
            }

            return push_switch< InputValueT, K + 1u >( x, k );
        }
        else
        {
            return push< max_K >( x );
        }
    }

public:
    /**
     * \brief Construct the encoder
     *
     * \param output  Output iterator to which the encoded values are written
     *
     * \note Be sure that `output` can buffer the resulting encoded bitstream.
     */
    constexpr encoder( OutputIt output )
        : output( output )
        , buffer( static_cast< OutputDataT>( 0u ) )
        , buffer_bits_free( output_digits )
    {}

    /**
     * \brief Encodes the given value and writes the resulting bitstream to output
     *
     * \tparam k            Golomb order in which \em x will be encoded
     * \tparam InputValueT  Integral type of value \em x
     *
     * \param x  The value to encode
     *
     * \return The output iterator one past the data that has been written to the output
     */
    template< size_t k, typename InputValueT >
    requires std::integral< InputValueT > && detail::overruns_buffer< InputValueT, OutputDataT >
    constexpr OutputIt push( InputValueT value )
    {
        using UnsignedInputValueT = typename std::make_unsigned< InputValueT >::type;

        constexpr auto base         = static_cast< UnsignedInputValueT >( 1u ) << k;
        constexpr auto value_digits = std::numeric_limits< UnsignedInputValueT >::digits;

        const auto unsigned_value = to_unsigned( value );
        auto       data           = static_cast< UnsignedInputValueT >( unsigned_value + base );

        const auto overflowed         = data < unsigned_value;
        auto       data_bits_to_write = overflowed ? value_digits : std::bit_width( data );
        auto       zeros              = data_bits_to_write - static_cast< int >( overflowed ? k : k + 1 );

        while( zeros >= buffer_bits_free )
        {
            detail::write( output, buffer );
            buffer            = {};
            zeros            -= buffer_bits_free;
            buffer_bits_free  = output_digits;
        }
        buffer_bits_free -= zeros;

        if( overflowed )
        {
            --buffer_bits_free;
            buffer |= static_cast< OutputDataT >( 1u ) << buffer_bits_free;
        }

        while( data_bits_to_write > 0 )
        {
            if( buffer_bits_free == 0 )
            {
                detail::write( output, buffer );
                buffer           = {};
                buffer_bits_free = output_digits;
            }
            else
            {
                if( data_bits_to_write < buffer_bits_free )
                {
                    const auto shift = buffer_bits_free - data_bits_to_write;

                    buffer             |= data << shift;
                    buffer_bits_free   -= data_bits_to_write;
                    data_bits_to_write  = {};
                }
                else
                {
                    const auto shift = data_bits_to_write - buffer_bits_free;
                    const auto mask  = detail::make_mask< UnsignedInputValueT >( buffer_bits_free ) << shift;
                    const auto chunk = data & mask;

                    buffer             |= chunk >> shift;
                    data_bits_to_write -= buffer_bits_free;
                    buffer_bits_free    = {};
                    data               &= ~mask;
                }
            }
        }

        return output;
    }

    /**
     * \overload OutputIt push( InputValueT value )
     */
    template< size_t k, typename InputValueT >
    requires std::integral< InputValueT > && ( !detail::overruns_buffer< InputValueT, OutputDataT > )
    constexpr OutputIt push( InputValueT value )
    {
        constexpr auto base = static_cast< OutputDataT >( 1u ) << k;

        const auto unsigned_value     = to_unsigned( value );
        auto       data               = static_cast< OutputDataT >( unsigned_value + base );
        auto       data_bits_to_write = std::bit_width( data );
        auto       zeros              = data_bits_to_write - static_cast< int >( k + 1 );

        if( zeros >= buffer_bits_free )
        {
            detail::write( output, buffer );
            buffer            = {};
            zeros            -= buffer_bits_free;
            buffer_bits_free  = output_digits;
        }
        buffer_bits_free -= zeros;

        if( data_bits_to_write >= buffer_bits_free )
        {
            const auto shift  = data_bits_to_write - buffer_bits_free;
            const auto shift2 = output_digits - shift;

            buffer |= data >> shift;
            detail::write( output, buffer );

            buffer           = data << shift2;
            buffer_bits_free = shift2;
        }
        else
        {
            const auto shift = buffer_bits_free - data_bits_to_write;

            buffer             |= data << shift;
            buffer_bits_free   -= data_bits_to_write;
        }

        return output;
    }

    template< std::integral InputValueT >
    constexpr OutputIt push( InputValueT x, size_t k )
    {
        return push_switch< InputValueT, 0u >( x, k );
    }

    /**
     * \brief Flushes the internal bitbuffer to output
     *
     * \return The output iterator one past the data that has been flushed to the output
     */
    constexpr OutputIt flush()
    {
        if( buffer_bits_free < output_digits )
        {
            detail::write( output, buffer );
            buffer           = {};
            buffer_bits_free = output_digits;
        }

        return output;
    }
};

/**
 * \brief Encodes integeral values from an input with a specified golomb order 
 *
 * \param input  An input iterator to read integral values from
 * \param last   The iterator that marks the end of input range
 * \param output The output iterator to which the ecoded values are written
 * \param k      The order the values from input will be encoded
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

    encoder< OutputIt, OutputDataT > e( output );

    while( input != last )
    {
        e.push( static_cast< ValueT >( *input++ ), k );
    }

    return e.flush();
}

/**
 * \overload encode( InputIt input, InputIt last, OutputIt output, size_t k = {} )
 * 
 * \param input  A range to read integral values from that are encoded
 * \param output The output iterator to which the ecoded values are written
 * \param k      The order the values from input will be encoded
 */
template< std::unsigned_integral OutputDataT = uint8_t,
          detail::integral_input_range InputRangeT,
          typename OutputIt >
requires std::output_iterator< OutputIt, OutputDataT >
constexpr auto encode( InputRangeT input, OutputIt output, size_t k = {} )
{
    using InputValueT = std::ranges::range_value_t< InputRangeT >;

    encoder< OutputIt, OutputDataT > e( output );

    for( const auto& value : input )
    {
        e.push( static_cast< InputValueT >( value ), k );
    }

    return e.flush();
}

/**
 * \brief Golomb decoder_result object that holds the decoded value and/or the decoder status
 */
template< std::integral OutputValueT >
struct decoder_result
{
    enum status
    {
        success,        ///< Decoded successfuly; value is valid
        done,           ///< No more data available to decode
        zero_overflow,  ///< Exceeded leading zeros for \em OutputVaueT and given order;
                        // value holds the number of zeros (clipped at max for OutputValueT).
    };

    OutputValueT value;
    status       status;
};

/**
 * \brief Golomb decoder object that decodes golomb data and write its values to an output
 *
 * All data fed to the decoder is scanned immediately. The values found while scanning are written to the output.
 * Remainig bits are buffered and processed when more data is fed to the decoder.
 *
 * \tparam OutputIt    The type of the output iterator to which the decoded values are written
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \note Be sure the values encoded in the golomb data fits within value range of \em OutputDataT.
 */
template< detail::unsigned_integral_input_iterator InputIt >
class decoder
{
    using InputDataT = typename std::iterator_traits< InputIt >::value_type;

    static constexpr auto data_digits = std::numeric_limits< InputDataT >::digits;
    static constexpr auto data_mask   = std::numeric_limits< InputDataT >::max();

    InputIt    input;
    InputIt    input_end;
    size_t     data_bits_remaining;
    InputDataT data;

    template< std::integral OutputValueT, size_t K = {} >
    [[nodiscard]] decoder_result< OutputValueT > constexpr pull_switch( size_t k )
    {
        using UnsignedOutputValueT = typename std::make_unsigned< OutputValueT >::type;

        constexpr auto max_K = std::numeric_limits< UnsignedOutputValueT >::digits - 1;
        if constexpr ( K < max_K )
        {
            if( k == K )
            {
                return pull< OutputValueT, K >();
            }

            return pull_switch< OutputValueT, K + 1u >( k );
        }
        else
        {
            return pull< OutputValueT, max_K >();
        }
    }

    [[nodiscard]] constexpr bool check_data()
    {
        if( data_bits_remaining )
        {
            return true;
        }
        else if( input != input_end )
        {
            data                = detail::read( input );
            data_bits_remaining = data_digits;

            return true;
        }

        return false;
    }

public:
    /**
     * \brief Construct the decoder
     *
     * \param input      Begin iterator of the decoder's input containing encoded golomb data.
     * \param input_end  End iterator that marks the end of the input data.
     */
    [[nodiscard]] constexpr decoder( InputIt input_, InputIt input_end_ )
        : input( input_ )
        , input_end( input_end_ )
        , data_bits_remaining( input == input_end ? 0u : data_digits )
        , data( input == input_end ? 0u : detail::read( input ) )
    {}

    /**
     * \brief Reads data from its input and returns a decoded result
     *
     * \tparam OutputValueT  Type of the value that is pulled
     * \tparam k             Order of the golomb data to decode for the value that is pulled
     *
     * \note \em k should be smaler than the \em OutputValueT's maximum number of binary digits.
     *
     * \return A \em decoder_result struct containing the decoded value and/or decoder status
     */
    template< std::integral OutputValueT, size_t k = {} >
    [[nodiscard]] decoder_result< OutputValueT > constexpr pull()
    {
        using ResultT              = decoder_result< OutputValueT >;
        using UnsignedOutputValueT = typename std::make_unsigned< OutputValueT >::type;
        using CommonT              = typename std::common_type< InputDataT, UnsignedOutputValueT >::type;

        constexpr auto max_value_digits = std::numeric_limits< UnsignedOutputValueT >::digits;

        static_assert( k < max_value_digits );

        // Scan zeros
        size_t zeros = {};
        do
        {
            if( !check_data() )
            {
                return { {}, ResultT::done };
            }

            const auto bit_width = std::bit_width( data );
            const auto counted   = data_bits_remaining - bit_width;

            zeros               += counted;
            data_bits_remaining  = bit_width;
        }
        while( !data );

        // Skip and clear the '1' that followed the zeros
        data_bits_remaining -= 1;
        data                &= ~( static_cast< InputDataT >( 1u ) << data_bits_remaining );

        size_t digits = zeros + k;
        if( digits > max_value_digits )
        {
            constexpr auto max_output_value = std::numeric_limits< OutputValueT >::max();
            const auto     zeros_clamped    = std::min( static_cast< size_t >( max_output_value ), zeros );

            return { static_cast< OutputValueT >( zeros_clamped ), ResultT::zero_overflow };
        }

        // Read value
        UnsignedOutputValueT buffer = {};
        do
        {
            if( digits && !check_data() )
            {
                return { {}, ResultT::done };
            }

            if( digits >= data_bits_remaining )
            {
                const auto shift = digits - data_bits_remaining;

                buffer |= static_cast< CommonT >( data ) << shift;

                digits              -= data_bits_remaining;
                data_bits_remaining  = {};
            }
            else
            {
                const auto shift = data_bits_remaining - digits;

                buffer              |= data >> shift;
                data_bits_remaining -= digits;
                digits               = {};

                const auto mask = detail::make_mask< InputDataT >( data_bits_remaining );

                data &= mask;
            }
        }
        while( digits );

        const auto base           = detail::make_mask< UnsignedOutputValueT >( zeros ) << k;
        const auto buffered_value = static_cast< UnsignedOutputValueT >( buffer + base );
        const auto value          = to_integral< OutputValueT >( buffered_value );

        return { value, ResultT::success };
    }

    /**
     * \overload pull() noexcept
     *
     * \tparam  OutputValueT  Type of the value that is pulled
     * 
     * \param k  Order of the golomb data to decode for the value that is pulled
     *
     * \note \em k should be smaler than the \em OutputValueT's maximum number of binary digits.
     *
     * \return A \em decoder_result struct containing the decoded value and/or decoder status
     */
    template< std::integral OutputValueT >
    [[nodiscard]] constexpr auto pull( size_t k )
    {
        return pull_switch< OutputValueT >( k );
    }

    /**
     * \brief Checks if the decoder has ready data to decode
     *
     * \return True when the decoder has data buffered or has not yet reached the end of its input
     *
     * \note The function returns true in case input contains but which is not enough to decode a valid value.
     */
    [[nodiscard]] constexpr bool has_data() const
    {
        return data_bits_remaining || input != input_end;
    }
};

/**
 * \brief Decodes binary golomb data from an input with a specified golomb order 
 *
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \param input   The input iterator from which the read binary golomb data is read from
 * \param last    The iterator that marks the end of input range
 * \param output  The output iterator to which the decoded values are written
 * \param k       The order the values in the golomb data are encoded
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
constexpr auto decode( InputIt input, InputIt last, OutputIt output, size_t k = {} )
{
    decoder d( input, last );

    while( d.has_data() )
    {
        const auto [ value, status ] = d.template pull< OutputValueT >( k );
        if( status == decoder_result< OutputValueT >::success )
        {
            *output++ = value;
        }
    }

    return output;
}

/**
 * \overload decode( InputIt input, InputIt last, OutputIt output, size_t k = {} )
 *
 * \tparam OutputDataT The integral type of the decoded values
 *
 * \param input  An input range to read binary golomb data from
 * \param output The output iterator to which the decoded values are written
 * \param k      The order in which the data from input is encoded
 */
template< std::integral OutputValueT,
          detail::unsigned_integral_input_range InputRangeT,
          typename OutputIt >
requires std::output_iterator< OutputIt , OutputValueT >
constexpr auto decode( InputRangeT input, OutputIt output, size_t k = 0u )
{
    return decode< OutputValueT >( std::begin( input ), std::end( input ), output, k );
}

}
