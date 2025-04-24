#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    char *host = NULL;
    char *commands = NULL;

    if (argc < 3 || strcmp(argv[1], "-h") != 0) {
        fprintf(stderr, "Usage: %s -h <host-a-conectar> [<lista-de-comandos-a-ejecutar>]\n", argv[0]);
        return 1;
    }

    host = argv[2];
    if (argc > 3) {
        commands = argv[3]; // Optional commands
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, host);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Write response to stdout
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

        // If commands are provided, handle them (e.g., append to URL or process differently)
        if (commands) {
            fprintf(stdout, "Executing commands: %s\n", commands);
            // Additional logic for commands can be added here
        }

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return 0;
}