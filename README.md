# A C++ library to encode and decode values using Exponential Golomb Encoding

This library implements [Exponential Golomb Encoding](https://en.wikipedia.org/wiki/Exponential-Golomb_coding) that can efficiently encode data with lots of small integeral values.

The advantage of the Exponential Golomb Encoding over other compression methods is that it can encode data in a single pass and does not require any buffering of the input or output data.
These properties can make Exponential Golomb Encoding a good fit for applications that are tight on memory usage or require low latencies.

For better efficiency the data may need preprocessing before encoding to reduce the size of the values, for example with [Delta Encoding](https://en.wikipedia.org/wiki/Delta_encoding).

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
const uint8_t          values[ 8 ] = { 0u, 1u, 2u, 3u, 4u, 255u, 0u, 2u };
std::vector< uint8_t > data;

pg::golomb::encode( std::begin( values ), std::end( values ), std::back_inserter( data ) );

assert( data.size() == 4 );
```

### Decode

```c++
const uint8_t          data[ 5 ] = { 0xA6u, 0x42u, 0x80u, 0x40u, 0x2Cu };
std::vector< int16_t > values;

pg::golomb::decode< int16_t >( std::begin( data ), std::end( data ), std::back_inserter( values ) );

assert_same( values.size(), 8 );
```

## golomb utility

The golomb utility is an implementation for a commandline program that use this library.
The utility can be used to test the efficiency of the compression for your use case.

You can build the utility with the provided makefile by running the following make command;

```sh
make golomb
```

Information about the usage is displayed by running the executable without parameters or with the `-h` option.
You can also read the help text that is displayed by the executable from the [source file](https://github.com/PG1003/golomb/blob/main/util/golomb.cpp).

## Documentation

### API

This library has two functions that live in the `pg::glomb` namespace; `encode` and `decode`.  
Like the algorithms in the STL these functions use iterators to read and write values.
Iterators give you the freedom to use raw pointers, iterators from standard containers or your own fancy iterator.

#### `output_iterator pg::data::encode< OutputDataT = uint8_t >( input_iterator in, input_iterator last, output_iterator out, size_t k = 0 )`

Reads data from `in` until the iterator is equal to last. The encoded data is written to `out`.
The function returns an `output_iterator` that points to one past the last written data.

The data type returned by the `input_iterator` can be of any size but must be an signed or unsigned integral type, e.g. `unsigned char`, `uint32_t`, `long`, `int8_t`, etc.  
The `output_iterator` must handle _unsigned_ data. Default is `uint8_t` but can be overridden by the `OutputDataT` template parameter.

`out` must accommodate enough space to buffer the encoded data which may require more space than the input data.

`k` is the order of the algorithm that specifies the miminum number of bits that is used for a value.
A higher number may increase the efficiency when the values are relative large at the expense of smaller values.

#### `output_iterator pg::brle::decode< OutputValueT >( input_iterator in, input_iterator last, output_iterator out, size_t k = 0 )`

Reads data values from `in` until the iterator is equal to last. The decoded data is written to `out`.
The function returns an output_iterator that points to one past the last written decoded value.

The `input_iterator` must dereference to an _unsigned_ data type.
The underlaying value type of the `output_iterator` is specified by the `OutputValueT` template parameter.
This parameter must be a signed or an unsigned integer type.

Be sure that `out` can buffer all the data that is decoded from the input.

`k` is the order in which the input data is encoded and _must_ match the order used when encoding the values or else the decoded values are undefined.

### Endianess

The functions do not handle endianess in a specific way; this depends on the platform.  
Use `uint8_t` for encoded input and output data when cross-platform data exchange is required.
