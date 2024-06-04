#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#define MY_PORT 8989
#define MAXBUF 256

int main() {
    WSADATA wsa;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAXBUF];
    char clientMsg[MAXBUF];

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d", WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    /*---create streaming socket---*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(errno);
    }

    printf("Socket created.\n");

    /*---initialize address/port structure for the server---*/
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MY_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //server and client is running on the same machine

    /*---connect to server---*/
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect");
        exit(errno);
    }

    printf("Connected to the server.\n");

    while (1) {
        printf("Enter: ");
        fgets(clientMsg, MAXBUF, stdin);
        clientMsg[strcspn(clientMsg, "\n")] = '\0'; //remove newline character from user input

        send(sockfd, clientMsg, strlen(clientMsg), 0);//sends client msg to server

        if (strcmp(clientMsg, "Exit Client") == 0) {
            closesocket(sockfd);
            printf("Client disconnecting...\n");//prints at client side
            break;//exit loop and proceed to cleanup
        }

        int msgServer = recv(sockfd, buffer, MAXBUF, 0);//receive modified data from server

        if (msgServer == SOCKET_ERROR) {//when no response is received from server
            printf("Server disconnected. No response from server.\n");
            buffer[0] = '\0'; //set buffer to an empty string
        }
        else{
            buffer[msgServer] = '\0';//null-terminate the receive data to make it a valid string
        }
        printf("From server: %s\n", buffer);
    }
    /*---clean up---*/
    WSACleanup();
    return 0;
} 