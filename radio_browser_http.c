/*
 * Small fixed-buffer HTTP GET helper for Radio Browser.
 *
 * Test harness build:
 *   gcc -std=gnu89 -Wall -Wextra -DRB_HTTP_TEST radio_browser_http.c radio_browser_url.c radio_browser_json.c -o /tmp/rb_http_test
 */

#include "radio_browser_http.h"

#include <stdio.h>
#include <string.h>

#if defined(AMIGA_M68K)
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef RB_HTTP_EXTERNAL_SOCKETBASE
/* Weak so this module can link either standalone or beside radio_stream.c,
 * which also provides SocketBase for bsdsocket.library builds. */
struct Library *SocketBase __attribute__((weak));
#endif
#define RB_HTTP_SOCKET long
#define RB_HTTP_INVALID_SOCKET (-1)
#define rb_http_close_socket(s) CloseSocket(s)
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#define RB_HTTP_SOCKET int
#define RB_HTTP_INVALID_SOCKET (-1)
#define rb_http_close_socket(s) close(s)
#endif

#define RB_HTTP_PORT 80
#define RB_HTTP_READ_CHUNK 512
#define RB_HTTP_MAX_REQUEST 512

typedef struct RbHttpTransport {
    RB_HTTP_SOCKET sock;
} RbHttpTransport;

static int rb_http_append(char *out, int out_size, int *pos, const char *text)
{
    if (!out || !pos || out_size <= 0 || !text) return RB_HTTP_ERR_BAD_ARG;
    while (*text) {
        if (*pos >= out_size - 1) return RB_HTTP_ERR_BODY_TOO_BIG;
        out[*pos] = *text;
        (*pos)++;
        out[*pos] = '\0';
        text++;
    }
    return 0;
}

static int rb_http_build_request(char *out, int out_size,
                                 const char *host, const char *path)
{
    int pos;
    int rc;

    if (!out || out_size <= 0 || !host || !*host || !path || !*path)
        return RB_HTTP_ERR_BAD_ARG;
    out[0] = '\0';
    pos = 0;
    rc = rb_http_append(out, out_size, &pos, "GET ");
    if (rc < 0) return rc;
    rc = rb_http_append(out, out_size, &pos, path);
    if (rc < 0) return rc;
    rc = rb_http_append(out, out_size, &pos, " HTTP/1.1\r\nHost: ");
    if (rc < 0) return rc;
    rc = rb_http_append(out, out_size, &pos, host);
    if (rc < 0) return rc;
    return rb_http_append(out, out_size, &pos,
        "\r\nUser-Agent: BoingPlayer/0.1 AmigaOS\r\n"
        "Accept: application/json\r\n"
        "Connection: close\r\n\r\n");
}

static int rb_http_transport_open(RbHttpTransport *transport, const char *host, int port)
{
    struct hostent *he;
    struct sockaddr_in sa;

    if (!transport || !host) return RB_HTTP_ERR_BAD_ARG;
    transport->sock = RB_HTTP_INVALID_SOCKET;

#if defined(AMIGA_M68K) && !defined(RB_HTTP_EXTERNAL_SOCKETBASE)
    if (!SocketBase) {
        SocketBase = OpenLibrary("bsdsocket.library", 4);
        if (!SocketBase) return RB_HTTP_ERR_CONNECT;
    }
#endif

    he = gethostbyname(host);
    if (!he || !he->h_addr_list || !he->h_addr_list[0]) return RB_HTTP_ERR_DNS;

    transport->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (transport->sock == RB_HTTP_INVALID_SOCKET) return RB_HTTP_ERR_CONNECT;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    memcpy(&sa.sin_addr, he->h_addr_list[0], (size_t)he->h_length);

    if (connect(transport->sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        rb_http_close_socket(transport->sock);
        transport->sock = RB_HTTP_INVALID_SOCKET;
        return RB_HTTP_ERR_CONNECT;
    }
    return 0;
}

static void rb_http_transport_close(RbHttpTransport *transport)
{
    if (transport && transport->sock != RB_HTTP_INVALID_SOCKET) {
        rb_http_close_socket(transport->sock);
        transport->sock = RB_HTTP_INVALID_SOCKET;
    }
}

static int rb_http_transport_send_all(RbHttpTransport *transport,
                                      const char *buf, int len)
{
    int sent;

    if (!transport || transport->sock == RB_HTTP_INVALID_SOCKET || !buf || len < 0)
        return RB_HTTP_ERR_BAD_ARG;
    sent = 0;
    while (sent < len) {
        int n;
        n = (int)send(transport->sock, (char *)buf + sent, len - sent, 0);
        if (n <= 0) return RB_HTTP_ERR_SEND;
        sent += n;
    }
    return 0;
}

static int rb_http_find_headers_end(const char *buf, int len)
{
    int i;

    for (i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') return i + 4;
    }
    return -1;
}

static int rb_http_status_is_2xx(const char *response, int len)
{
    int code;

    if (!response || len < 12) return 0;
    if (strncmp(response, "HTTP/", 5) != 0) return 0;
    code = 0;
    if (sscanf(response, "%*s %d", &code) != 1) return 0;
    return code >= 200 && code < 300;
}

int rb_http_get_json(const char *host, const char *path,
                     char *out_body, int out_body_size)
{
    RbHttpTransport transport;
    char request[RB_HTTP_MAX_REQUEST];
    int request_len;
    int len;
    int rc;
    int header_end;
    int body_len;

    if (!host || !path || !out_body || out_body_size <= 0) return RB_HTTP_ERR_BAD_ARG;
    out_body[0] = '\0';

    rc = rb_http_build_request(request, (int)sizeof(request), host, path);
    if (rc < 0) return rc;
    request_len = (int)strlen(request);

    rc = rb_http_transport_open(&transport, host, RB_HTTP_PORT);
    if (rc < 0) return rc;

    rc = rb_http_transport_send_all(&transport, request, request_len);
    if (rc < 0) {
        rb_http_transport_close(&transport);
        return rc;
    }

    len = 0;
    while (len < out_body_size - 1) {
        int want;
        int n;

        want = out_body_size - 1 - len;
        if (want > RB_HTTP_READ_CHUNK) want = RB_HTTP_READ_CHUNK;
        n = (int)recv(transport.sock, out_body + len, want, 0);
        if (n < 0) {
            rb_http_transport_close(&transport);
            out_body[0] = '\0';
            return RB_HTTP_ERR_READ;
        }
        if (n == 0) break;
        len += n;
    }
    if (len >= out_body_size - 1) {
        char tmp;
        int n;

        n = (int)recv(transport.sock, &tmp, 1, 0);
        if (n > 0) {
            rb_http_transport_close(&transport);
            out_body[0] = '\0';
            return RB_HTTP_ERR_BODY_TOO_BIG;
        }
        if (n < 0) {
            rb_http_transport_close(&transport);
            out_body[0] = '\0';
            return RB_HTTP_ERR_READ;
        }
    }
    rb_http_transport_close(&transport);
    out_body[len] = '\0';

    header_end = rb_http_find_headers_end(out_body, len);
    if (header_end < 0) {
        out_body[0] = '\0';
        return RB_HTTP_ERR_NO_HEADERS;
    }
    if (!rb_http_status_is_2xx(out_body, len)) {
        out_body[0] = '\0';
        return RB_HTTP_ERR_STATUS;
    }

    body_len = len - header_end;
    if (body_len >= out_body_size) {
        out_body[0] = '\0';
        return RB_HTTP_ERR_BODY_TOO_BIG;
    }
    memmove(out_body, out_body + header_end, (size_t)body_len);
    out_body[body_len] = '\0';
    return 0;
}

#ifdef RB_HTTP_TEST
#include "radio_browser_url.h"
#include "radio_browser_json.h"

#define RB_HTTP_TEST_BODY_SIZE 65536

int main(void)
{
    static char path[512];
    static char body[RB_HTTP_TEST_BODY_SIZE];
    RadioBrowserStation stations[RB_MAX_STATIONS];
    int rc;
    int count;
    int i;

    rc = rb_build_station_search_path(path, (int)sizeof(path), 0, 0, "MP3", 0, 10, 0);
    if (rc < 0) {
        printf("build path failed: %d\n", rc);
        return 1;
    }

    rc = rb_http_get_json("de1.api.radio-browser.info", path, body, (int)sizeof(body));
    if (rc < 0) {
        printf("http get failed: %d\n", rc);
        return 1;
    }

    count = rb_parse_stations_json(body, stations, RB_MAX_STATIONS);
    printf("station count: %d\n", count);
    for (i = 0; i < count; i++) {
        printf("%d: %s\n", i + 1, rb_station_play_url(&stations[i]));
    }
    return 0;
}
#endif
