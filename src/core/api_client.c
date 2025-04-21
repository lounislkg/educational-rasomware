#include "core/api_client.h"

const char *IP = "127.0.0.1"; // Adresse IP du serveur
const u_short PORT = 3000;          // Port du serveur

static int error_handling(const char *msg, SOCKET sock, WSADATA wsaData)
{
    perror(msg);
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
    }
    else
    {
        printf("Socket is invalid\n");
    }
    if (wsaData.wVersion != 0)
    {
        WSACleanup();
    }
    else
    {
        printf("WSA is not initialized\n");
    }
    return 1;
}

int SendKey(char _key[129])
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in serverInfo;
    // Contenu JSON à envoyer
    //printf("Key : %s\n", _key);
    char *host = "/";
    //we use X to control the string length
    if (_key[128] != 0)
    {
        return error_handling("The given key is not a 128 bytes (the 256 char is not X)", ConnectSocket, wsaData);
    }
    // Remove the last char X
    char key[128] = "";
    for (int i = 0; i < 218; i++)
    {
        key[i] = _key[i];
    }
    
    char json_body[512];
    snprintf(json_body, sizeof(json_body), "{\"key\":\"%s\"}", key);
    //printf("JSON body: %s\n", json_body); // Afficher le corps JSON
    char recvbuf[512];
    int result, recvbuflen = 512;

    // Initialiser Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // Créer un socket
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Transformer l'adresse du serveur en adresse binaire utilisable et se connecter
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(PORT); // Port HTTP

    if (inet_pton(AF_INET, IP, &serverInfo.sin_addr) != 1) // Convertir l'adresse IP en binaire et check si tout s'est bien passé
    {
        char *msg = "";
        snprintf(msg, 1024, "inet_pton failed%d\n", WSAGetLastError());
        return error_handling(msg, ConnectSocket, wsaData);
    }
    // printf("IP address: %s\n", inet_ntoa(serverInfo.sin_addr)); // Afficher l'adresse IP
    // printf("Port: %d\n", ntohs(serverInfo.sin_port));           // Afficher le port
    result = connect(ConnectSocket, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
    if (result == SOCKET_ERROR)
    {
        char *msg = "";
        snprintf(msg, 1024, "Connect failed%d\n", WSAGetLastError());
        return error_handling(msg, ConnectSocket, wsaData);
    }

    char *sendbuf = (char *)malloc(1024); // Allocation de mémoire pour la requête
    if (sendbuf == NULL)
    {
        return error_handling("Memory allocation failed", ConnectSocket, wsaData);
    }

   
    // Construction de la requête HTTP POST avec le corps JSON
    // Le corp fait forcément 42 bits, donc on le met en dur
    snprintf(sendbuf, 512,
             "POST / HTTP/1.1\r\n" // Remplacez par votre endpoint
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             host, 138, json_body);

    // Envoyer et recevoir des données
    result = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (result == SOCKET_ERROR)
    {
        printf("Send failed: %d\n", WSAGetLastError());
        return error_handling("Send failed", ConnectSocket, wsaData);
    }

    result = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (result > 0)
        printf("Bytes received: %d\n", result);
    else
        printf("Recv failed: %d\n", WSAGetLastError());

    // Nettoyage
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
/* 
int main(int argc, char *argv[])
{
    return SendKey(argv[1]);
} */