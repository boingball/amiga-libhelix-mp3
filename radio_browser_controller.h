#ifndef RADIO_BROWSER_CONTROLLER_H
#define RADIO_BROWSER_CONTROLLER_H

#include "radio_browser.h"
#include "radio_stream_probe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RB_CONTROLLER_MAX_STATIONS 50
#define RB_CONTROLLER_JSON_BUFFER_SIZE 131072
#define RB_CONTROLLER_LAST_ERROR_SIZE 128
#define RB_CONTROLLER_LAST_PATH_SIZE 512
#define RB_CONTROLLER_DEFAULT_HOST "de1.api.radio-browser.info"

typedef struct RadioBrowserController {
    RadioBrowserStation stations[RB_CONTROLLER_MAX_STATIONS];
    int station_count;
    int selected_index;
    char json_buffer[RB_CONTROLLER_JSON_BUFFER_SIZE];
    char last_error[RB_CONTROLLER_LAST_ERROR_SIZE];
    char last_path[RB_CONTROLLER_LAST_PATH_SIZE];
    int raw_station_count;
    char name[RB_MAX_NAME];
    char codec[RB_MAX_CODEC];
    char countrycode[RB_MAX_COUNTRY];
    char tag[RB_MAX_TAGS];
    int limit;
    int max_bitrate;
    int offset;
    int hide_hls;
    int hide_offline;
    int hide_ssl_error;
} RadioBrowserController;

enum {
    RB_CONTROLLER_OK = 0,
    RB_CONTROLLER_ERR_BAD_ARG = -1,
    RB_CONTROLLER_ERR_NO_SELECTION = -2,
    RB_CONTROLLER_ERR_HTTPS_UNSUPPORTED = -3,
    RB_CONTROLLER_ERR_RESPONSE_TOO_LARGE = -4,
    RB_CONTROLLER_ERR_PARSE_FAILED = -5,
    RB_CONTROLLER_ERR_SEARCH_FAILED = -6
};

void rb_controller_init(RadioBrowserController *controller);
int rb_controller_search(RadioBrowserController *controller);
const RadioBrowserStation *rb_controller_get_station(
    const RadioBrowserController *controller,
    int index
);
int rb_controller_set_selected(
    RadioBrowserController *controller,
    int index
);
int rb_controller_probe_selected(
    RadioBrowserController *controller,
    RbStreamInfo *info,
    unsigned char *peek_buf,
    int peek_buf_size,
    int *peek_len
);

#ifdef __cplusplus
}
#endif

#endif
