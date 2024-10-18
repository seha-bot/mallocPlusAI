#include "malloc_plus_ai.h"

#include <ctype.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define API_URL "https://api.openai.com/v1/chat/completions"

#define DATA_START                                                             \
  "{"                                                                          \
  "\"model\": \"gpt-4o-mini\","                                                \
  "\"messages\": [{\"role\": \"user\", \"content\": \""

#define DATA_END                                                               \
  "\"}],"                                                                      \
  "\"temperature\": 0.7"                                                       \
  "}"

#define BASE_PROMPT                                                            \
  "You are a memory allocator for a computer, and you need to tell me how "    \
  "many bytes of memory I would need to accomplish a certain task. Make sure " \
  "to give your response as only a whole number of bytes, do not provide any " \
  "other text. Here is what I request: "

#define BUFFER_SIZE 100

struct sized_buffer {
  size_t len;
  char buf[BUFFER_SIZE];
};

static char *create_request_data(const char *prompt) {
  const size_t len = strlen(prompt);
  char *buf = malloc(sizeof(DATA_START) + sizeof(DATA_END) +
                     sizeof(BASE_PROMPT) + len - 2);
  if (!buf) {
    return 0;
  }

  char *end;
  end = strcpy(buf, DATA_START);
  end = strcpy(end, BASE_PROMPT);
  end = strcpy(end, prompt);
  strcpy(buf, DATA_END);

  return buf;
}

static int is_valid_prompt(const char *prompt) {
  const size_t len = strlen(prompt);
  for (size_t i = 0; i < len; ++i) {
    if (!isalnum(prompt[i])) {
      return 0;
    }
  }
  return 1;
}

static int is_valid_response(struct sized_buffer *sb) {
  for (size_t i = 0; i < sb->len; ++i) {
    if (!isdigit(sb->buf[i])) {
      return 0;
    }
  }
  return 1;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  struct sized_buffer *sb = userdata;

  if (nmemb > BUFFER_SIZE - sb->len) {
    return 0;
  }

  for (size_t i = 0; i < nmemb; ++i) {
    sb->buf[size++] = ptr[i];
  }

  return nmemb;
}

void *mallocPlusAI(const char *prompt) {
  if (!is_valid_prompt(prompt)) {
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

#define APPEND(data)                                                           \
  temp = curl_slist_append(list, data);                                        \
  if (!temp) {                                                                 \
    goto list_cleanup;                                                         \
  }                                                                            \
  list = temp

  APPEND("Content-Type: application/json");
  APPEND("Authorization: Bearer " MALLOC_PLUS_AI_API_KEY);
#undef APPEND

  if (curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list) != CURLE_OK) {
    goto list_cleanup;
  }

  char *data = create_request_data(prompt);
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

  if (!is_valid_response(&sb)) {
    goto data_cleanup;
  }

  if (sb.len == BUFFER_SIZE) {
    goto data_cleanup;
  }

  sb.buf[sb.len++] = '\0';
  int byte_cnt = atoi(sb.buf);
  if (byte_cnt < 0) {
    goto data_cleanup;
  }

  free(data);
  curl_slist_free_all(list);
  curl_easy_cleanup(curl);
  return malloc(byte_cnt);

data_cleanup:
  free(data);
list_cleanup:
  curl_slist_free_all(list);
cleanup:
  curl_easy_cleanup(curl);

  return 0;
}
