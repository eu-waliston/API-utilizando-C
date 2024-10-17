#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_request(int new_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(new_socket, buffer, BUFFER_SIZE);
    printf("Request:\n%s\n", buffer);

    // Resposta simples
    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Connection: close\r\n\r\n"
                           "{\"message\": \"Hello, World!\"}";
    write(new_socket, response, strlen(response));
    close(new_socket);
}

//função para tratar a resposta do servidor
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    // A resposta do servidor é arm,azenada aqui
    size_t total_size = size * nmemb;
    printf("%. *s", (int)total_size, (char *)ptr);
    return total_size;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    CURL *curl;
    CURLcode res;

    //Inicializando a biblioteca curl
    curl_global_init(CURL_GLOBAL_INIT);
    curl = curl_easy_init();

    // Criar socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Opções do socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Definir endereço e porta do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associar o socket ao endereço
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escutar por conexões
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor ouvindo na porta %d\n", PORT);

    while (1) {
        // Aceitar uma nova conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        handle_request(new_socket);
    }

    if(curl) {
        //URL para onde o POST será enviado
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/endpoint");

        // O JSON que será enviado
        const char *json_data = "{\"chave\":\"valor\"}";

        //Configura o cabeçalho para indicar que estamos enviando JSON
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,"Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Configure os dados a serem enviados
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

        // Configura a função de callback para tratar a resposta
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        //Executa a requisição
        res = curl_easy_perform(curl);

        // Verifica se houve erro
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        //Limpa os cabeçalhos
        curl_slist_free_all(headers);

        //Finaliza a sessão curl
        curl_easy_cleanup(curl);
    }


    //Finalzia a biblioteca curl
    curl_global_cleanup();
    return 0;
}
