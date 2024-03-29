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

#include <golomb.h>
#include <bit>
#include <iterator>
#include <string>
#include <string_view>
#include <charconv>
#include <cassert>
#include <cstdarg>
#include <cctype>
#include <cstdio>


static void golomb_argument_error( const char * const format, ... )
{
    va_list args;
    va_start( args, format );

    std::vfprintf( stderr, format, args );
    std::fputc( '\n', stderr );
    std::puts( "Use the '-h' option to read about the usage of this program." );

    va_end( args );

    std::exit( 1 );
}

static void golomb_errno( const char * const prefix )
{
    std::perror( prefix );
    std::exit( 1 );
}


// Helper for parsing posix style commandline arguments.
struct options
{
    options( const int argc, const char ** const argv )
        : argc( argc )
        , argv( argv )
        , index( 1 )    // Skip executable name
        , opt( nullptr )
    {}

    // Reads an option character.
    // Returns a '\0' when no more options are available, however there may be additional operands.
    char read_option()
    {
        if( ( index < argc ) && ( opt == nullptr || *opt == '\0' ) )
        {
            opt = argv[ index++ ];
            if( *opt != '-' || opt[ 1 ] == '\0' )
            {
                return '\0';    // End of option list, doesn't start with '-' or a single '-' detected
            }
            if( *++opt == '-' )
            {
                ++opt;
                return '\0';    // End of option list, '--' marker
            }
        }

        return opt ? *opt++ : '\0';
    }

    // Reads a option argument or operand
    // An empty std::string_view is returned when no more arguments are available.
    std::string_view read_argument()
    {
        const char * arg = opt;
        if( ( index < argc ) && ( arg == nullptr || *arg == '\0' ) )
        {
            arg = argv[ index++ ];
        }
        opt = nullptr;

        return arg ? std::string_view( arg ) : std::string_view();
    }

private:
    const int           argc;
    const char ** const argv;
    int                 index;
    const char *        opt;
};

template< typename T >
struct binary_input_file_iterator
{
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = T *;
    using reference         = T &;
    using iterator_category = std::input_iterator_tag;

    [[nodiscard]] binary_input_file_iterator( std::FILE * file = nullptr )
        : file( file )
    {
        if( file )
        {
            next();
        }
    }

    [[nodiscard]] bool operator==( const binary_input_file_iterator & other ) const
    {
        if( file == nullptr || other.file == nullptr )
        {
            return file == other.file;
        }

        return file == other.file && std::ftell( file ) == std::ftell( other.file );
    }

    [[nodiscard]] bool operator!=( const binary_input_file_iterator & other ) const
    {
        return !operator==( other );
    }

    const value_type &           operator*()  const noexcept { return value; }
    binary_input_file_iterator * operator->() const noexcept { return &value; }
    binary_input_file_iterator & operator++() noexcept       { next(); return *this; }
    binary_input_file_iterator   operator++( int ) noexcept  { auto it = *this; next(); return it; }

private:
    std::FILE * file  = nullptr;
    T           value = T();

    void next()
    {
        assert( file != nullptr );

        if( std::fread( &value, sizeof( T ), 1, file ) )
        {
            return;
        }
        else if( std::feof( file ) )
        {
            file = nullptr;
            return;
        }

        if( const int err = std::ferror( file ) )
        {
            golomb_errno( "Input" );
        }
    }
};

template< typename T >
struct binary_input_file
{
    using value_type      = T;
    using const_pointer   = const T *;
    using const_reference = const T &;
    using difference_type = std::ptrdiff_t;
    using iterator        = binary_input_file_iterator< T >;

    std::FILE * file  = nullptr;

    [[nodiscard]] binary_input_file( std::FILE * file ) noexcept
        : file( file )
    {}

    [[nodiscard]] auto begin() const noexcept -> iterator
    {
        return iterator( file );
    }

    [[nodiscard]] auto end() const noexcept -> iterator
    {
        return iterator();
    }
};

template< typename T >
struct binary_output_file_iterator
{
    using difference_type   = std::ptrdiff_t;
    using value_type        = void;
    using pointer           = void;
    using reference         = void;
    using iterator_category = std::output_iterator_tag;

    [[nodiscard]] binary_output_file_iterator( std::FILE * file = nullptr ) noexcept
        : file( file )
    {}

    T operator=( T value ) noexcept
    {
        assert( file );

        if( std::fwrite( &value, sizeof( T ), 1, file ) != 1 )
        {
            std::perror( "output" );
            std::exit( errno );
        }

        return value;
    }

    [[nodiscard]] bool operator==( const binary_output_file_iterator & other ) const noexcept
    {
        if( file == nullptr || other.file == nullptr )
        {
            return file == other.file;
        }

        return file == other.file && std::ftell( file ) == std::ftell( other.file );
    }

    [[nodiscard]] bool operator!=( const binary_output_file_iterator & other ) const noexcept
    {
        return !operator==( other );
    }

    binary_output_file_iterator & operator*() noexcept       { return *this; }
    binary_output_file_iterator & operator++() noexcept      { return *this; }
    binary_output_file_iterator   operator++( int ) noexcept { return *this; }

private:
    std::FILE * file = nullptr;
};


static void print_help()
{
    const char * const help =
        "golomb v1.0.0\n"
        "\n"
        "A tool to compress or expand binary data using Exponential Golomb Encoding.\n"
        "\n"
        "SYNOPSIS\n"
        "    golomb [-aN] [-{e|d}[FORMAT]] [-h] [-kN] input output\n"
        "\n"
        "DESCRIPTION\n"
        "    golomb reduces the size of its input by using Exponential Golomb Encoding\n"
        "    that uses a variable number of bits per value. Small numbers use less bits\n"
        "    than large numbers.\n"
        "\n"
        "    With this utility you can test the efficiency of the compression for your\n"
        "    use case.\n"
        "\n"
        "    The advantage of the Exponential Golomb Encoding over other compression\n"
        "    methods is that it compresses data in a single pass and does not require any\n"
        "    buffering of the input or output data. These properties may be a good fit\n"
        "    for applications that are tight on memory usage or require low latencies.\n"
        "    However due to its simplicity of Exponential Golomb Encoding the compression\n"
        "    may not be as good as achieved by other utilities.\n"
        "\n"
        "OPTIONS\n"
        "    -aN         Enable adaptive mode with factor 'N', must be a positive number.\n"
        "    -e[FORMAT]  Encode and specifies the input format, default format is 'u8'.\n"
        "    -d[FORMAT]  Decode and specifies the output format, default format is 'u8'.\n"
        "    -h          Shows this help.\n"
        "    -kN         Order 'N', must be a positive number. Default is '0'.\n"
        "\n"
        "ADAPTIVE MODE\n"
        "    When adaptive mode is anabled the golomb order automatically is adjusted"
        "    based on the processed data. For each value the optimum golomb order is"
        "    calculated. A simple smoothing filter with is applied. The"
        "    result is used to encode the next value."
        "\n"
        "    The order cannot be negative and must be smaller than the number of bits of\n"
        "    the values that are encoded or decoded.\n"
        "\n"
        "    The filter used to caculate the order is an exponential smoothing filter.\n"
        "    The filter factor is caculated as 2^N\n"
        "    The order passed with option 'k' is used to initialize the filter.\n"
        "\n"
        "    You must use the same adaptive mode to decode glomb data as it was encoded.\n"
        "\n"
        "FORMAT\n"
        "    The following formats are supported:\n"
        "\n"
        "    Format |   Description\n"
        "    -------------------------\n"
        "       i8  |   signed  8 bit\n"
        "       u8  | unsinged  8 bit\n"
        "      i16  |   signed 16 bit\n"
        "      u16  | unsigned 16 bit\n"
        "      i32  |   signed 32 bit\n"
        "      u32  | unsinged 32 bit\n"
        "      i64  |   signed 64 bit\n"
        "      u64  | unsigned 64 bit\n"
        "\n"
        "    The endianness of the input or output format cannot be specified. This is\n"
        "    the system's native byte order."
        "\n"
        "    You must decode data with the a format that ensures that de decoded values\n"
        "    fit in the output data type or else he result is undefined. A guideline is\n"
        "    to use the same format for encoding and decoding.\n"
        "\n"
        "ORDER\n"
        "    The order is number of 0 or larger and specifies the minimum bits that is\n"
        "    used per value. Higher numbers may increase the efficiency when the values\n"
        "    are relative large at the expense of smaller values.\n"
        "\n"
        "    Data must be decoded with the same order as it is encoded or else the result\n"
        "    is undefined.\n"
        "\n"
        "    When the adaptive mode is enabled the order is used to initialize the\n"
        "    smoothing filter that is used to caculate the order in which the next value\n"
        "    is encoded.\n"
        "\n"
        "USAGE\n"
        "    The '-eu8' and '-k0' options are use as default when these options are not\n"
        "    provided.\n"
        "\n"
        "        golomb file1 file2\n"
        "\n"
        "    Encode signed 16 bit values from 'file1' and encode it with an order of ´4'.\n"
        "\n"
        "        golomb -ei16 -k4 file1 file2\n"
        "\n"
        "    Decode data from 'file1' and output the values as unsigned 32 bit to 'file2'.\n"
        "    The data from 'file1' is expected to be order '0'.\n"
        "\n"
        "        golomb -du32 -k0 file1 file2\n"
        "\n"
        "    Encode the output from another command as input, in this example 'cat'.\n"
        "\n"
        "        cat file1 | golomb -ei8 - file\n"
        "\n"
        "    Decode from from input 'file' and write the results the to standard output.\n"
        "\n"
        "        golomb -di8 file -\n";

    std::puts( help );
}

enum class data_type
{
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64
};

[[nodiscard]] static data_type decode_format_arg( char option, std::string_view fmt ) noexcept
{
    if( fmt.size() > 0 )
    {
        const auto signness = std::tolower( fmt.front() );

        fmt.remove_prefix( 1u );

        const auto begin = fmt.data();
        const auto end   = begin + fmt.size();

        int        width           = 8;
        const auto [ pos_ptr, ec ] = std::from_chars( begin, end, width );

        if( !( signness == 'i' || signness == 'u' ) ||
            ( pos_ptr == begin || pos_ptr != end ) ||
            !( width == 8 || width == 16 || width == 32 || width == 64 ) )
        {
            golomb_argument_error( "Invalid argument for option '%c'.", option );
        }

        switch( width )
        {
        case 8:
            return signness == 'i' ? data_type::int8 : data_type::uint8;
        case 16:
            return signness == 'i' ? data_type::int16 : data_type::uint16;
        case 32:
            return signness == 'i' ? data_type::int32 : data_type::uint32;
        case 64:
            return signness == 'i' ? data_type::int64 : data_type::uint64;
        }
    }

    return data_type::uint8;
}

[[nodiscard]] static int decode_adaptive_arg( std::string_view a ) noexcept
{
    auto       begin = a.data();
    const auto end   = begin + a.size();
    int        order = {};

    const auto [ pos_ptr, ec ] = std::from_chars( begin, end, order );
    if( pos_ptr == begin || pos_ptr != end || order < 0 )
    {
        golomb_argument_error( "Invalid argument for option 'a'." );
    }

    return order;
}

[[nodiscard]] static size_t decode_k_arg( std::string_view k ) noexcept
{
    auto       begin = k.data();
    const auto end   = begin + k.size();
    size_t     order = {};

    const auto [ pos_ptr, ec ] = std::from_chars( begin, end, order );
    if( pos_ptr == begin || pos_ptr != end )
    {
        golomb_argument_error( "Invalid argument for option 'k'." );
    }

    return order;
}

template< typename InputValueT, typename OutputDataT >
static void adaptive_encode( std::FILE * const in_file,
                             std::FILE * const out_file,
                             size_t k,
                             int adaptive )
{
    using UnsignedInputValueT = std::make_unsigned< InputValueT >::type;
    if( adaptive >= std::numeric_limits< UnsignedInputValueT >::digits )
    {
        golomb_argument_error( "Invalid argument for option 'a'." );
    }

    auto input = binary_input_file_iterator< InputValueT >( in_file );
    auto last  = binary_input_file_iterator< InputValueT >();

    using OutputItT = binary_output_file_iterator< OutputDataT >;
    auto output     = OutputItT( out_file );

    pg::golomb::encoder< OutputItT, OutputDataT > e( output );

    for( ; input != last ; ++input )
    {
        const auto unsigned_value = pg::golomb::to_unsigned( *input );

        e.push( unsigned_value, k );

        k = k - ( k >> adaptive ) + ( std::bit_width( unsigned_value ) >> adaptive );
    }

    e.flush();
}

template< typename InputValueT, typename OutputDataT >
static void encode( std::FILE * const in_file,
                    std::FILE * const out_file,
                    size_t k,
                    int adaptive )
{
    if( adaptive >= 0 )
    {
        adaptive_encode< InputValueT, OutputDataT >( in_file, out_file, k, adaptive );
    }
    else
    {
        pg::golomb::encode( binary_input_file< InputValueT >( in_file ),
                            binary_output_file_iterator< OutputDataT >( out_file ),
                            k );
    }
}

static void encode( std::FILE * const in_file,
                    std::FILE * const out_file,
                    data_type type,
                    size_t k,
                    int adaptive ) noexcept
{
    switch( type )
    {
    case data_type::int8:
        return encode< int8_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::uint8:
        return encode< uint8_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::int16:
        return encode< int16_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::uint16:
        return encode< uint16_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::int32:
        return encode< int32_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::uint32:
        return encode< uint32_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::int64:
        return encode< int64_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::uint64:
        return encode< uint64_t, uint8_t >( in_file, out_file, k, adaptive );
    }
}

template< typename InputDataT, typename OutputValueT >
static void adaptive_decode( std::FILE * const in_file,
                             std::FILE * const out_file,
                             size_t k,
                             int adaptive )
{
    using UnsignedOutputValueT = std::make_unsigned< OutputValueT >::type;

    if( adaptive >= std::numeric_limits< UnsignedOutputValueT >::digits )
    {
        golomb_argument_error( "Invalid argument for option 'a'." );
    }

    auto input     = binary_input_file_iterator< InputDataT >( in_file );
    auto input_end = binary_input_file_iterator< InputDataT >();
    auto output    = binary_output_file_iterator< OutputValueT >( out_file );

    pg::golomb::decoder d( input, input_end );

    while( d.has_data() )
    {
        const auto [ value, status ] = d.template pull< UnsignedOutputValueT >( k );
        if( status == pg::golomb::decoder_status::success )
        {
            k         = k - ( k >> adaptive ) + ( std::bit_width( value ) >> adaptive );
            *output++ = pg::golomb::to_integral< OutputValueT >( value );
        }
    }
}

template< typename InputDataT, typename OutputValueT >
static void decode( std::FILE * const in_file,
                    std::FILE * const out_file,
                    size_t k,
                    int adaptive )
{
    if( adaptive >= 0 )
    {
        adaptive_decode< InputDataT, OutputValueT >( in_file, out_file, k, adaptive );
    }
    else
    {
        pg::golomb::decode< OutputValueT >( binary_input_file< InputDataT >( in_file ),
                                            binary_output_file_iterator< OutputValueT >( out_file ),
                                            k );
    }
}

static void decode( std::FILE * const in_file,
                    std::FILE * const out_file,
                    data_type type,
                    size_t k,
                    int adaptive ) noexcept
{
    switch( type )
    {
    case data_type::int8:
        return decode< uint8_t, int8_t >( in_file, out_file, k, adaptive );

    case data_type::uint8:
        return decode< uint8_t, uint8_t >( in_file, out_file, k, adaptive );

    case data_type::int16:
        return decode< uint8_t, int16_t >( in_file, out_file, k, adaptive );

    case data_type::uint16:
        return decode< uint8_t, uint16_t >( in_file, out_file, k, adaptive );

    case data_type::int32:
        return decode< uint8_t, int32_t >( in_file, out_file, k, adaptive );

    case data_type::uint32:
        return decode< uint8_t, uint32_t >( in_file, out_file, k, adaptive );

    case data_type::int64:
        return decode< uint8_t, int64_t >( in_file, out_file, k, adaptive );

    case data_type::uint64:
        return decode< uint8_t, uint64_t >( in_file, out_file, k, adaptive );
    }
}

int main( const int argc, const char * argv[] )
{
    enum transformation : char { encode_ = 'e', decode_ = 'd' };

    transformation   direction = transformation::encode_;
    data_type        type      = data_type::uint8;
    size_t           k         = {};
    int              adaptive  = -1;
    std::string_view input;
    std::string_view output;

    {
        options opts( argc, argv );
        for( char opt = opts.read_option() ; opt != '\0' ; opt = opts.read_option() )
        {
            switch( opt )
            {
            case 'a':
                adaptive = decode_adaptive_arg( opts.read_argument() );
                break;

            case 'e':
                direction = transformation::encode_;
                type      = decode_format_arg( opt, opts.read_argument() );
                break;

            case 'd':
                direction = transformation::decode_;
                type      = decode_format_arg( opt, opts.read_argument() );
                break;

            case 'h':
                print_help();
                break;

            case 'k':
                k = decode_k_arg( opts.read_argument() );
                break;

            default:
                golomb_argument_error( "Unrecognized option '%c'.", opt );
            }
        }

        input  = opts.read_argument();
        output = opts.read_argument();
    }

    if( input.empty() )
    {
        golomb_argument_error( "No input input parameter provided." );
    }

    if( output.empty() )
    {
        golomb_argument_error( "No output input parameter provided." );
    }

    std::FILE * const in_file  = input == "-" ? stdin : std::fopen( std::string( input ).c_str(), "rb" );
    if( in_file == nullptr )
    {
        golomb_errno( "Input" );
    }

    std::FILE * const out_file = output == "-" ? stdout : std::fopen( std::string( output ).c_str(), "wb" );
    if( out_file == nullptr )
    {
        golomb_errno( "Output" );
    }

    if( direction == transformation::encode_ )
    {
        encode( in_file, out_file, type, k, adaptive );
    }
    else
    {
        decode( in_file, out_file, type, k, adaptive );
    }

    return 0;
}
