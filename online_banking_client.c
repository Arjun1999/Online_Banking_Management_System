#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SEND_USERBUFFER_SIZE 1024
#define RECV_USERBUFFER_SIZE 1024
// #include <netinet/ip.h>

//SIGINT handler.
void signal_handler()
{
    printf("\n\nClient Shut Down.\n");
    exit(1);
}

void send_to_server(int sockfd, char *message)
{
    int size_header = (strlen(message) - 1) / SEND_USERBUFFER_SIZE + 1;
    int send_status = write(sockfd, &size_header, sizeof(int));

    char *send_message = (char *)malloc(size_header * SEND_USERBUFFER_SIZE);
    strcpy(send_message, message);

    for (int i = 0; i < size_header; i++)
    {
        int send_status = write(sockfd, send_message, SEND_USERBUFFER_SIZE);
        send_message += SEND_USERBUFFER_SIZE;
    }
}

char* receive_from_server(int sockfd)
{
    int size_header = 0;
    // printf("%d\n", sockfd);
    int receive_status = read(sockfd, &size_header, sizeof(int));
    // printf("%d\n", receive_status);
    if (receive_status <= 0)
    {
        shutdown(sockfd, SHUT_WR);
        return NULL;
    }

    char *message = (char *)malloc(size_header * RECV_USERBUFFER_SIZE);
    memset(message, 0, size_header * RECV_USERBUFFER_SIZE);

    char *receive_message = message;

    for (int i = 0; i < size_header; i++)
    {
        int receive_status = read(sockfd, message, RECV_USERBUFFER_SIZE);
        message += RECV_USERBUFFER_SIZE;
    }

    return receive_message;
}

int main(int argc, char **argv)
{
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printf("Error in catching SIGINT\n");
    }
    
    while(1) {
        
    if (argc < 2)
    {
        printf("Insufficient Number of arguments\nPlease enter a port number!\n");
        exit(0);
    }

    int port_number = atoi(argv[1]);
    
    printf("CLIENT...\n");

    int sockfd = socket(AF_INET, SOCK_STREAM,0);

    if(sockfd == -1)
    {
        perror("Socket Error\n");
        exit(0);
    }

    struct sockaddr_in sock_name;
    sock_name.sin_family = AF_INET;
    sock_name.sin_port = htons(port_number);
    sock_name.sin_addr.s_addr = inet_addr("127.0.0.1");

    int c = connect(sockfd, (struct sockaddr*)(&sock_name), sizeof(sock_name));

    if(c == -1)
    {
        perror("Connect Error\n");
        exit(0);
    }

    char *server_response;
    char message[140];
    
    while(1)
    {
    
    
    server_response = receive_from_server(sockfd);
    if(server_response == NULL)
    {
        printf("You have been disconnected from the server.\n");
        break;
    }
    printf("%s\n", server_response);
    free(server_response);


    memset(message,0,sizeof(message));
    // printf("WTF\n");
    printf("Enter response to server: \n");
    scanf(" %[^\n]", message);
    // printf("%s\n", message);
    send_to_server(sockfd, message);
    }
    exit(1);
    }
    return 0;
}
