#include <golomb.h>
#include <cstdint>
#include <array>
#include <vector>
#include <iostream>

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
}

static void encode_narrow_to_wide_k0()
{
    const std::array< uint8_t, 2 > values = { 0x00u, 0xFFu };
    std::vector< uint32_t >        result;

    pg::golomb::encode< uint32_t >( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 1u );
    assert_same( result[ 0 ], 0x80400000u );
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

    //assert_same( result.size(), 9u );
    //assert_same( result[ 0 ], 0x80u );
    //assert_same( result[ 1 ], 0x00u );
    //assert_same( result[ 2 ], 0x00u );
    //assert_same( result[ 3 ], 0x00u );
    //assert_same( result[ 4 ], 0x40u );
    //assert_same( result[ 5 ], 0x00u );
    //assert_same( result[ 6 ], 0x00u );
    //assert_same( result[ 7 ], 0x00u );
    //assert_same( result[ 8 ], 0x00u );

    std::array< int32_t, 2 > decoded = { 1, 0 };
    pg::golomb::decode< int32_t >( result.cbegin(), result.cend(), decoded.begin(), 3u );

    assert_same( decoded.size(), 2u );
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
    const std::array< uint32_t, 1 > values = { 0x80400000u };
    std::vector< uint8_t >          result;

    pg::golomb::decode< uint8_t >( values.cbegin(), values.cend(), std::back_inserter( result ) );

    assert_same( result.size(), 2u );
    assert_same( result[ 0 ], 0x00u );
    assert_same( result[ 1 ], 0xFFu );
}

static void integer_output_adapter()
{
    const std::array< uint8_t, 7 > values = { 0x00u, 0x01u, 0x02u, 0x03u, 0x04u, 0xFEu, 0xFFu };
    std::array< int8_t, 7 >        result = {};

    std::copy( values.cbegin(),
               values.cend(),
               pg::golomb::detail::integer_output_adapter( result.begin() ) );

    assert_same( result[ 0 ],    0 );
    assert_same( result[ 1 ],   -1 );
    assert_same( result[ 2 ],    1 );
    assert_same( result[ 3 ],   -2 );
    assert_same( result[ 4 ],    2 );
    assert_same( result[ 5 ],  127 );
    assert_same( result[ 6 ], -128 );
}

static void readme()
{
    {
        const uint8_t          values[ 8 ] = { 0u, 1u, 2u, 3u, 4u, 255u, 0u, 2u };
        std::vector< uint8_t > data;

        pg::golomb::encode( std::begin( values ), std::end( values ), std::back_inserter( data ) );

        assert_same( data.size(), 5 );
    }
    {
        const uint8_t          data[ 5 ] = { 0xA6u, 0x42u, 0x80u, 0x40u, 0x2Cu };
        std::vector< int16_t > values;

        pg::golomb::decode< int16_t >( std::begin( data ), std::end( data ), std::back_inserter( values ) );

        assert_same( values.size(), 8 );
    }
}

int main()
{
    encode_all_zeros_k0();
    encode_overflow_k0();
    encode_overflow_k2();
    encode_narrow_to_wide_k0();
    encode_wide_to_narrow_k0();
    encode_wide_to_narrow_k3();
    decode_all_zeros_k0();
    decode_overflow_k0();
    decode_overflow_k2();
    decode_narrow_to_wide_k0();
    decode_wide_to_narrow_k0();
    integer_output_adapter();
    readme();

    std::cout << "Total tests: " << total_checks << ", Tests failed: " << failed_checks << '\n';

    return failed_checks ? 1 : 0;
}
