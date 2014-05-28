#include "catch.hpp"

extern "C" {
#include "haywire.h"

#include "http_parser.h"
#include "http_request.h"
#include "http_connection.h"
}

#include <string>

#define CRLF "\r\n"
static const char SIMPLE_GET_REQUEST[] = "GET / HTTP/1.1" CRLF CRLF;

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
    uv_loop_t * loop = (uv_loop_t *)user;
    REQUIRE(1 == 1);
    uv_run(loop, UV_RUN_NOWAIT);
}


TEST_CASE("can run uv loop", "[uv][unit]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();

    uv_thread_create(&thread, thread_main, loop);

    uv_thread_join(&thread);
}

void set_sentinel(uv_check_t * handle) {
    int * p = (int*)(handle->data);
    *p = 1;
    uv_check_stop(handle);
}

TEST_CASE("can attach event to loop", "[uv]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();
    uv_check_t check;
    int sentinel = 0;

    uv_check_init(loop, &check);
    check.data = &sentinel;
    uv_check_start(&check, set_sentinel);

    REQUIRE(sentinel == 0);

    uv_thread_create(&thread, thread_main, loop);

    uv_thread_join(&thread);

    REQUIRE(sentinel == 1);
}

struct test_data {
    int * p;
    unsigned long tWork;
    unsigned long tUV;
};

void work_set(uv_work_t * handle) {
    test_data * d = (test_data *)(handle->data);

    d->tWork = uv_thread_self();
    *(d->p) = 1;
}

void after_set(uv_work_t * handle, int status) {
    test_data * d = (test_data *)(handle->data);
    /* should be on uv thread */
    d->tUV = uv_thread_self();
    REQUIRE(d->tWork != uv_thread_self());
}

TEST_CASE("can queue async work event on loop", "[uv]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();

    uv_work_t work;
    int sentinel = 0;
    test_data td = {0};

    td.p = &sentinel;

    work.data = &td;
    uv_queue_work(loop, &work, work_set, after_set);

    REQUIRE(sentinel == 0);

    uv_thread_create(&thread, thread_main, loop);

    uv_thread_join(&thread);

    REQUIRE(sentinel == 1);

    REQUIRE(td.tUV == (unsigned long)thread);
    REQUIRE(td.tWork != td.tUV);
}
