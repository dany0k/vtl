/* Простой HTTP-клиент поверх libcurl.
   Реализует API из VTL/utils/curl/VTL_utils_curl_http_client_simple.h (borisenkov-reddit). */

#include <VTL/utils/curl/VTL_utils_curl_http_client_simple.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- общий write-callback (игнорируем тело ответа) ---------- */
static size_t discard_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    (void)ptr; (void)userdata;
    return size * nmemb;
}

/* ---------- helpers ------------------------------------------------ */
static int curl_perform(CURL *curl)
{
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return -1;
    return (code >= 200 && code < 300) ? 0 : (int)code;
}

static struct curl_slist *build_slist(const char **headers, int count)
{
    struct curl_slist *list = NULL;
    for (int i = 0; i < count; i++)
        list = curl_slist_append(list, headers[i]);
    return list;
}

/* ---------- public API --------------------------------------------- */
int http_get(const char *url)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_easy_setopt(c, CURLOPT_URL, url);
    return curl_perform(c);
}

int http_get_with_headers(const char *url, const char **headers, int headers_count)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_easy_setopt(c, CURLOPT_URL, url);
    struct curl_slist *h = build_slist(headers, headers_count);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    int ret = curl_perform(c);
    curl_slist_free_all(h);
    return ret;
}

int http_post(const char *url, const char *payload)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, payload);
    return curl_perform(c);
}

int http_post_with_headers(const char *url, const char *payload,
                           const char **headers, int headers_count)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, payload);
    struct curl_slist *h = build_slist(headers, headers_count);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    int ret = curl_perform(c);
    curl_slist_free_all(h);
    return ret;
}

int http_post_multipart(const char *url, const char *field_name,
                        const char *file_path, const char *chat_id,
                        const char *caption)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_mime *mime = curl_mime_init(c);
    curl_mimepart *part;

    part = curl_mime_addpart(mime);
    curl_mime_name(part, field_name);
    curl_mime_filedata(part, file_path);

    if (chat_id) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "chat_id");
        curl_mime_data(part, chat_id, CURL_ZERO_TERMINATED);
    }
    if (caption) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "caption");
        curl_mime_data(part, caption, CURL_ZERO_TERMINATED);
    }

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_MIMEPOST, mime);
    int ret = curl_perform(c);
    curl_mime_free(mime);
    return ret;
}

int http_post_multipart_group(const char *url, const char *chat_id,
                              const char *json_metadata, int count,
                              char **file_paths)
{
    CURL *c = curl_easy_init();
    if (!c) return -1;
    curl_mime *mime = curl_mime_init(c);
    curl_mimepart *part;

    if (chat_id) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "chat_id");
        curl_mime_data(part, chat_id, CURL_ZERO_TERMINATED);
    }
    if (json_metadata) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "media");
        curl_mime_data(part, json_metadata, CURL_ZERO_TERMINATED);
    }
    for (int i = 0; i < count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "file%d", i);
        part = curl_mime_addpart(mime);
        curl_mime_name(part, name);
        curl_mime_filedata(part, file_paths[i]);
    }

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_MIMEPOST, mime);
    int ret = curl_perform(c);
    curl_mime_free(mime);
    return ret;
}
