#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <libwebsockets.h>

struct word {
    int num_lit;
    int word_len;
    char *word_str;
    int *positions;
};

int num_words;
struct word *word_list;

static int destroy_flag = 0;
static void INT_HANDLER(int signo) {
    destroy_flag = 1;
}

int search(char* word) {
    int low = 0, high = num_words, mid;

    while (low < high) {
        mid = (low + high) / 2;
        int result = strcmp(word, (word_list + mid)->word_str);
        if (result < 0)
            high = mid;
        else if (result > 0)
            low = mid + 1;
        else
            return mid;
    }

    return -1;
}

int dfs(int pos, int level, int *visited, int *ans) {
    visited[pos] = 1;

    int total = word_list[pos].num_lit;

    for(int i = rand()%total; i < total; i++) {
        int k = word_list[pos].positions[i];
        if (!visited[k]) {
            ans[level] = pos;
            int ret = dfs(k, level + 1, visited, ans);
            if(ret > 0)
                return ret;
        }
        else {
            ans[level] = pos;
            ans[level + 1] = k;
            return level + 2;
        }
    }

    return 0;
}

char *str[1000009];

char *solve(char *str, int *len) {
    srand(time(NULL));
    int *visited = (int *)malloc(sizeof(int) * num_words);
    int *ans = (int *)malloc(sizeof(int) * num_words);

    memset(visited, 0, sizeof(visited));

    int g = search(str);
    if (g == -1) {
        *len = 3;
        char *out = (char *)malloc(*len);
        strcpy(out, "[]");
        return out;
    }

    int done = 0;
    int loop_level = dfs(g, 0, visited, ans);
    if(loop_level > 0) {
        *len = 10;
        for(int i = 0; i < loop_level; i++)
            *len += word_list[ans[i]].word_len + 4;

        char *out = (char *)malloc(sizeof(char) * (*len));
        int written = 2;
        out[0] = '[';
        out[1] = '\"';
        for(int i = 0; i < loop_level; i++) {
            strcpy(out + written, word_list[ans[i]].word_str);
            written += word_list[ans[i]].word_len;
            strcpy(out + written, "\", \"");
            written += 4;
        }
        strcpy(out + written - 3, "]");
        *len = written - 2;
        return out;
    }
    else {
        *len = 3;
        char *out = (char *)malloc(*len);
        strcpy(out, "[]");
        return out;
    }
}

int write_back(struct lws *wsi_in, char *str) {
    if(str == NULL || wsi_in == NULL)
        return -1;

    int n, len;

    char *reply = solve(str, &len);

    char *out = NULL;

    out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING +
                                       len +
                                       LWS_SEND_BUFFER_POST_PADDING));
    memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, reply, len );
    n = lws_write(wsi_in, out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

    free(reply);
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
    FILE* in = fopen("data.bin", "rb");
    fread(&num_words, sizeof(int), 1, in);
    word_list = (struct word*)malloc(sizeof(struct word) * num_words);

    for (int i = 0; i < num_words; i++) {
        fread(&word_list[i].word_len, sizeof(int), 1, in);

        word_list[i].word_str = (char*)malloc(word_list[i].word_len);
        fread(word_list[i].word_str, 1, word_list[i].word_len, in);

        fread(&(word_list[i].num_lit), sizeof(int), 1, in);

        word_list[i].positions = (int*)malloc(sizeof(int) * (word_list[i].num_lit));
        fread((word_list[i].positions), sizeof(int), word_list[i].num_lit, in);
    }
    fclose(in);

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
