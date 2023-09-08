#include <golomb.h>
#include <cstdint>
#include <array>
#include <vector>
#include <iostream>
#include <algorithm>

static int total_checks  = 0;
static int failed_checks = 0;

static bool report_failed_check( const char* const file, const int line, const char * const condition )
{
    std::cout << "check failed! (file " << file << ", line " << line << "): " << condition << '\n';
    ++failed_checks;
    return false;
}

#define assert_true( c ) do { ++total_checks; ( c ) || report_failed_check( __FILE__, __LINE__, #c ); } while( false );
#define assert_false( c ) do { ++total_checks; !( c ) || report_failed_check( __FILE__, __LINE__, #c ); } while( false );
#define assert_same( x, y ) do { ++total_checks; ( x == y ) || report_failed_check( __FILE__, __LINE__, #x" == "#y ); } while( false );


static void encode_all_zeros_k0()
{
    const std::array< uint8_t, 8 > zeros = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    std::vector< uint8_t >         result;

    pg::golomb::encode( zeros.cbegin(), zeros.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 1u );
    assert_same( result.front(), 0xFFu );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ) );

    assert_same( decoded.size(), zeros.size() );
    assert_same( decoded[  0 ], zeros[  0 ] );
    assert_same( decoded[  1 ], zeros[  1 ] );
    assert_same( decoded[  2 ], zeros[  2 ] );
    assert_same( decoded[  3 ], zeros[  3 ] );
    assert_same( decoded[  4 ], zeros[  4 ] );
    assert_same( decoded[  5 ], zeros[  5 ] );
    assert_same( decoded[  6 ], zeros[  6 ] );
    assert_same( decoded[  7 ], zeros[  7 ] );
}

static void encode_overflow_k0()
{
    const std::array< uint8_t, 2 > ones = { 0xFFu, 0xFFu };
    std::vector< uint8_t >         result;

    pg::golomb::encode( ones.cbegin(), ones.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 5u );
    assert_same( result[ 0 ], 0x00u );
    assert_same( result[ 1 ], 0x80u );
    assert_same( result[ 2 ], 0x00u );
    assert_same( result[ 3 ], 0x40u );
    assert_same( result[ 4 ], 0x00u );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ) );

    assert_same( decoded.size(), ones.size() );
    assert_same( decoded[  0 ], ones[  0 ] );
    assert_same( decoded[  1 ], ones[  1 ] );
}

static void encode_overflow_k2()
{
    const std::array< uint8_t, 2 > ones = { 0xFFu, 0xFFu };
    std::vector< uint8_t >         result;

    pg::golomb::encode( ones.cbegin(), ones.cend(), std::back_inserter( result ), 2u );

    assert_same( result.size(), 4u );
    assert_same( result[ 0 ], 0x02u );
    assert_same( result[ 1 ], 0x06u );
    assert_same( result[ 2 ], 0x04u );
    assert_same( result[ 3 ], 0x0Cu );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ), 2u );

    assert_same( decoded.size(), ones.size() );
    assert_same( decoded[  0 ], ones[  0 ] );
    assert_same( decoded[  1 ], ones[  1 ] );
}

static void encode_narrow_to_wide_k0()
{
    const std::array< uint8_t, 12 > values = { 0x00u, 0xFFu, 0x00u, 0xFFu,
                                               0x00u, 0xFFu, 0x00u, 0xFFu,
                                               0x00u, 0xFFu, 0x00u, 0xFFu };
    std::vector< uint32_t > result;

    pg::golomb::encode< uint32_t >( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 4u );
    assert_same( result[ 0 ], 0x10204080u );
    assert_same( result[ 1 ], 0x01020408u );
    assert_same( result[ 2 ], 0x20408000u );
    assert_same( result[ 3 ], 0x00000010u );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ) );

    assert_same( decoded.size(), values.size() );
    assert_same( decoded[  0 ], values[  0 ] );
    assert_same( decoded[  1 ], values[  1 ] );
    assert_same( decoded[  2 ], values[  2 ] );
    assert_same( decoded[  3 ], values[  3 ] );
    assert_same( decoded[  4 ], values[  4 ] );
    assert_same( decoded[  5 ], values[  5 ] );
    assert_same( decoded[  6 ], values[  6 ] );
    assert_same( decoded[  7 ], values[  7 ] );
    assert_same( decoded[  8 ], values[  8 ] );
    assert_same( decoded[  9 ], values[  9 ] );
    assert_same( decoded[ 10 ], values[ 10 ] );
    assert_same( decoded[ 11 ], values[ 11 ] );
}

static void encode_narrow_to_wide_k4()
{
    const std::array< uint8_t, 8 > values = { 0x00u, 0xFFu, 0x00u, 0xFFu,
                                              0x00u, 0xFFu, 0x00u, 0xFFu };
    std::vector< uint16_t > result;

    pg::golomb::encode< uint16_t >( values.cbegin(), values.cend(), std::back_inserter( result ), 4u );

    assert_same( result.size(), 5u );
    assert_same( result[ 0 ], 0x4380u );
    assert_same( result[ 1 ], 0x10E0u );
    assert_same( result[ 2 ], 0x04F8u );
    assert_same( result[ 3 ], 0x013Eu );
    assert_same( result[ 4 ], 0x000Fu );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ), 4u );

    assert_same( decoded.size(), values.size() );
    assert_same( decoded[ 0 ], values[ 0 ] );
    assert_same( decoded[ 1 ], values[ 1 ] );
    assert_same( decoded[ 2 ], values[ 2 ] );
    assert_same( decoded[ 3 ], values[ 3 ] );
    assert_same( decoded[ 4 ], values[ 4 ] );
    assert_same( decoded[ 5 ], values[ 5 ] );
    assert_same( decoded[ 6 ], values[ 6 ] );
    assert_same( decoded[ 7 ], values[ 7 ] );
}

static void encode_narrow_to_wide_k1()
{
    const std::array< uint8_t, 8 > values = { 0x00u, 0xFFu, 0x00u, 0xFFu,
                                              0x00u, 0xFFu, 0x00u, 0xFFu };
    std::vector< uint64_t > result;

    pg::golomb::encode< uint64_t >( values.cbegin(), values.cend(), std::back_inserter( result ), 1u );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0x0106041810604080 );
    assert_same( result[ 1 ], 0x0000000000000001 );

    std::vector< uint8_t > decoded;

    pg::golomb::decode< uint8_t >( result.cbegin(), result.cend(), std::back_inserter( decoded ), 1u );

    assert_same( decoded.size(), values.size() );
    assert_same( decoded[ 0 ], values[ 0 ] );
    assert_same( decoded[ 1 ], values[ 1 ] );
    assert_same( decoded[ 2 ], values[ 2 ] );
    assert_same( decoded[ 3 ], values[ 3 ] );
    assert_same( decoded[ 4 ], values[ 4 ] );
    assert_same( decoded[ 5 ], values[ 5 ] );
    assert_same( decoded[ 6 ], values[ 6 ] );
    assert_same( decoded[ 7 ], values[ 7 ] );
}

static void encode_wide_to_narrow_k0()
{
    const std::array< uint32_t, 2 > values = { 0x00u, 0xFFFFFFFFu };
    std::vector< uint8_t >          result;

    pg::golomb::encode( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 9u );
    assert_same( result[ 0 ], 0x80u );
    assert_same( result[ 1 ], 0x00u );
    assert_same( result[ 2 ], 0x00u );
    assert_same( result[ 3 ], 0x00u );
    assert_same( result[ 4 ], 0x40u );
    assert_same( result[ 5 ], 0x00u );
    assert_same( result[ 6 ], 0x00u );
    assert_same( result[ 7 ], 0x00u );
    assert_same( result[ 8 ], 0x00u );
}

static void encode_wide_to_narrow_k3()
{
    const std::array< int32_t, 2 > values = { 2147483646, 2147483647 };
    std::vector< uint8_t >          result;

    pg::golomb::encode( values.cbegin(), values.cend(), std::back_inserter( result ), 3u );

    assert_same( result.size(), 16u );
    assert_same( result[  0 ], 0x00u );
    assert_same( result[  1 ], 0x00u );
    assert_same( result[  2 ], 0x00u );
    assert_same( result[  3 ], 0x04u );
    assert_same( result[  4 ], 0x00u );
    assert_same( result[  5 ], 0x00u );
    assert_same( result[  6 ], 0x00u );
    assert_same( result[  7 ], 0x10u );
    assert_same( result[  8 ], 0x00u );
    assert_same( result[  9 ], 0x00u );
    assert_same( result[ 10 ], 0x00u );
    assert_same( result[ 11 ], 0x10u );
    assert_same( result[ 12 ], 0x00u );
    assert_same( result[ 13 ], 0x00u );
    assert_same( result[ 14 ], 0x00u );
    assert_same( result[ 15 ], 0x60u );
}

static void decode_all_zeros_k0()
{
    const std::array< uint8_t, 1 > data = { 0xFFu };
    std::vector< uint8_t >         result;

    pg::golomb::decode< uint8_t >( data.cbegin(), data.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 8u );
    assert_same( result[ 0 ], 0x00u );
    assert_same( result[ 1 ], 0x00u );
    assert_same( result[ 2 ], 0x00u );
    assert_same( result[ 3 ], 0x00u );
    assert_same( result[ 4 ], 0x00u );
    assert_same( result[ 5 ], 0x00u );
    assert_same( result[ 6 ], 0x00u );
    assert_same( result[ 7 ], 0x00u );
}

static void decode_overflow_k0()
{
    const std::array< uint8_t, 5 > data = { 0x00u, 0x80u, 0x00u, 0x40u, 0x00u };
    std::vector< uint8_t >         result;

    pg::golomb::decode< uint8_t >( data.cbegin(), data.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0xFFu );
    assert_same( result[ 1 ], 0xFFu );
}

static void decode_overflow_k2()
{
    const std::array< uint8_t, 4 > data = { 0x02u, 0x06u, 0x04u, 0x0Cu };
    std::vector< uint8_t >         result;

    pg::golomb::decode< uint8_t >( data.cbegin(), data.cend(), std::back_inserter( result ), 2u );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0xFFu );
    assert_same( result[ 1 ], 0xFFu );
}

static void decode_narrow_to_wide_k0()
{
    const std::array< uint8_t, 9 > values = { 0x80u, 0x00u, 0x00u, 0x00u, 0x40u, 0x00u, 0x00u, 0x00u, 0x00u };
    std::vector< uint32_t >        result;

    pg::golomb::decode< uint32_t >( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0x00u);
    assert_same( result[ 1 ], 0xFFFFFFFFu );
}

static void decode_wide_to_narrow_k0()
{
    const std::array< uint32_t, 1 > values = { 0x00004080u };
    std::vector< uint8_t >          result;

    pg::golomb::decode< uint8_t >( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0x00u );
    assert_same( result[ 1 ], 0xFFu );
}

static void readme()
{
    {
        const std::array< uint8_t, 8 > values = { 0u, 1u, 2u, 3u, 4u, 255u, 0u, 2u };

        // Encoding using a range as input
        std::vector< uint8_t > out_range;

        pg::golomb::encode( values, std::back_inserter( out_range ) );

        assert_same( out_range.size(), 5 );

        // Encoding using iterators as input
        std::vector< uint8_t > out_iter;

        pg::golomb::encode( std::begin( values ), std::end( values ), std::back_inserter( out_iter ) );

        assert_same( out_iter.size(), 5 );

        assert_true( std::ranges::equal( out_range, out_iter ) );
    }
    {
        const std::array< uint8_t, 5 > data = { 0xA6u, 0x42u, 0x80u, 0x40u, 0x2Cu };

        // Decoding using a ranges as input
        std::vector< int16_t > values_range;

        pg::golomb::decode< int16_t >( data, std::back_inserter( values_range ) );

        assert_same( values_range.size(), 8 );

        // Decoding using iterators as input
        std::vector< int32_t > values_iter;

        pg::golomb::decode< int32_t >( std::begin( data ), std::end( data ), std::back_inserter( values_iter ) );

        assert_same( values_iter.size(), 8 );

        assert_true( std::ranges::equal( values_range, values_iter ) );
    }
}

int main()
{
    encode_all_zeros_k0();
    encode_overflow_k0();
    encode_overflow_k2();
    encode_narrow_to_wide_k0();
    encode_narrow_to_wide_k4();
    encode_wide_to_narrow_k0();
    encode_wide_to_narrow_k3();
    encode_narrow_to_wide_k1();
    decode_all_zeros_k0();
    decode_overflow_k0();
    decode_overflow_k2();
    decode_narrow_to_wide_k0();
    decode_wide_to_narrow_k0();
    readme();

    std::cout << "Total tests: " << total_checks << ", Tests failed: " << failed_checks << '\n';

    return failed_checks ? 1 : 0;
}
