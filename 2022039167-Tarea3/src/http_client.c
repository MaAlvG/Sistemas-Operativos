#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    char *host = NULL;
    char *method = "GET"; // Default method
    char *target = "/";
    char *data = NULL;

    if (argc < 3 || strcmp(argv[1], "-h") != 0) {
        fprintf(stderr, "Usage: %s -h <host> [<method> <target> <data>]\n", argv[0]);
        return 1;
    }

    host = argv[2];
    if (argc > 3) {
        method = argv[3]; // HTTP method (GET, POST, etc.)
    }
    if (argc > 4) {
        target = argv[4]; // Target file or path
    }
    if (argc > 5) {
        data = argv[5]; // Data for POST/PUT requests
    }

    curl = curl_easy_init();
    if (curl) {
        char url[1024];
        snprintf(url, sizeof(url), "%s%s", host, target);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);

        if (data && (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0)) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        }

        // Write response to stdout
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}