set(FOXY_TESTING ON)
# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_USE_DEBUG_LIBS ON)
# set(Boost_USE_RELEASE_LIBS OFF)
# set(Boost_USE_DEBUG_RUNTIME ON)
# set(BOOST_ROOT "/Users/cmaza/cpp/boost-169/install")

set(Boost_DIR "/Users/cmaza/cpp/boost-170/lib/cmake/Boost-1.70.0")

add_compile_options("/permissive-")
add_link_options("/NODEFAULTLIB:library" "/debug:fastlink")
add_compile_definitions("BOOST_ASIO_NO_DEPRECATED")
include("/Users/cmaza/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake")
