#include "catch.hpp"

extern "C" {
#include "haywire.h"

#include "http_parser.h"
#include "http_request.h"
#include "http_connection.h"

#include "hw_string.h"
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
    uv_run(loop, UV_RUN_DEFAULT);
}


TEST_CASE("can run uv loop", "[uv][unit]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();

    uv_thread_create(&thread, thread_main, loop);

    uv_thread_join(&thread);
}



struct check_thread_data {
    uv_loop_t * loop;
    uv_check_t * check;
    int * sentinel;
};

void set_sentinel(uv_check_t * handle) {
    check_thread_data * p = (check_thread_data *)(handle->data);

    *(p->sentinel) = 1;

    uv_unref((uv_handle_t *)handle);
}


static void check_thread_main( void * user ) {
    check_thread_data * p = (check_thread_data *)user;

    p->loop = uv_loop_new();

    uv_check_init(p->loop, p->check);
    p->check->data = p;
    REQUIRE(1 == 1);

    uv_check_start(p->check, set_sentinel);

    uv_run(p->loop, UV_RUN_NOWAIT);

    uv_close((uv_handle_t *)(p->check), NULL);

    uv_run(p->loop, UV_RUN_NOWAIT);

    uv_loop_delete(p->loop);
}


int sentinel = 0;

TEST_CASE("can attach event to loop", "[uv]") {
    uv_thread_t thread;
    uv_check_t check;
    check_thread_data cd = {0};

    cd.check = &check;
    cd.sentinel = &sentinel;

    REQUIRE(sentinel == 0);

    uv_thread_create(&thread, check_thread_main, &cd);

    uv_thread_join(&thread);

    REQUIRE(sentinel == 1);
}

struct async_test_data {
    uv_loop_t * loop;
    int * p;
    unsigned long tWork;
    unsigned long tUV;
};

static void async_thread_main( void * user ) {
    async_test_data * td = (async_test_data *)user;
    uv_loop_t * loop = td->loop;

    td->tUV = uv_thread_self();

    REQUIRE(1 == 1);
    uv_run(loop, UV_RUN_DEFAULT);
}



void work_set(uv_work_t * handle) {
    async_test_data * d = (async_test_data *)(handle->data);

    d->tWork = uv_thread_self();
    *(d->p) = 1;
}

void after_set(uv_work_t * handle, int status) {
    async_test_data * d = (async_test_data *)(handle->data);
    
    /* work should be completed */
    REQUIRE(*(d->p) == 1);

    /* should be on uv thread */
    REQUIRE(d->tUV == uv_thread_self());
    REQUIRE(d->tWork != uv_thread_self());
}

TEST_CASE("can queue async work event on loop", "[uv]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();

    uv_work_t work;
    int sentinel = 0;
    async_test_data td = {0};

    td.loop = loop;
    td.p = &sentinel;

    work.data = &td;

    REQUIRE(sentinel == 0);
    uv_queue_work(loop, &work, work_set, after_set);
    REQUIRE(sentinel == 0);

    uv_thread_create(&thread, async_thread_main, &td);

    uv_thread_join(&thread);

    REQUIRE(sentinel == 1);

    REQUIRE(td.tWork != td.tUV);
}

struct http_parser_work {
    http_parser parser;
    hw_string body;
    http_parser_settings * parser_settings;
    http_connection * connection;
    http_request * request;
    size_t parsed;
};

void http_parser_worker(uv_work_t * handle) {
    http_parser_work * pwork = (http_parser_work *)(handle->data);

    http_parser_init(&(pwork->parser), HTTP_REQUEST);
    pwork->parsed = http_parser_execute(&(pwork->parser), pwork->parser_settings, 
                                        pwork->body.value, pwork->body.length);

    REQUIRE(pwork->parsed == pwork->body.length);

    REQUIRE(pwork->parser.type == HTTP_REQUEST);
    REQUIRE(pwork->parser.method == HTTP_GET);

    REQUIRE(pwork->request->url == std::string("/"));
}

void http_parser_after(uv_work_t * handle, int status) {
    http_parser_work * pwork = (http_parser_work *)(handle->data);

    REQUIRE(pwork->request->url == std::string("/"));
    REQUIRE(1 == 1);

}


TEST_CASE("can queue async http parse on loop", "[uv][http_parser]") {
    uv_thread_t thread;
    uv_loop_t * loop = uv_loop_new();

    uv_work_t work;
    http_parser_work hpw = {0};

    http_parser_settings parser_settings = {0};
    http_connection connection = {0};
    http_request request = {0};

    hpw.parser_settings = &parser_settings;
    hpw.connection = &connection;
    hpw.request = &request;

    parser_settings.on_url = http_request_on_url;

    connection.request = &request;
    hpw.parser.data = &connection;

    hpw.body = *(create_string(SIMPLE_GET_REQUEST));

    work.data = &hpw;

    uv_queue_work(loop, &work, http_parser_worker, http_parser_after);

    uv_thread_create(&thread, thread_main, loop);

    uv_thread_join(&thread);

    REQUIRE(request.url == std::string("/"));
    free(request.url);
    free_string(&(hpw.body));

}
