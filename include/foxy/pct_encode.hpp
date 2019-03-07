//
// Copyright (c) 2018-2019 Christian Mazakas (christian dot mazakas at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#ifndef FOXY_PCT_ENCODE_HPP_
#define FOXY_PCT_ENCODE_HPP_

#include <boost/locale/utf.hpp>
#include <cstdint>

namespace foxy
{
namespace uri
{
//
// to_utf8_encoding takes the Boost.Locale type `utf::code_point` which is capable of holding any
// Unicode code point. We convert the number to an unsigned 32 bit integer which represents the 1-4
// byte UTF-8 binary encoding scheme seen here:
// https://en.wikipedia.org/wiki/UTF-8
//
// UTF-8 encoding writes the representation of the code point's value to the binary templates
// described below:
//
// 7 bits =>  [U+0000,  U+007F]   => 0xxxxxxx
// 11 bits => [U+0080,  U+07FF]   => 110xxxxx 10xxxxxx
// 16 bits => [U+0800,  U+FFFF]   => 1110xxxx 10xxxxxx 10xxxxxx
// 21 bits => [U+10000, U+10FFFF] => 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
auto
to_utf8_encoding(boost::locale::utf::code_point const code_point) -> std::uint32_t;

} // namespace uri
} // namespace foxy

#endif // FOXY_PCT_ENCODE_HPP_
