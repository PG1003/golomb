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

    binary_input_file_iterator( std::FILE * file = nullptr )
        : file( file )
    {
        if( file )
        {
            next();
        }
    }

    bool operator==( const binary_input_file_iterator & other ) const
    {
        if( file == nullptr || other.file == nullptr )
        {
            return file == other.file;
        }

        return file == other.file && std::ftell( file ) == std::ftell( other.file );
    }

    bool operator!=( const binary_input_file_iterator & other ) const
    {
        return !operator==( other );
    }

    T &                          operator*()        { return value; }
    const T &                    operator*()  const { return value; }
    binary_input_file_iterator * operator->() const { return &value; }
    binary_input_file_iterator & operator++()       { next(); return *this; }
    binary_input_file_iterator   operator++( int )  { auto it = *this; next(); return it; }

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
struct binary_output_file_iterator
{
    using difference_type   = std::ptrdiff_t;
    using value_type        = void;
    using pointer           = void;
    using reference         = void;
    using iterator_category = std::output_iterator_tag;

    binary_output_file_iterator( std::FILE * file = nullptr )
        : file( file )
    {}

    T operator=( T value )
    {
        assert( file );

        if( std::fwrite( &value, sizeof( T ), 1, file ) != 1 )
        {

        }

        return value;
    }

    bool operator==( const binary_output_file_iterator & other ) const
    {
        if( file == nullptr || other.file == nullptr )
        {
            return file == other.file;
        }

        return file == other.file && std::ftell( file ) == std::ftell( other.file );
    }

    bool operator!=( const binary_output_file_iterator & other ) const
    {
        return !operator==( other );
    }

    binary_output_file_iterator & operator*()       { return *this; }
    binary_output_file_iterator & operator++()      { return *this; }
    binary_output_file_iterator   operator++( int ) { return *this; }

private:
    std::FILE * const file = nullptr;
};


static void print_help()
{
    const char * const help =
        "golomb v1.0.0\n"
        "\n"
        "A tool to compress or expand binary data using Exponential Golomb Encoding.\n"
        "\n"
        "SYNOPSIS\n"
        "    golomb -[ed] -[k] [-h] input output\n"
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
        "    -e[FORMAT]  Encode and specifies the input format, default format is 'u8'.\n"
        "    -d[FORMAT]  Decode and specifies the output format, default format is 'u8'.\n"
        "    -h          Shows this help.\n"
        "    -kN         Order 'N', must be a positive number. Default is '0'.\n"
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

static data_type decode_format_arg( char option, std::string_view fmt )
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
            !( width == 8 || width == 16 || width == 32 || width == 63 ) )
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

static size_t decode_k_arg( std::string_view k )
{

    const auto begin = k.data();
    const auto end   = begin + k.size();

    int        order           = 0;
    const auto [ pos_ptr, ec ] = std::from_chars( begin, end, order );

    if( ( pos_ptr == begin || pos_ptr != end ) ||
        ( order < 0 ) )
    {
        golomb_argument_error( "Invalid argument for option 'k'." );
    }

    return static_cast< size_t >( order );
}

static void encode( std::FILE * const in, std::FILE * const out, data_type type, size_t k )
{
    switch( type )
    {
    case data_type::int8:
        pg::golomb::encode( binary_input_file_iterator< int8_t >( in ),
                            binary_input_file_iterator< int8_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::uint8:
        pg::golomb::encode( binary_input_file_iterator< uint8_t >( in ),
                            binary_input_file_iterator< uint8_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::int16:
        pg::golomb::encode( binary_input_file_iterator< int16_t >( in ),
                            binary_input_file_iterator< int16_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::uint16:
        pg::golomb::encode( binary_input_file_iterator< uint16_t >( in ),
                            binary_input_file_iterator< uint16_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::int32:
        pg::golomb::encode( binary_input_file_iterator< int32_t >( in ),
                            binary_input_file_iterator< int32_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::uint32:
        pg::golomb::encode( binary_input_file_iterator< uint32_t >( in ),
                            binary_input_file_iterator< uint32_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::int64:
        pg::golomb::encode( binary_input_file_iterator< int64_t >( in ),
                            binary_input_file_iterator< int64_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;

    case data_type::uint64:
        pg::golomb::encode( binary_input_file_iterator< uint64_t >( in ),
                            binary_input_file_iterator< uint64_t >(),
                            binary_output_file_iterator< uint8_t >( out ),
                            k );
        break;
    }
}

static void decode( std::FILE * const in, std::FILE * const out, data_type type, size_t k )
{
    switch( type )
    {
    case data_type::int8:
        pg::golomb::decode< int8_t >( binary_input_file_iterator< uint8_t >( in ),
                                      binary_input_file_iterator< uint8_t >(),
                                      binary_output_file_iterator< int8_t >( out ),
                                      k );
        break;

    case data_type::uint8:
        pg::golomb::decode< uint8_t >( binary_input_file_iterator< uint8_t >( in ),
                                       binary_input_file_iterator< uint8_t >(),
                                       binary_output_file_iterator< uint8_t >( out ),
                                       k );
        break;

    case data_type::int16:
        pg::golomb::decode< int16_t >( binary_input_file_iterator< uint8_t >( in ),
                                       binary_input_file_iterator< uint8_t >(),
                                       binary_output_file_iterator< int16_t >( out ),
                                       k );
        break;

    case data_type::uint16:
        pg::golomb::decode< uint16_t >( binary_input_file_iterator< uint8_t >( in ),
                                        binary_input_file_iterator< uint8_t >(),
                                        binary_output_file_iterator< uint16_t >( out ),
                                        k );
        break;

    case data_type::int32:
        pg::golomb::decode< int32_t >( binary_input_file_iterator< uint8_t >( in ),
                                       binary_input_file_iterator< uint8_t >(),
                                       binary_output_file_iterator< int32_t >( out ),
                                       k );
        break;

    case data_type::uint32:
        pg::golomb::decode< uint32_t >( binary_input_file_iterator< uint8_t >( in ),
                                        binary_input_file_iterator< uint8_t >(),
                                        binary_output_file_iterator< uint32_t >( out ),
                                        k );
        break;

    case data_type::int64:
        pg::golomb::decode< int64_t >( binary_input_file_iterator< uint8_t >( in ),
                                       binary_input_file_iterator< uint8_t >(),
                                       binary_output_file_iterator< int64_t >( out ),
                                       k );
        break;

    case data_type::uint64:
        pg::golomb::decode< uint64_t >( binary_input_file_iterator< uint8_t >( in ),
                                        binary_input_file_iterator< uint8_t >(),
                                        binary_output_file_iterator< uint64_t >( out ),
                                        k );
        break;
    }
}

int main( const int argc, const char * argv[] )
{
    enum transformation : char { encode_ = 'e', decode_ = 'd' };

    transformation   direction = transformation::encode_;
    data_type        type      = data_type::uint8;
    size_t           k         = 0;
    std::string_view input;
    std::string_view output;

    {
        options opts( argc, argv );
        for( char opt = opts.read_option() ; opt != '\0' ; opt = opts.read_option() )
        {
            switch( opt )
            {
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
        encode( in_file, out_file, type, k );
    }
    else
    {
        decode( in_file, out_file, type, k );
    }

    return 0;
}
