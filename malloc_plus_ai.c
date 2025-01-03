#include "malloc_plus_ai.h"

#include <ctype.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

#define API_URL "https://api.openai.com/v1/chat/completions"

#define DATA_START                                                             \
    "{\"model\":\"gpt-4o-mini\",\"messages\":[{\"role\":\"user\",\"content\":" \
    "\""

#define DATA_END "\"}],\"temperature\": 0.7}"

#define BASE_PROMPT                                                           \
    "You are a memory allocator for a computer, and you need to tell me how " \
    "many bytes of memory I would need to accomplish a certain task. Make "   \
    "sure to give your response as only a whole number of bytes, do not "     \
    "provide any other text. Here is what I request: "

static char *create_request_data(const char *prompt, size_t prompt_len) {
    const size_t header_len = sizeof(DATA_START BASE_PROMPT) - 1;

    char *buf = malloc(header_len + prompt_len + sizeof(DATA_END));
    if (!buf) {
        return 0;
    }

    memcpy(buf, DATA_START BASE_PROMPT, header_len);
    memcpy(buf + header_len, prompt, prompt_len);
    memcpy(buf + header_len + prompt_len, DATA_END, sizeof(DATA_END));

    return buf;
}

static int is_valid_prompt(const char *prompt, size_t prompt_len) {
    for (size_t i = 0; i < prompt_len; ++i) {
        if (!isalnum(prompt[i]) && !isspace(prompt[i])) {
            return 0;
        }
    }
    return 1;
}

#define BUFFER_SIZE 1024

struct sized_buffer {
    size_t len;
    char buf[BUFFER_SIZE];
};

static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
    struct sized_buffer *sb = userdata;

    if (nmemb > BUFFER_SIZE - sb->len) {
        return 0;
    }

    for (size_t i = 0; i < nmemb; ++i) {
        sb->buf[sb->len++] = ptr[i];
    }

    return nmemb;
}

void *mallocPlusAI(const char *prompt) {
    const size_t prompt_len = strlen(prompt);

    if (!is_valid_prompt(prompt, prompt_len)) {
        return 0;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return 0;
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, API_URL) != CURLE_OK) {
        goto cleanup;
    }

    struct curl_slist *list = 0, *temp = 0;

#define APPEND(data)                      \
    temp = curl_slist_append(list, data); \
    if (!temp) {                          \
        goto list_cleanup;                \
    }                                     \
    list = temp

    APPEND("Content-Type: application/json");
    APPEND("Authorization: Bearer " MALLOC_PLUS_AI_API_KEY);
#undef APPEND

    if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list) != CURLE_OK) {
        goto list_cleanup;
    }

    char *data = create_request_data(prompt, prompt_len);
    if (!data) {
        goto list_cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data) != CURLE_OK) {
        goto data_cleanup;
    }

    struct sized_buffer sb;
    sb.len = 0;

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) !=
        CURLE_OK) {
        goto data_cleanup;
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sb) != CURLE_OK) {
        goto data_cleanup;
    }

    if (curl_easy_perform(curl) != CURLE_OK) {
        goto data_cleanup;
    }

    // not enough space for NUL
    if (sb.len == BUFFER_SIZE) {
        goto data_cleanup;
    }
    sb.buf[sb.len++] = '\0';

    json_object *jso_root = json_tokener_parse(sb.buf);
    if (!jso_root) {
        goto data_cleanup;
    }

    json_object *jso_choices;
    if (!json_object_object_get_ex(jso_root, "choices", &jso_choices)) {
        goto json_cleanup;
    }

    if (json_object_get_type(jso_choices) != json_type_array) {
        goto json_cleanup;
    }

    json_object *jso_choice = json_object_array_get_idx(jso_choices, 0);
    if (!jso_choice) {
        goto json_cleanup;
    }

    json_object *jso_message;
    if (!json_object_object_get_ex(jso_choice, "message", &jso_message)) {
        goto json_cleanup;
    }

    json_object *jso_content;
    if (!json_object_object_get_ex(jso_message, "content", &jso_content)) {
        goto json_cleanup;
    }

    int byte_cnt = json_object_get_int(jso_content);
    if (byte_cnt <= 0) {
        goto data_cleanup;
    }

    json_object_put(jso_root);
    free(data);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return malloc(byte_cnt);

json_cleanup:
    json_object_put(jso_root);
data_cleanup:
    free(data);
list_cleanup:
    curl_slist_free_all(list);
cleanup:
    curl_easy_cleanup(curl);
    return 0;
}
