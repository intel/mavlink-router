/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2016  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <arpa/inet.h>
#include <asm/termbits.h>
#include <errno.h>
#include <inttypes.h>

#include <mavlink.h>

#include "macro.h"

#define MAX_TIMEOUT 5

/*
 * mavlink 2.0 packet in its wire format
 *
 * Packet size:
 *      sizeof(mavlink_router_mavlink2_header)
 *      + payload length
 *      + 2 (checksum)
 *      + signature (0 if not signed)
 */
struct _packed_ mavlink_router_mavlink2_header {
    uint8_t magic;
    uint8_t payload_len;
    uint8_t incompat_flags;
    uint8_t compat_flags;
    uint8_t seq;
    uint8_t sysid;
    uint8_t compid;
    uint32_t msgid : 24;
};

/*
 * mavlink 1.0 packet in its wire format
 *
 * Packet size:
 *      sizeof(mavlink_router_mavlink1_header)
 *      + payload length
 *      + 2 (checksum)
 */
struct _packed_ mavlink_router_mavlink1_header {
    uint8_t magic;
    uint8_t payload_len;
    uint8_t seq;
    uint8_t sysid;
    uint8_t compid;
    uint8_t msgid;
};

struct buffer {
    unsigned int len;
    uint8_t *data;
};

class Pollable {
public:
    int fd = -1;
    virtual ~Pollable();
};

class Timeout : public Pollable {
public:
    bool (*cb)(void *data);
    const void *data;
    bool remove_me = false;
};

class Endpoint : public Pollable {
public:
    Endpoint(const char *name, bool crc_check_enabled);
    virtual ~Endpoint();

    int read_msg(struct buffer *pbuf);
    void print_statistics();
    virtual int write_msg(const struct buffer *pbuf) = 0;
    int write_msg(const mavlink_message_t *msg);
    virtual int flush_pending_msgs() = 0;

    /*
     * Copy message in from_buffer buffer adding back the trimmed zeros to to_buffer.
     */
    void untrim_msg(const struct buffer *from_buffer, struct buffer *to_buffer);

    struct buffer rx_buf;
    struct buffer tx_buf;

protected:
    virtual ssize_t _read_msg(uint8_t *buf, size_t len) = 0;
    bool _check_crc();

    const char *_name;
    size_t _last_packet_len = 0;

    uint32_t _read_crc_errors = 0;
    uint32_t _read_total = 0;
    uint32_t _write_total = 0;
    const bool _crc_check_enabled;
};

class UartEndpoint : public Endpoint {
public:
    UartEndpoint() : Endpoint{"UART", true} { }
    virtual ~UartEndpoint() { }
    int write_msg(const struct buffer *pbuf) override;
    int flush_pending_msgs() override { return -ENOSYS; }

    int open(const char *path, speed_t baudrate);
protected:
    ssize_t _read_msg(uint8_t *buf, size_t len) override;
};

class UdpEndpoint : public Endpoint {
public:
    UdpEndpoint();
    virtual ~UdpEndpoint() { }

    int write_msg(const struct buffer *pbuf) override;
    int flush_pending_msgs() override { return -ENOSYS; }

    int open(const char *ip, unsigned long port, bool bind = false);

    struct sockaddr_in sockaddr;

protected:
    ssize_t _read_msg(uint8_t *buf, size_t len) override;
};

class TcpEndpoint : public Endpoint {
public:
    TcpEndpoint();
    virtual ~TcpEndpoint();

    int accept(int listener_fd);

    int write_msg(const struct buffer *pbuf) override;
    int flush_pending_msgs() override { return -ENOSYS; }

    struct sockaddr_in sockaddr;

protected:
    ssize_t _read_msg(uint8_t *buf, size_t len) override;
};

class Mainloop {
public:
    int open();
    int add_fd(int fd, void *data, int events);
    int mod_fd(int fd, void *data, int events);
    int remove_fd(int fd);
    void loop();
    void handle_read(Endpoint *e);
    void handle_canwrite(Endpoint *e);
    void handle_tcp_connection();
    int write_msg(Endpoint *e, const struct buffer *buf);
    void process_tcp_hangups();
    Timeout *timeout_add(uint32_t timeout_ms, bool (*cb)(void *data), const void *data);
    void timeout_del(Timeout *t);

    int epollfd = -1;
    bool should_process_tcp_hangups = false;

private:
    Timeout *_timeout_list[MAX_TIMEOUT];
    void _timeout_process_del(bool del_all);
};
