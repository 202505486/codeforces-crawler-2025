#include "curl/curl.h"

#pragma comment(lib, "wininet.lib")

typedef struct {
    char *url;
    curl_write_callback write_func;
    void *write_data;
} CURL_Internal;

void curl_global_init(long flags) {
    UNREFERENCED_PARAMETER(flags);
}

void curl_global_cleanup(void) {
}

CURL *curl_easy_init(void) {
    CURL_Internal *curl = malloc(sizeof(CURL_Internal));
    memset(curl, 0, sizeof(CURL_Internal));
    return (CURL *)curl;
}

void curl_easy_cleanup(CURL *curl) {
    if (curl) {
        CURL_Internal *c = (CURL_Internal *)curl;
        free(c->url);
        free(c);
    }
}

CURLcode curl_easy_setopt(CURL *curl, int option, ...) {
    va_list args;
    va_start(args, option);
    
    CURL_Internal *c = (CURL_Internal *)curl;
    
    switch (option) {
        case CURLOPT_URL: {
            const char *url = va_arg(args, const char *);
            free(c->url);
            c->url = malloc(strlen(url) + 1);
            strcpy(c->url, url);
            break;
        }
        case CURLOPT_WRITEFUNCTION:
            c->write_func = va_arg(args, curl_write_callback);
            break;
        case CURLOPT_WRITEDATA:
            c->write_data = va_arg(args, void *);
            break;
        case CURLOPT_FOLLOWLOCATION:
            (void)va_arg(args, long);
            break;
    }
    
    va_end(args);
    return CURLE_OK;
}

CURLcode curl_easy_perform(CURL *curl) {
    CURL_Internal *c = (CURL_Internal *)curl;
    
    if (!c->url || !c->write_func) {
        return 1;
    }
    
    HINTERNET hInternet = InternetOpenA("curl/7.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return 1;
    
    HINTERNET hConnect = InternetOpenUrlA(hInternet, c->url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return 1;
    }
    
    char buffer[4096];
    DWORD bytesRead;
    
    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        c->write_func(buffer, 1, bytesRead, c->write_data);
    }
    
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    return CURLE_OK;
}