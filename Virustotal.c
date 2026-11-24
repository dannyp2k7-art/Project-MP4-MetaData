#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <jansson.h>
#include <openssl/sha.h>
#include "Virustotal.h"
#include "gtkMP4.h"

//Ligne permettant de tranformer les printfs en fonctions permettant d'afficher dans la box GTK (afficher_texte_format garde les mêmes arguments qu'un printf)
#define printf(...) afficher_texte_format(__VA_ARGS__)

// Ton propre memcpy (copie N octets, même les \0 !)
void *my_memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}



struct MemoryStruct {
    char *memory;
    size_t size;
};


const char* read_apikey() {
    FILE *file = fopen("apikey.txt", "r");
    if (!file) {
        printf("❌ ERREUR: Créez apikey.txt avec votre clé VirusTotal !\n");
        return NULL;
    }

    static char buffer[256];  // Buffer statique (persistant)
    if (fgets(buffer, sizeof(buffer), file)) {
        // Enlève \n final
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = 0;
        fclose(file);
        printf("🔑 Clé API chargée (%ld chars)\n", strlen(buffer));
        return buffer;
    }
    fclose(file);
    printf("❌ apikey.txt vide !\n");
    return NULL;
}



static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->memory = ptr;
    my_memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

// Calcul SHA256
int calculate_sha256(const char *path, char outputBuffer[65]) {
    FILE *file = fopen(path, "rb");
    if (!file) return -1;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = malloc(bufSize);

    if (!buffer) { fclose(file); return -1; }

    int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, bufSize, file))) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;

    fclose(file);
    free(buffer);
    return 0;
}
int virus_analyse(const char* filepath) {

    const char *apikey = read_apikey();
    if (!apikey) return 1;
    char sha256_hash[65];

    printf("Calcul du SHA256 de '%s'...\n", filepath);
    if (calculate_sha256(filepath, sha256_hash) != 0) {
        printf("Erreur lecture fichier !\n");
        return 1;
    }
    printf("Hash: %s\n", sha256_hash);

    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = { malloc(1), 0 };

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        char url[200];
        snprintf(url, sizeof(url), "https://www.virustotal.com/api/v3/files/%s", sha256_hash);

        struct curl_slist *headers = NULL;
        char auth_header[100];
        snprintf(auth_header, sizeof(auth_header), "x-apikey: %s", apikey);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        printf("Interrogation VirusTotal...\n");

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            printf("Erreur CURL: %s\n", curl_easy_strerror(res));
        } else {
            json_t *root = json_loads(chunk.memory, 0, NULL);
            if (!root) {
                printf("Erreur JSON (ou réponse vide)\n");
            } else {
                json_t *data = json_object_get(root, "data");
                if (data) {
                    json_t *attrs = json_object_get(data, "attributes");
                    json_t *stats = json_object_get(attrs, "last_analysis_stats");

                    int malicious = json_integer_value(json_object_get(stats, "malicious"));
                    int harmless = json_integer_value(json_object_get(stats, "harmless"));
                    int undetected = json_integer_value(json_object_get(stats, "undetected"));
                    int total = malicious + harmless + undetected;

                    printf("\nRÉSULTAT DU SCAN :\n");
                    printf("Danger: %d\n", malicious);
                    printf("Total moteurs: %d\n", total);

                    if (malicious > 0) printf("\n️  DANGER ! Fichier infecté.\n");
                    else printf("\nFichier PROPRE.\n");

                    printf("🔗 https://www.virustotal.com/gui/file/%s\n", sha256_hash);
                } else {
                    json_t *error = json_object_get(root, "error");
                    if (error) {
                        const char *code = json_string_value(json_object_get(error, "code"));
                        if (code && strcmp(code, "NotFoundError") == 0) {
                            printf("\nFichier inconnu (jamais scanné).\n");

                        } else {
                            printf("\nErreur API: %s\n", json_string_value(json_object_get(error, "message")));
                        }
                    }
                }
                json_decref(root);
            }
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();
    return 0;
}
