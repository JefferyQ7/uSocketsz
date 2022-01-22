#ifdef LIBUS_USE_QUIC

/* Experimental HTTP/3 server */
#include <libusockets.h>

/* Quic interface is not exposed under libusockets.h yet */
#include "quic.h"

#include <stdio.h>

/* Loop callbacks not used in this example */
void on_wakeup(struct us_loop_t *loop) {}
void on_pre(struct us_loop_t *loop) {}
void on_post(struct us_loop_t *loop) {}

/* No need to handle this one */
void on_server_quic_stream_open() {

}

/* This would be a request */
void on_server_quic_stream_headers() {
    printf("HTTP/3 request:\n");
}

/* And this would be the body of the request */
void on_server_quic_stream_data(us_quic_stream_t *s, char *data, int length) {
    printf("==========================\n%.*s\n", length, data);

    us_quic_stream_write(s, "Jaja hello!", 11);
}

void on_server_quic_stream_writable() {

}

void on_server_quic_stream_close() {

}

void on_server_quic_open() {

}

void on_server_quic_close() {

}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* SSL cert is always needed for quic */
    us_quic_socket_context_options_t options = {
        .cert_file_name = "",
        .key_file_name = ""
    };

    /* Create quic socket context (assumes h3 for now) */
    us_quic_socket_context_t *context = us_create_quic_socket_context(loop, options);

    /* Specify application callbacks */
    us_quic_socket_context_on_stream_data(context, on_server_quic_stream_data);
    us_quic_socket_context_on_stream_open(context, on_server_quic_stream_open);
    us_quic_socket_context_on_stream_close(context, on_server_quic_stream_close);
    us_quic_socket_context_on_stream_writable(context, on_server_quic_stream_writable);
    us_quic_socket_context_on_stream_headers(context, on_server_quic_stream_headers);
    us_quic_socket_context_on_open(context, on_server_quic_open);
    us_quic_socket_context_on_close(context, on_server_quic_close);

    /* The listening socket is the actual UDP socket used */
    us_quic_listen_socket_t *listen_socket = us_quic_socket_context_listen(context, "127.0.0.1", 9004);

    /* Run the event loop */
    us_loop_run(loop);
}
#else

#include <stdio.h>

int main() {
    printf("Compile with WITH_QUIC=1 make examples in order to build this example\n");
}

#endif