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
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SEND_USERBUFFER_SIZE 1024
#define RECV_USERBUFFER_SIZE 1024
// #include <netinet/ip.h>

// Send and Receive protocols to take care of the message boundary problem.
// Refer to "https://www.codeproject.com/Articles/11922/Solution-for-TCP-IP-client-socket-message-boundary"
// for the message boundary problem

void send_to_client(int clientfd, char *message)
{
    int size_header = (strlen(message)-1)/ SEND_USERBUFFER_SIZE + 1;
    int send_status = write(clientfd, &size_header, sizeof(int));

    char *send_message = (char *)malloc(size_header * SEND_USERBUFFER_SIZE );
    strcpy(send_message, message);

    for(int i = 0 ; i < size_header; i++)
    {
        int send_status = write(clientfd, send_message, SEND_USERBUFFER_SIZE);
        send_message += SEND_USERBUFFER_SIZE;
    }
}

char* receive_from_client(int clientfd)
{
    int size_header = 0;
    // printf("%d\n", clientfd);
    // printf("%ls\n", &size_header);
    int receive_status = read(clientfd, &size_header, sizeof(int));
    // printf("%d\n", receive_status);
    if(receive_status <= 0)
    {
        // printf("WTF\n");
        shutdown(clientfd, SHUT_WR);
        return NULL;
    }

    char *message = (char *)malloc(size_header * RECV_USERBUFFER_SIZE);
    memset(message, 0, size_header * RECV_USERBUFFER_SIZE);

    char *receive_message = message;

    for (int i = 0; i < size_header; i++)
    {
        int receive_status = read(clientfd, message, RECV_USERBUFFER_SIZE);
        message += RECV_USERBUFFER_SIZE;
    }
    // printf("%s\n", receive_message);

    return receive_message;
}

// void check_client_connectivity(int client_message)
// {
//     if (client_message == 0)
//     {
//         printf("\nClient has disconnected abruptly.\n");
//         pthread_exit(NULL);
//     }
// }

void login_prompt(char *username, char *password, int clientfd)
{
    char *entered_username;
    char *entered_password;

    send_to_client(clientfd, "Enter Username : ");
    entered_username = receive_from_client(clientfd);
    // printf("%s\n", entered_username);

    send_to_client(clientfd, "Enter Password : ");
    entered_password = receive_from_client(clientfd);
    // printf("%s\n", entered_password);
    // Copying the entered_username and entered_password to passed arguments username and password
    int i = 0;
    while(entered_username[i] != '\0' && entered_username[i] != '\n')
    {
        username[i] = entered_username[i];
        i+=1;
    }
    username[i] = '\0';

    i = 0;
    while(entered_password[i] != '\0' && entered_password[i] != '\n')
    {
        password[i] = entered_password[i];
        i += 1;
    }
    password[i] = '\0';
}

void *client_handler(void *a )
{
    int clientfd = *((int *)a);

    char *username;
    char *password;

    username = (char *) malloc(140);
    password = (char *) malloc(140);

    login_prompt(username, password, clientfd);
    printf("%s\n%s\n", username, password);

    // while(1)
    // {
    //     char buffer[100];

    //     int client_message = read(clientfd, buffer, sizeof(buffer));
        
    //     if (client_message == 0)
    //     {
    //         printf("\nClient has disconnected abruptly.\n");
    //         pthread_exit(NULL);
    //     }
    //     printf("Message from Client: %s\n\n", buffer);

    //     char response[100];

    //     printf("Enter response to client: \n");
    //     scanf(" %[^\n]", response);
    //     write(clientfd, response, sizeof(response));
    // }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    printf("SERVER...\n");
    
    int sockfd;
    int clientfd;
    int port_number;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t address_size;

    address_size = sizeof(client_address);
    
    if(argc < 2)
    {
        printf("Insufficient Number of arguments\nPlease enter a port number!\n");
        exit(0);
    }

    
    port_number = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM,0);
    if(sockfd == -1)
    {
        perror("Socket Error\n");
        exit(0);
    }

    memset((void *)&server_address, 0, sizeof(server_address));
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = INADDR_ANY;                            

    
    int b = bind(sockfd, (struct sockaddr*)(&server_address), sizeof(server_address));
    if(b == -1)
    {
        perror("Bind Error\n");
        exit(EXIT_FAILURE);
    }

    
    listen(sockfd,5);
    printf("Listening for clients\n");

    pthread_t thread_ids[10];
    
    int i = 0;
    while(1)
    {
        memset(&client_address, 0 , sizeof(client_address));

        int a = accept(sockfd, (struct sockaddr*)(&client_address), &address_size);
        if(a == -1)
        {
            perror("Accept Error\n");
            exit(0);
        }

        printf("Connected to client: %s\n", inet_ntoa(client_address.sin_addr));

        if( pthread_create( &thread_ids[i++], NULL, client_handler, &a) != 0 )
            perror("Could not create thread.\n ");

        if( i >= 5 )
        {
            i = 0;
            while (i < 5)
            {
                pthread_join(thread_ids[i++], NULL);
            }
            i = 0;  
        }
        
    }
    return 0;
}

