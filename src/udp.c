/*
 * Authored by Alex Hultman, 2018-2021.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libusockets.h"
#include "internal/internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WIN32_EXPORT int us_udp_packet_buffer_ecn(struct us_udp_packet_buffer_t *buf, int index) {
    return 0;
}

WIN32_EXPORT char *us_udp_packet_buffer_peer(struct us_udp_packet_buffer_t *buf, int index) {
    return bsd_udp_packet_buffer_peer(buf, index);
}

WIN32_EXPORT char *us_udp_packet_buffer_payload(struct us_udp_packet_buffer_t *buf, int index) {
    return bsd_udp_packet_buffer_payload(buf, index);
}

WIN32_EXPORT int us_udp_packet_buffer_payload_length(struct us_udp_packet_buffer_t *buf, int index) {
    return bsd_udp_packet_buffer_payload_length(buf, index);
}

// what should we return? number of sent datagrams?
WIN32_EXPORT int us_udp_socket_send(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf, int num) {
    int fd = us_poll_fd((struct us_poll_t *) s);

    // we need to poll out if we failed

    return bsd_sendmmsg(fd, buf, num, 0);
}

WIN32_EXPORT int us_udp_socket_receive(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf) {
    int fd = us_poll_fd((struct us_poll_t *) s);
    return bsd_recvmmsg(fd, buf, LIBUS_UDP_MAX_NUM, 0, 0);
}

WIN32_EXPORT void us_udp_buffer_set_packet_payload(struct us_udp_packet_buffer_t *send_buf, int index, int offset, void *payload, int length, void *peer_addr) {
    bsd_udp_buffer_set_packet_payload(send_buf, index, offset, payload, length, peer_addr);
}

WIN32_EXPORT struct us_udp_packet_buffer_t *us_create_udp_packet_buffer() {
    return (struct us_udp_packet_buffer_t *) bsd_create_udp_packet_buffer();
}

struct us_internal_udp_t {
    struct us_internal_callback_t cb;
    struct us_udp_packet_buffer_t *receive_buf;
    void (*data_cb)(struct us_udp_socket_t *, struct us_udp_packet_buffer_t *, int);
    void (*drain_cb)(struct us_udp_socket_t *);
    void *user;
};

/* Internal wrapper, move from here */
void internal_on_udp_read(struct us_udp_socket_t *s) {

    // lookup receive buffer and callback here
    struct us_internal_udp_t *udp = (struct us_internal_udp_t *) s;

    int packets = us_udp_socket_receive(s, udp->receive_buf);
    //printf("Packets: %d\n", packets);

    // we need to get the socket data and lookup its callback here


    udp->data_cb(s, udp->receive_buf, packets);
}

void *us_udp_socket_user(struct us_udp_socket_t *s) {
    struct us_internal_udp_t *udp = (struct us_internal_udp_t *) s;

    return udp->user;
}

typedef struct {
    void *data;
    size_t length;
} us_iov_t;

typedef struct {
    size_t iovlen;
    us_iov_t *iov;
} us_udp_datagram_t;

int us_udp_socket_send_datagrams(struct us_udp_socket_t *s, void *name, int name_length, us_udp_datagram_t d) {
    
}

WIN32_EXPORT struct us_udp_socket_t *us_create_udp_socket(struct us_loop_t *loop, struct us_udp_packet_buffer_t *buf, void (*data_cb)(struct us_udp_socket_t *, struct us_udp_packet_buffer_t *, int), void (*drain_cb)(struct us_udp_socket_t *), char *host, unsigned short port, void *user) {
    
    LIBUS_SOCKET_DESCRIPTOR fd = bsd_create_udp_socket(host, port);
    if (fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    /* If buf is 0 then create one here */
    if (!buf) {
        buf = us_create_udp_packet_buffer();
    }

    int ext_size = 0;
    int fallthrough = 0;

    struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_udp_t) + ext_size);
    us_poll_init(p, fd, POLL_TYPE_CALLBACK);

    struct us_internal_udp_t *cb = (struct us_internal_udp_t *) p;
    cb->cb.loop = loop;
    cb->cb.cb_expects_the_loop = 0;
    cb->cb.leave_poll_ready = 1;

    /* There is no udp socket context, only user data */
    /* This should really be ext like everything else */
    cb->user = user;

    cb->data_cb = data_cb;
    cb->receive_buf = buf;
    cb->drain_cb = drain_cb;

    cb->cb.cb = (void (*)(struct us_internal_callback_t *)) internal_on_udp_read;

    us_poll_start((struct us_poll_t *) cb, cb->cb.loop, LIBUS_SOCKET_READABLE);
    
    return (struct us_udp_socket_t *) cb;
}