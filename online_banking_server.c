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

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
// #include <netinet/ip.h>
// Send and Receive protocols to take care of the message boundary problem.
// Refer to "https://www.codeproject.com/Articles/11922/Solution-for-TCP-IP-client-socket-message-boundary"
// to learn more about the message boundary problem.
void send_to_client(int clientfd, char *message)
{
    int size_header = (strlen(message) - 1) / SEND_USERBUFFER_SIZE + 1;
    int send_status = write(clientfd, &size_header, sizeof(int));

    char *send_message = (char *)malloc(size_header * SEND_USERBUFFER_SIZE);
    strcpy(send_message, message);

    for (int i = 0; i < size_header; i++)
    {
        int send_status = write(clientfd, send_message, SEND_USERBUFFER_SIZE);
        send_message += SEND_USERBUFFER_SIZE;
    }
}

char *receive_from_client(int clientfd)
{
    int size_header = 0;
    // printf("%d\n", clientfd);
    // printf("%ls\n", &size_header);
    int receive_status = read(clientfd, &size_header, sizeof(int));
    // printf("%d\n", receive_status);
    if (receive_status <= 0)
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

int deposit_withdraw_handler(int account_id, int operation, int amount)
{
    int acc_id = account_id;
    int op = operation;
    int amt = amount;

    int fd1;
    int fd2;
    char* buffer = (char *)malloc(140);

    // pthread_mutex_lock(&mutex1);
    
    fd1 = open("AccountsInformation.txt", O_RDONLY, 0);
    printf("fd1 : %d\n", fd1);
    if (fd1 < 0)
    {
        perror("Could not open the file containing accounts information.\n");
        // return NULL;
    }

    fd2 = open("AccountsInformationDuplicate.txt", O_WRONLY | O_CREAT);
    printf("fd2 : %d\n", fd2);
    if (fd2 < 0)
    {
        perror("Could not create duplicate file containing accounts information.\n");
        // return NULL;
    }

    int i = 0;
    while (read(fd1, &buffer[i], 1) == 1)
    {
        if (buffer[i] == '\n' || buffer[i] == 0x0)
        {
            buffer[i] = 0;
            printf("%s\n", buffer);
            char buffer_copy[140];
            // printf("YOLO\n");
            char *token = strtok(buffer, " "); //account ID
            strcpy(buffer_copy, token);
            strcat(buffer_copy, " ");
            // printf("YOLO\n");
            int read_accid = atoi(token);
            token = strtok(NULL, " ");
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");  //First Name
            token = strtok(NULL, " ");
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");  //Last Name
            token = strtok(NULL, " ");
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");  //Account Type
            token = strtok(NULL, " ");
            // printf("%s\n", buffer_copy);
            

            if(read_accid == acc_id)
            {
                
                int read_amt = atoi(token);
                int new_amt;
                
                if (op == 1)
                {
                    new_amt = read_amt + amt;
                }
                else
                {
                    if((read_amt - amt) > 0)
                    {
                        new_amt = read_amt - amt;
                    }
                    else
                    {
                        return -1;
                        // send_to_client(clientfd, "Insufficient Balance!\n");
                    }
                    
                }
                
                printf("New amt : %d\n", new_amt);
                char* new_amt_str = (char *)malloc(140);
                
                sprintf(new_amt_str, "%d", new_amt);
                
                strncpy(token, new_amt_str, strlen(new_amt_str)); 
                int n = strlen(token) - strlen(new_amt_str);
                printf("Sub : %d\n", n);
                if(n > 0)
                {
                    for (int i = 0; i < n; i++)
                    {
                        token[strlen(new_amt_str) + i] = '\0';
                    }
                }
            }
            strcat(buffer_copy, token); //Amount
            strcat(buffer_copy, "\n"); 
            // printf("Token : %s\n", token);
            printf("Buffer : %s\n", buffer_copy);
            int w = write(fd2, buffer_copy, strlen(buffer_copy));
            printf("Write : %d\n", w);

            i = 0;
            continue;
        }

        i += 1;
    }
    printf("WTF\n");
    close(fd1);
    close(fd2);
    int rem = remove("AccountsInformation.txt");
    printf("Remove : %d\n", rem);
    int ren = rename("AccountsInformationDuplicate.txt", "AccountsInformation.txt");
    printf("Rename : %d\n", ren);
    printf("WTAF\n");
    return 0;
    // pthread_mutex_unlock(&mutex1);
}

void enquiry_handler(int account_id, int operation)
{

}

int request_admin_password_change(char *username, char *new_password)
{

}

void user_handler(char *username, char* password, int clientfd)
{   
    int track_session = 1;
    send_to_client(clientfd, "Do you want to:\n1. Deposit\n2. Withdraw\n3. Balance Enquiry\n4. Password Change\n5. View Details\n6. Exit\nPlease enter number of your choice: \n\n");

    char *response = (char *)malloc(140);
    while(track_session == 1)
    {
        int choice;
        response = receive_from_client(clientfd);
        choice = atoi(response);

        int account_id;
        int operation;
        int amount;
        char *acc_id = (char *)malloc(140);
        char *amt = (char *)malloc(140);

        char *old_password = (char *)malloc(140);
        char *new_password = (char *)malloc(140);
        char *confirm_password = (char *)malloc(140);

        int ret;
        switch (choice)
        {
            case 1:
                operation = 1;
                
                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);

                send_to_client(clientfd, "Enter amount to deposit: \n");
                amt = receive_from_client(clientfd);
                amount = atoi(amt);

                ret = deposit_withdraw_handler(account_id, operation, amount);
                if(ret == 0)
                {
                    send_to_client(clientfd, "Money Deposited Successfully\n");
                }
                else
                {
                    send_to_client(clientfd, "Transaction Failed\n");
                }
                
                break;
        
            case 2:
                operation = 2;

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);

                send_to_client(clientfd, "Enter amount to withdraw: \n");
                amt = receive_from_client(clientfd);
                amount = atoi(amt);

                ret = deposit_withdraw_handler(account_id, operation, amount);
                if (ret == -1)
                {
                    send_to_client(clientfd, "Transaction Failed. Insufficient Balance\n");
                }
                else
                {
                    send_to_client(clientfd, "Money Withdrawn Successfully\n");
                }
                
                break;

            case 3:
                operation = 3;
                

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);

                
                enquiry_handler(account_id, operation);
                break;

            case 4:

                send_to_client(clientfd, "Enter old password: \n");
                old_password = receive_from_client(clientfd);

                if(strcmp(old_password, password) == 0)
                {
                
                
                    send_to_client(clientfd, "Enter new password: \n");
                    new_password = receive_from_client(clientfd);

                    send_to_client(clientfd, "Confirm new password: \n");
                    confirm_password = receive_from_client(clientfd);
                    
                    if(strcmp(new_password, confirm_password) == 0)
                    {
                        request_admin_password_change(username, new_password);

                    }
                    else
                    {
                        printf("Incorrect confirmation password.\n");
                    }
                    
                }
                else
                {
                    printf("The password you have entered is incorrect.\n");
                }
                
                break;

            case 5:
                operation = 5;

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);

                enquiry_handler(account_id, operation);
                break;

            case 6:
                // int account_id;
                // int operation = 3;

                // send_to_client(clientfd, "Enter your account number: \n");
                // char *acc_id = (char *)malloc(140);
                // acc_id = receive_from_client(clientfd);
                // account_id = atoi(acc_id);

                // enquiry_handler(account_id, operation);
                break;
        }
    }
}

void admin_user_handler(char *username, char *password, int clientfd)
{

}

int* authenticate(char *username, char *password)
{
    int fd;
    int user_type;
    char *buffer = (char *)malloc(140);
    static int auth_arr[3];
    
    fd = open("LoginInformation.txt", O_RDONLY, 0);
    if (fd <= 0)
    {
        perror("Could not open the file containing login information.\n");
        return NULL;
    }

    int found_username = 0;
    int correct_password = 0;
    int undefined_usertype = 0;

    int i = 0;
    while(read(fd, &buffer[i], 1) == 1)
    {
        if(buffer[i] == '\n' || buffer[i] == 0x0 )
        {
            buffer[i] = 0;

            char *token = strtok(buffer, " ");
            // printf("%s\n", token);
            if(strcmp(token, username) == 0)
            {
                found_username = 1;
                token = strtok(NULL, " ");
                if(strcmp(token, password) == 0)
                {
                    correct_password = 1;
                    token = strtok(NULL, " ");
                    if(!strncmp(token, "Normal", strlen("Normal")))
                    {
                        user_type = 0;
                        break;
                    }

                    else if (!strncmp(token, "Joint", strlen("Joint")))
                    {
                        user_type = 1;
                        printf("%s\n", token);
                        printf("%d\n", user_type);
                        break;
                    }

                    else if (!strncmp(token, "Admin", strlen("Admin")))
                    {
                        user_type = 2;
                        break;
                    }

                    else
                    {
                        user_type = 3;
                        break;
                    }
                }
            
                else
                {
                    correct_password = -1;
                    break;
                }
            }

            else
            {
                found_username = 0;
                
            }
            
            i = 0;
            continue;
        }

        i += 1;

    }

    if (found_username == 0)
    {
        found_username = -1;
    }
    
    auth_arr[0] = found_username;
    auth_arr[1] = correct_password;
    auth_arr[2] = user_type;
    
    return auth_arr;
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

    int* auth_values;

    login_prompt(username, password, clientfd);
    // printf("%s\n%s\n", username, password);
    auth_values = authenticate(username, password);

    if(auth_values[0] == -1)
    {
        printf("No such user exists!\n");
        pthread_exit(NULL);
    } 

    if(auth_values[1] == -1)
    {
        printf("Incorrect password entered!\n");
        pthread_exit(NULL);
    }

    switch(auth_values[2])
    {
        case 0:
            printf("Normal User\n");
            user_handler(username, password, clientfd);
            break;

        case 1:
            printf("Joint User\n");
            user_handler(username, password, clientfd);
            break;

        case 2:
            printf("Admin User\n");
            admin_user_handler(username, password, clientfd);
            break;

        case 3:
            printf("You are unauthorized!\n");
            break;
    }
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

    pthread_mutex_init(&mutex1, NULL);
    // pthread_mutex_init(&mutex2, NULL);

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

