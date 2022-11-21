# A C++ library to compress integer values using Exponential Golomb Encoding

This library implements [Exponential Golomb Encoding](https://en.wikipedia.org/wiki/Exponential-Golomb_coding) that can efficiently pack data with lots of small integeral values.

The advantage of the Exponential Golomb Encoding over other compression methods is that it can encode data in a single pass and does not require any buffering of the input or output data.
These properties can make Exponential Golomb Encoding a good fit for applications that are tight on memory usage or require low latencies.

The data may need preprocessing before encoding to reduce the size of the values for better efficiency, for example with [Delta Encoding](https://en.wikipedia.org/wiki/Delta_encoding).

## Features

* Optimized for data with lots of small integeral values.
* Compress and expand data in a single pass.
* No buffering of input data or output data.

## Requirements

A C++20 compliant compiler.

## Goals

* Use standard C++.
* Minimalistic.
* Flexibile in usage.

## Non-goals

* Optimazations for specific compilers and targets.
* Defining a container format.  
  This library focuses only on converting data to and from Exponential Golomb Encoding.
  You have to take care for the information such as the original data size, length of the encoded stream, checksums, etc.

## Examples

Besides the following examples you can take a peek in the sources of the [test program](https://github.com/PG1003/golomb/blob/main/tests/test.cpp) and the [golomb utility](https://github.com/PG1003/golomb/blob/main/util/golomb.cpp) about the usage of the `golomb` library.

### Encode

```c++
const uint8_t values[ 8 ] = { 0u, 1u, 2u, 3u, 4u, 255u, 0u, 2u };

// Encode using a range as input
std::vector< uint8_t > out_range;

pg::golomb::encode( values, std::back_inserter( out_range ) );

assert( out_range.size() == 5 );

// Encode using iterators as input
std::vector< uint8_t > out_iter;

pg::golomb::encode( std::begin( values ), std::end( values ), std::back_inserter( out_iter ) );

assert( out_iter.size() == 5 );
```

### Decode

```c++
const uint8_t data[ 5 ] = { 0xA6u, 0x42u, 0x80u, 0x40u, 0x2Cu };

// Decode using a ranges as input
std::vector< int16_t > values_range;

pg::golomb::decode< int16_t >( data, std::back_inserter( values_range ) );

assert( values_range.size() == 8 );

// Decode using iterators as input
std::vector< int32_t > values_iter;

pg::golomb::decode< int32_t >( std::begin( data ), std::end( data ), std::back_inserter( values_iter ) );

assert( values_iter.size() == 8 );
```

## Endianess

The golomb library do not handle endianess in a specific way; this depends on the platform.  
Use `uint8_t` for golomb encoded input and output data when cross-platform data exchange is required.

## golomb utility

The golomb utility is an implementation for a commandline program that use this library.
The utility can be used to test the efficiency of the compression for your use case.

You can build the utility with the provided makefile by running the following make command;

```sh
make golomb
```

Information about the usage is displayed by running the executable with the `-h` option.
You can also read the help text that is displayed by the executable from the [source file](https://github.com/PG1003/golomb/blob/main/util/golomb.cpp).
