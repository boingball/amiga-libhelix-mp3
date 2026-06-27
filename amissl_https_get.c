/* Tiny standalone AmiSSL HTTPS GET smoke test.
 *
 * This intentionally does not use the player, decoders, GUI, Radio Browser, or
 * radio_stream transport code.  It exists to isolate AmiSSL library/init/vector
 * problems from playback integration bugs.
 */

#if !defined(AMIGA_M68K) || !defined(HAVE_AMISSL)
#include <stdio.h>
int main(void)
{
    puts("amissl_https_get must be built for AmigaOS with HAVE_AMISSL");
    return 1;
}
#else

#include <stdio.h>
#include <string.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <libraries/amisslmaster.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <amissl/amissl.h>

struct Library *SocketBase = NULL;
struct Library *AmiSSLMasterBase = NULL;
struct Library *AmiSSLBase = NULL;

#define TEST_HOST "ice1.somafm.com"
#define TEST_PATH "/groovesalad-128-mp3"
#define TEST_PORT 443
#define HEADER_BUF_SIZE 8192

static long test_socket = -1;
static SSL_CTX *test_ctx = NULL;
static SSL *test_ssl = NULL;

static void cleanup(void)
{
    if (test_ssl) {
        SSL_shutdown(test_ssl);
        SSL_free(test_ssl);
        test_ssl = NULL;
    }
    if (test_ctx) {
        SSL_CTX_free(test_ctx);
        test_ctx = NULL;
    }
    if (test_socket != -1) {
        CloseSocket(test_socket);
        test_socket = -1;
    }
    if (AmiSSLBase) {
        CloseAmiSSL();
        AmiSSLBase = NULL;
    }
    if (AmiSSLMasterBase) {
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
    }
    if (SocketBase) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
}

static int find_header_end(const char *buf, int len)
{
    int i;
    for (i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
            return i + 4;
    }
    return -1;
}

static int ascii_tolower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static int ascii_strncasecmp(const char *a, const char *b, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int ca = ascii_tolower((unsigned char)a[i]);
        int cb = ascii_tolower((unsigned char)b[i]);
        if (ca != cb || ca == 0 || cb == 0) return ca - cb;
    }
    return 0;
}

static void print_line_value(const char *headers, int header_len, const char *name)
{
    int name_len = (int)strlen(name);
    int i = 0;
    while (i < header_len) {
        int line_start = i;
        int line_end;
        while (i < header_len && headers[i] != '\n') i++;
        line_end = i;
        if (line_end > line_start && headers[line_end - 1] == '\r') line_end--;
        if (line_end - line_start >= name_len && ascii_strncasecmp(headers + line_start, name, name_len) == 0) {
            int v = line_start + name_len;
            while (v < line_end && (headers[v] == ' ' || headers[v] == '\t')) v++;
            printf("%s%.*s\n", name, line_end - v, headers + v);
            return;
        }
        i++;
    }
    printf("%s<not present>\n", name);
}

static int open_libraries(void)
{
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase) { puts("failed: OpenLibrary(bsdsocket.library)"); return 1; }

    AmiSSLMasterBase = OpenLibrary("amisslmaster.library", AMISSLMASTER_MIN_VERSION);
    if (!AmiSSLMasterBase) { puts("failed: OpenLibrary(amisslmaster.library)"); return 1; }

    if (!InitAmiSSLMaster(AMISSL_CURRENT_VERSION, TRUE)) {
        puts("failed: InitAmiSSLMaster version/init mismatch");
        return 1;
    }

    AmiSSLBase = OpenAmiSSL();
    if (!AmiSSLBase) { puts("failed: OpenAmiSSL"); return 1; }
    return 0;
}

static int tcp_connect(void)
{
    struct hostent *he;
    struct sockaddr_in sa;

    he = gethostbyname(TEST_HOST);
    if (!he || !he->h_addr_list || !he->h_addr_list[0]) { puts("failed: DNS lookup"); return 1; }

    test_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (test_socket == -1) { printf("failed: socket errno=%ld\n", Errno()); return 1; }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TEST_PORT);
    memcpy(&sa.sin_addr, he->h_addr_list[0], (size_t)he->h_length);

    if (connect(test_socket, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        printf("failed: TCP connect errno=%ld\n", Errno());
        return 1;
    }
    return 0;
}

static int tls_connect(void)
{
    test_ctx = SSL_CTX_new(TLS_client_method());
    if (!test_ctx) { puts("failed: SSL_CTX_new"); return 1; }
    SSL_CTX_set_verify(test_ctx, SSL_VERIFY_NONE, NULL);

    test_ssl = SSL_new(test_ctx);
    if (!test_ssl) { puts("failed: SSL_new"); return 1; }
    SSL_set_fd(test_ssl, (int)test_socket);
    if (SSL_connect(test_ssl) != 1) {
        puts("failed: SSL_connect");
        return 1;
    }
    return 0;
}

int main(void)
{
    static char headers[HEADER_BUF_SIZE + 1];
    const char *request =
        "GET " TEST_PATH " HTTP/1.1\r\n"
        "Host: " TEST_HOST "\r\n"
        "User-Agent: BoingPlayer/0.1 AmigaOS\r\n"
        "Icy-MetaData: 1\r\n"
        "Connection: close\r\n"
        "\r\n";
    int total = 0;
    int header_end = -1;
    int rc = 1;

    puts("AmiSSL HTTPS GET smoke test: https://" TEST_HOST TEST_PATH);
    if (open_libraries() != 0) goto out;
    if (tcp_connect() != 0) goto out;
    if (tls_connect() != 0) goto out;

    if (SSL_write(test_ssl, request, (int)strlen(request)) <= 0) {
        puts("failed: SSL_write request");
        goto out;
    }

    while (total < HEADER_BUF_SIZE) {
        int n = (int)SSL_read(test_ssl, headers + total, HEADER_BUF_SIZE - total);
        if (n <= 0) break;
        total += n;
        header_end = find_header_end(headers, total);
        if (header_end >= 0) break;
    }
    if (header_end < 0) { puts("failed: response headers not complete"); goto out; }
    headers[header_end] = '\0';

    printf("Status: %.*s\n", (int)(strchr(headers, '\n') ? strchr(headers, '\n') - headers : strlen(headers)), headers);
    print_line_value(headers, header_end, "Content-Type:");
    rc = 0;

out:
    cleanup();
    return rc;
}
#endif
