#include "catch.hpp"

#include "http_parser.h"

TEST_CASE( "http_parser stub test", "[http_parser][unit]" ) {
    REQUIRE(1 == 1);
}

TEST_CASE( "http_parser simple setup", "[http_parser][unit]" ) {
    size_t parsed;
    http_parser parser = {0};
    http_parser_settings parser_settings = {0};

    http_parser_init(&parser, HTTP_REQUEST);
    parsed = http_parser_execute(&parser, &parser_settings, "", 0);

    REQUIRE(parsed == 0);
}

#define CRLF "\r\n"
const char SIMPLE_GET_REQUEST[] = "GET / HTTP/1.1" CRLF CRLF;

TEST_CASE( "http_parser parses simple GET request", "[http_parser][unit]" ) {
    size_t parsed;
    size_t len;
    http_parser parser = {0};
    http_parser_settings parser_settings = {0};

    len = strlen(SIMPLE_GET_REQUEST);

    http_parser_init(&parser, HTTP_REQUEST);
    parsed = http_parser_execute(&parser, &parser_settings, SIMPLE_GET_REQUEST, len);

    REQUIRE(parsed == len);

    REQUIRE(parser.type == HTTP_REQUEST);
    REQUIRE(parser.method == HTTP_GET);
}
