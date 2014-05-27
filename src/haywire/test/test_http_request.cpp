#include "catch.hpp"

extern "C" {
#include "haywire.h"

#include "http_parser.h"
#include "http_request.h"
#include "http_connection.h"
}

#include <string>

namespace {

#define CRLF "\r\n"
const char SIMPLE_GET_REQUEST[] = "GET / HTTP/1.1" CRLF CRLF;

TEST_CASE( "http_parser returns URL on simple GET request", "[http_parser][unit]" ) {
    size_t parsed;
    size_t len;
    http_parser parser = {0};
    http_parser_settings parser_settings = {0};
    http_connection connection = {0};
    http_request request = {0};

    connection.request = &request;
    parser.data = &connection;

    parser_settings.on_url = http_request_on_url;

    len = strlen(SIMPLE_GET_REQUEST);

    http_parser_init(&parser, HTTP_REQUEST);
    parsed = http_parser_execute(&parser, &parser_settings, SIMPLE_GET_REQUEST, len);

    REQUIRE(parsed == len);

    REQUIRE(parser.type == HTTP_REQUEST);
    REQUIRE(parser.method == HTTP_GET);

    REQUIRE(request.url == std::string("/"));
    free(request.url);
}

static void thread_main( void * user ) {
    uv_loop_t * loop = uv_loop_new();
    REQUIRE(1 == 1);
    uv_run(loop, UV_RUN_DEFAULT);
}


TEST_CASE("can run uv loop", "[uv][unit]") {
    uv_thread_t thread1;

    uv_thread_create(&thread1, thread_main, 0);

    uv_thread_join(&thread1);
}

} /* end anonymous namespace */
