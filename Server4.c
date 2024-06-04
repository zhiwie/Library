#include <io.h>
#include <stdio.h>
#include <winsock2.h>
#include <ctype.h>
#include <time.h>

#define MY_PORT     8989
#define MAXBUF      256

void reverseOrder(char *buffer) {
    int i = 0, j = strlen(buffer) - 1; // strlen() - 1 to exclude the null character within the string
    char temp;

    while (i < j) {
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
        i++;
        j--;
    }
}

void upper(char *buffer) {
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 32;
        }
    }
}
void displayLength(char *buffer, int msgLength){
    if(!isspace((unsigned char)buffer[0])) {// If received message is not whitespaces, process it 
        printf("Received message of length: %d\n", msgLength);//to prevent printing 2 times
    }
}


void getTime(char *buffer, int offset) {
    time_t rawtime;//to hold current time
    struct tm *timeInfo;//ptr that holds current time in the form of hours, minutes, seconds

    time(&rawtime);//stores current time 
    timeInfo = gmtime(&rawtime);//stores current time in the form of hours, minutes, seconds 

    //to handle both +ve and -ve offset accurately
    rawtime = mktime(timeInfo);//convert broken-down time back to represent valid time
    rawtime += offset * 3600; //convert offset to seconds as linear time representation
    timeInfo = localtime(&rawtime);//converts back into broken-down time 

    strftime(buffer, MAXBUF, "%H:%M:%S", timeInfo);//formatting string to show time in HH:MM:SS
    sprintf(buffer + strlen(buffer), " GMT %+d\r\n", offset);//appends GMT offset to the end of buffer, hence shows time in HH:MM:SS GMT offset
}


int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET sockfd, clientfd;
    struct sockaddr_in self;
    char buffer[MAXBUF];
    char currentDate[MAXBUF];
    char currentTime[MAXBUF];
    int offset;

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

    /*---initialize address/port structure---*/
    self.sin_family = AF_INET;
    self.sin_port = htons(MY_PORT); // host to network short
    self.sin_addr.s_addr = INADDR_ANY;

    /*---assign a port number to the socket---*/
    if (bind(sockfd, (struct sockaddr *) &self, sizeof(self)) != 0) {
        perror("socket--bind");
        exit(errno);
    }

    puts("Bind done");

    /*---make it a "listening socket"---*/
    if (listen(sockfd, 20) != 0) {
        perror("socket--listen");
        exit(errno);
    }

    puts("Waiting for incoming connections...");

    int exitServer=0;
    while (!exitServer) {
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);

        //gets current time
        struct tm tm;
        time_t t;
        time(&t);
        tm = *localtime(&t);

        /*---accept a connection (creating a data pipe)---*/
        clientfd = accept(sockfd, (struct sockaddr *) &client_addr, &addrlen);
        printf("\nConnection successful!\nIP: %s Port number: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        while (1) {// forever receiving data from the client and store in buffer
            int msgLength = recv(clientfd, buffer, MAXBUF, 0);
            
            if (msgLength<=0){//not receiving any data from client (client disconnected)
                printf("Client disconnecting...\n");//prints at server side
                break;//break out of loop
            }

            buffer[msgLength] = '\0';//null-terminate the receive data to make it a valid string

            displayLength(buffer, msgLength);

            if (strcmp(buffer, "Exit Server") == 0) {
                printf("Server disconnecting...\n");//prints at server side
                exitServer=1;//update to break out infinite loop
                break;//keeps receiving client input but does not send any response
            }
            else if (strcmp(buffer, "Date")==0){
                time(&t);//update tm with current time
                tm = *localtime(&t);
                strftime(currentDate, 80, "%a %b %d %H:%M:%S %Y GMT +8\r\n", &tm);//formatting string
                send(clientfd,currentDate,strlen(currentDate),0);//represented as single line and terminated with carriage return and line feed
                fflush(stdout);//flush away prev data
                buffer[0] = '\0';//null terminate the string to make it valid
            }
            else if (strncmp(buffer, "Time", 4) == 0) {//compares first 4 characters from user input
                if (strcmp(buffer, "Time") == 0 ){
                    offset = 8;//Malaysia time GMT +8
                }else if (strcmp(buffer, "Time GMT") == 0) {
                    offset = 0;
                } else if (strcmp(buffer, "Time PST") == 0) {
                    offset = -8;
                } else if (strcmp(buffer, "Time MST") == 0) {
                    offset = -7;
                } else if (strcmp(buffer, "Time CST") == 0) {
                    offset = -6;
                } else if (strcmp(buffer, "Time EST") == 0) {
                    offset = -5;
                } else if (strcmp(buffer, "Time CET") == 0) {
                    offset = 1;
                } else if (strcmp(buffer, "Time MSK") == 0) {
                    offset = 3;
                } else if (strcmp(buffer, "Time JST") == 0) {
                    offset = 9;
                } else if (strcmp(buffer, "Time AEDT") == 0) {
                    offset = 11;
                } else {
                    send(clientfd, "Error. Invalid time zone command entered.\n", strlen("Error. Invalid time zone command entered.\n"),0);
                    buffer[0]='\0';
                    continue;
                }
                getTime(currentTime,offset);
                send(clientfd, currentTime,strlen(currentTime),0);
                buffer[0] = '\0';
            }
            else{
                reverseOrder(buffer);
                upper(buffer);
                send(clientfd, buffer, msgLength, 0);//send back reversed and capitalized version of input
            }
        }
        /*---close connection---*/
		close(clientfd);
    }
    /*---clean up (should never get here!)---*/
    close(sockfd);
    WSACleanup();
    return 0;
}