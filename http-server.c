#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8085

typedef struct REQUEST_DATA {
    char key[200];
    char value[200];
    struct REQUEST_DATA* next;
} REQUEST_DATA;

REQUEST_DATA* req_data_push(REQUEST_DATA* list, char* key, char* value){
    REQUEST_DATA* item = (REQUEST_DATA*)malloc(sizeof(REQUEST_DATA));
    item -> next = list;
    strcpy(item -> key, key);
    strcpy(item -> value, value);
    return item;
}

void req_data_free(REQUEST_DATA* list){
    if(list == NULL)
        return;
    if(list -> next != NULL)
        req_data_free(list -> next);
    free(list);
}

// https://github.com/irl/la-cucina/blob/master/str_replace.c
char* str_replace(char* string, const char* substr, const char* replacement) {
	char* tok = NULL;
	char* newstr = NULL;
	char* oldstr = NULL;
	int   oldstr_len = 0;
	int   substr_len = 0;
	int   replacement_len = 0;

	newstr = strdup(string);
	substr_len = strlen(substr);
	replacement_len = strlen(replacement);

	if (substr == NULL || replacement == NULL) {
		return newstr;
	}

	while ((tok = strstr(newstr, substr))) {
		oldstr = newstr;
		oldstr_len = strlen(oldstr);
		newstr = (char*)malloc(sizeof(char) * (oldstr_len - substr_len + replacement_len + 1));

		if (newstr == NULL) {
			free(oldstr);
			return NULL;
		}

		memcpy(newstr, oldstr, tok - oldstr);
		memcpy(newstr + (tok - oldstr), replacement, replacement_len);
		memcpy(newstr + (tok - oldstr) + replacement_len, tok + substr_len, oldstr_len - substr_len - (tok - oldstr));
		memset(newstr + oldstr_len - substr_len + replacement_len, 0, 1);

		free(oldstr);
	}

	free(string);

	return newstr;
}

void send_http(int request_socket, char* status, char* body){
    const char* format = "HTTP/1.1 %s\nContent-Length: %d\nConnection: close\nContent-Type: text/html; charset=UTF-8\n\n";

    char payload[10000];

    sprintf(payload, format, status, strlen(body));
    strcat(payload, body);

    printf("%s\n", payload);

    send(request_socket, payload, strlen(payload), 0);
}

void reply_error(int request_socket, char* status){
    send_http(request_socket, status, status);
}

void reply_file(int request_socket, char* filename, REQUEST_DATA* request_data){
    //remove o / do inicio
    sscanf(filename, " /%s", filename);

    if(strcmp(filename, "/") == 0)
        filename = "index.mustache";

    FILE* file = fopen(filename, "r");
    if(file == NULL){
        char temp[120];
        strcpy(temp, filename);
        strcat(temp, ".mustache");

        file = fopen(temp, "r");
        if(file == NULL){
            strcpy(temp, filename);
            strcat(temp, ".html");
            
            file = fopen(temp, "r");
            if(file == NULL)
                return reply_error(request_socket, "404 Not Found");
            else
                strcpy(filename, temp);
        }
        else
            strcpy(filename, temp);
    }

    char body[8192] = "";

    //le o arquivo
    char temp[1024];
    while(fscanf(file, " %1024[^\n]", temp) == 1)
        strcat(body, temp);
    
    fclose(file);

    if(strstr(filename, ".mustache") != NULL){
        char* parsing = (char*)(malloc(sizeof(char) * 8192));
        strcpy(parsing, body);


        REQUEST_DATA* item = request_data;
        while(item != NULL){
            char key[205] = "{{";
            strcat(key, item -> key);
            strcat(key, "}}");
            parsing = str_replace(parsing, key, item -> value);
            item = item -> next;
        }

        strcpy(body, parsing);
        free(parsing);
    }

    send_http(request_socket, "200 OK", body);
}

int main()
{
    // inicializa o socket do server
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // não zoa com Address Already in Use
    int opt = 1;
    if (setsockopt(server_socket,  SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // atribui ao socket do server o ip e porta
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // bota o socket do server em modo listening
    if (listen(server_socket, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int request_socket;

        struct sockaddr_in client_address;
        int client_address_length = 0;

        // aguarda conexao e aceita
        if ((request_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t*)&client_address_length)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        REQUEST_DATA* request_data = NULL;

        //nao ta pegando o valor certo do ip
        if(client_address.sin_family == AF_INET)
            request_data = req_data_push(request_data, "address", (char*)inet_ntoa(client_address.sin_addr));
        else
            request_data = req_data_push(request_data, "address", "localhost");

        //le a requisicao
        char buffer[1024];
        read(request_socket, buffer, 1024);

        char method[10];
        char path[200];
        sscanf(buffer, "%s %s", method, path);

        request_data = req_data_push(request_data, "method", method);
        request_data = req_data_push(request_data, "path", path);

        if(strcmp(method, "GET") != 0)
            reply_error(request_socket, "404 Not Found");
        else
            reply_file(request_socket, path, request_data);

        printf("Reply sent\n");
        close(request_socket);
        
        req_data_free(request_data);
    }
    return 0;
}