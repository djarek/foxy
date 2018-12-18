//
// Copyright (c) 2018-2018 Christian Mazakas (christian dot mazakas at gmail dot
// com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/LeonineKing1199/foxy
//

#include <foxy/uri_parts.hpp>
#include <foxy/uri.hpp>

namespace x3 = boost::spirit::x3;

auto
foxy::uri_parts::scheme() const -> string_view
{
  return string_view(scheme_.begin(), scheme_.end() - scheme_.begin());
}

auto
foxy::uri_parts::host() const -> string_view
{
  return string_view(host_.begin(), host_.end() - host_.begin());
}

auto
foxy::uri_parts::port() const -> string_view
{
  return string_view(port_.begin(), port_.end() - port_.begin());
}

auto
foxy::uri_parts::path() const -> string_view
{
  return string_view(path_.begin(), path_.end() - path_.begin());
}

auto
foxy::uri_parts::query() const -> string_view
{
  return string_view(query_.begin(), query_.end() - query_.begin());
}

auto
foxy::uri_parts::fragment() const -> string_view
{
  return string_view(fragment_.begin(), fragment_.end() - fragment_.begin());
}

auto
foxy::make_uri_parts(uri_parts::string_view const uri_view) -> uri_parts
{
  auto       iter  = uri_view.begin();
  auto const end   = uri_view.end();
  auto       parts = foxy::uri_parts();

  auto const path_absolute =
    x3::raw[x3::eps] >> x3::raw[x3::eps] >> x3::raw[foxy::uri::path_absolute()];

  auto const hier_part =
    (x3::lit("//") >> -(foxy::uri::userinfo() >> "@") >>
     x3::raw[foxy::uri::host()] >> -(":" >> x3::raw[foxy::uri::port()]) >>
     x3::raw[foxy::uri::path_abempty()]);

  auto old   = iter;
  auto match = false;

  match =
    x3::parse(iter, end, x3::raw[foxy::uri::scheme()] >> ":", parts.scheme_);
  if (!match) { goto upcall; }

  old = iter;

  match = x3::parse(iter, end,
                    x3::lit("//") >> -(foxy::uri::userinfo() >> "@") >>
                      x3::raw[foxy::uri::host()],
                    parts.host_);

  if (match) {
    match =
      x3::parse(iter, end, -(":" >> x3::raw[foxy::uri::port()]), parts.port_);
    if (!match) { goto upcall; }

    match =
      x3::parse(iter, end, x3::raw[foxy::uri::path_abempty()], parts.path_);
    if (!match) { goto upcall; }
  }

  if (!match) {
    iter = old;
    x3::parse(iter, end, x3::raw[foxy::uri::path_absolute()], parts.path_);
  }

  if (!match) {
    iter = old;
    match =
      x3::parse(iter, end, x3::raw[foxy::uri::path_rootless()], parts.path_);
  }

  if (!match) {
    iter  = old;
    match = x3::parse(iter, end, x3::raw[foxy::uri::path_empty()], parts.path_);
  }

  if (!match) { goto upcall; }

  match =
    x3::parse(iter, end, -("?" >> x3::raw[foxy::uri::query()]), parts.query_);
  if (!match) { goto upcall; }

  match = x3::parse(iter, end, -("#" >> x3::raw[foxy::uri::fragment()]),
                    parts.fragment_);

  if (!match) { goto upcall; }

upcall:
  return parts;
}