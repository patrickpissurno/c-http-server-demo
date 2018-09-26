#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8085

int main()
{

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";

    char* body = "<!doctype html>\
<html>\
<head>\
<meta charset=\"utf-8\">\
<meta name = \"viewport\" content = \"width=device-width, initial-scale=1, shrink-to-fit=no\">\
<title>Definitely not Apache</title>\
<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\" integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" crossorigin=\"anonymous\">\
</head>\
<body>\
<div class=\"d-flex flex-column justify-content-center align-items-center\" style=\"min-height:100vh; min-width: 100vw\">\
<h1 class=\"mb-0\">Hello!</h1>\
<p>Sim, eu realmente n&#xE3;o sou o Apache. Mas agrade&#xE7;o sua visita.</p>\
<p><strong>M&#xE9;todo</strong>: {} <br> <strong>URL</strong>: {} <br> <strong>Host</strong>: {} <br> <strong>Seu IP</strong>: {}<p>\
</div>\
</body>\
</html>";

    char bodyLen[60];
    sprintf(bodyLen, "%lu", strlen(body));

    char* header1 = "\
    HTTP/1.1 200 OK\n\
    Date: Mon, 23 May 2005 22:38:34 GMT\n\
    Server: DefinitelyNotApache/1.0 (Windows)  (Baidu/Linux)\n\
    Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n\
    Etag: \"3f80f-1b6-3e1cb03b\"\n\
    Accept-Ranges: bytes\n\
    Content-Length: ";

    char* header2 = "\n\
    Connection: close\n\
    Content-Type: text/html; charset=UTF-8\n\n";

    char* header = malloc(strlen(header1) + strlen(header2) + strlen(bodyLen) + 1);
    
    strcat(header, header1);
    strcat(header, bodyLen);
    strcat(header, header2);

    char* payload = malloc(strlen(header) + strlen(body) + 1);
    strcat(payload, header);
    strcat(payload, body);

    // abre o socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // não zoa com Address Already in Use
    if (setsockopt(server_fd, 
                    SOL_SOCKET,
                    SO_REUSEPORT,
                    &opt,
                    sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // conecta socket com endereço de rede e porta
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // fica escutando
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while(1) {
        // aceita conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        //pega a request (buffer)
        valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);

        //como dizia jack estripador, vamos desmembrar
        char method[10] = "";
        char path[20] = "";
        char version[9] = "";
        char host[20], connection[12], cacheControl[30], userAgent[200], accept[200], acceptEnconding[200], acceptLanguage[200];
        int upgradeInsecureRequests;

        sscanf(buffer, 
                "%s %s %s\n\
                Host: %s\n\
                Connection: %s\n\
                Cache-Control: %s\n\
                Upgrade-Insecure-Requests: %d\n\
                User-Agent: %[^\n]s\n\
                Accept: %[^\n]s\n\
                Accept-Encoding: %[^\n]s\n\
                Accept-Language: %[^\n]s\n", method, path, version, host, connection, cacheControl, &upgradeInsecureRequests, userAgent, accept, acceptEnconding, acceptLanguage);

        //tá zoando no path e após o User-Agent
        printf("\n\n\n\n");
        printf("%s %s %s\n\
                Host: %s\n\
                Connection: %s\n\
                Cache-Control: %s\n\
                Upgrade-Insecure-Requests: %d\n\
                User-Agent: %s\n\
                Accept: %s\n\
                Accept-Encoding: %s\n\
                Accept-Language: %s\n", method, path, version, host, connection, cacheControl, upgradeInsecureRequests, userAgent, accept, acceptEnconding, acceptLanguage);

        printf("----PAYLOAD----\n\n\n%s\n", payload);
        send(new_socket, payload, strlen(payload), 0);
        printf("Hello message sent\n");
    }
    return 0;
}