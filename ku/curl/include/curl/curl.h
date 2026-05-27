#ifndef CURL_CURL_H
#define CURL_CURL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wininet.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void CURL;
typedef size_t (*curl_write_callback)(void *contents, size_t size, size_t nmemb, void *userp);
typedef int CURLcode;

CURL *curl_easy_init(void);
void curl_easy_cleanup(CURL *curl);
CURLcode curl_easy_setopt(CURL *curl, int option, ...);
CURLcode curl_easy_perform(CURL *curl);
void curl_global_init(long flags);
void curl_global_cleanup(void);

#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLOPT_WRITEDATA 20012
#define CURLOPT_FOLLOWLOCATION 52
#define CURLE_OK 0

#define CURL_GLOBAL_ALL 3

#ifdef __cplusplus
}
#endif

#endif