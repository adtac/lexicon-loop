#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>

json_object *json;

static int destroy_flag = 0;
static void INT_HANDLER(int signo) {
    destroy_flag = 1;
}

char *solve(char *str, int *len) {
    json_object *obj = json_object_object_get(json, str);
    char *result = json_object_get_string(obj);

    *len = strlen(result);
    return result;
}

int write_back(struct lws *wsi_in, char *str) {
    if(str == NULL || wsi_in == NULL)
        return -1;

    int n, len;

    char *reply = solve(str, &len);

    char *out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING +
                                             len +
                                             LWS_SEND_BUFFER_POST_PADDING));
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, reply, len );
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

    free(out);

    return n;
}


static int ws_service_callback(struct lws *wsi,
                               enum lws_callback_reasons reason,
                               void *user,
                               void *in,
                               size_t len) {
    switch(reason) {
        case LWS_CALLBACK_ESTABLISHED:
            break;

        case LWS_CALLBACK_RECEIVE:
            write_back(wsi ,(char *)in);
            break;

        case LWS_CALLBACK_CLOSED:
            break;

        default:
            break;
    }

    return 0;
}

struct per_session_data {
    int fd;
};

int main(int argc, char *argv[]) {
    // Read the binary data
    FILE *json_file = fopen("content.json", "r");

    fseek(json_file, 0L, SEEK_END);
    int len = ftell(json_file);
    fseek(json_file, 0L, SEEK_SET);

    char *json_str = (char *)malloc(len + 10);
    if(json_file == NULL) {
        printf("error: words.json: no such file");
        return 0;
    }
    fgets(json_str, len + 10, json_file);
    json = json_tokener_parse(json_str);
    fclose(json_file);

    int port = atoi(argv[1]);
    const char *interface = NULL;
    struct lws_context_creation_info info;
    struct lws_protocols protocol;
    struct lws_context *context;

    const char *cert_path = NULL;
    const char *key_path = NULL;

    struct sigaction act;
    act.sa_handler = INT_HANDLER;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction( SIGINT, &act, 0);

    protocol.name = "lexicon-loop-protocol";
    protocol.callback = ws_service_callback;
    protocol.per_session_data_size=sizeof(struct per_session_data);
    protocol.rx_buffer_size = 0;

    memset(&info, 0, sizeof info);
    info.port = port;
    info.iface = interface;
    info.protocols = &protocol;
    info.extensions = lws_get_internal_extensions();
    info.ssl_cert_filepath = cert_path;
    info.ssl_private_key_filepath = key_path;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    context = lws_create_context(&info);
    if(context == NULL) {
        return -1;
    }

    while(!destroy_flag) {
        lws_service(context, 50);
    }
    usleep(10);
    lws_context_destroy(context);

    return 0;
}
