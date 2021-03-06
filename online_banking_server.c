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
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SEND_USERBUFFER_SIZE 1024
#define RECV_USERBUFFER_SIZE 1024

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

// #include <netinet/ip.h>

// SIGINT handler.
void signal_handler()
{
    printf("\nServer Shut Down.\n");
    exit(1);
}
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

void close_connection(int clientfd, char *closing_message)
{
    send_to_client(clientfd, closing_message);
    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
}

int deposit_withdraw_handler(int account_id, int operation, int amount)
{
    int acc_id = account_id;
    int op = operation;
    int amt = amount;

    int fd1;
    int fd2;
    char* buffer = (char *)malloc(140);

    int retval = 0;

    pthread_mutex_lock(&mutex1);
    
    fd1 = open("AccountsInformation.txt", O_RDONLY, 0);
    // printf("fd1 : %d\n", fd1);
    if (fd1 < 0)
    {
        perror("Could not open the file containing accounts information.\n");
        // return NULL;
    }

    fd2 = open("AccountsInformationDuplicate.txt", O_WRONLY | O_CREAT, 0777);
    // printf("fd2 : %d\n", fd2);
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
            // printf("%s\n", buffer);
            char buffer_copy[140];
            // printf("YOLO\n");
            char *token = strtok(buffer, " "); //Account ID
            strcpy(buffer_copy, token);
            strcat(buffer_copy, " ");
            // printf("YOLO\n");
            int read_accid = atoi(token);
            token = strtok(NULL, " "); //First Name
            strcat(buffer_copy, token); 
            strcat(buffer_copy, " ");
            token = strtok(NULL, " "); //Last Name
            strcat(buffer_copy, token); 
            strcat(buffer_copy, " ");
            token = strtok(NULL, " "); //Account Type
            strcat(buffer_copy, token); 
            strcat(buffer_copy, " ");  
            token = strtok(NULL, " "); //Amount
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
                    if((read_amt - amt) >= 0)
                    {
                        new_amt = read_amt - amt;
                    }
                    else
                    {
                        retval = -1;
                        new_amt = read_amt;
                        // send_to_client(clientfd, "Insufficient Balance!\n");
                    }
                    
                }
                
                // printf("New amt : %d\n", new_amt);
                char* new_amt_str = (char *)malloc(140);
                
                sprintf(new_amt_str, "%d", new_amt);
                
                strncpy(token, new_amt_str, strlen(new_amt_str)); 
                int n = strlen(token) - strlen(new_amt_str);
                // printf("Sub : %d\n", n);
                if(n > 0)
                {
                    for (int i = 0; i < n; i++)
                    {
                        token[strlen(new_amt_str) + i] = '\0';
                    }
                }
            }
            strcat(buffer_copy, token); 
            strcat(buffer_copy, "\n"); 
            // printf("Token : %s\n", token);
            // printf("Buffer : %s\n", buffer_copy);
            int w = write(fd2, buffer_copy, strlen(buffer_copy));
            // printf("Write : %d\n", w);

            i = 0;
            continue;
        }

        i += 1;
    }
    // printf("WTF\n");
    close(fd1);
    close(fd2);
    
    int rem = remove("AccountsInformation.txt");
    // printf("Remove : %d\n", rem);
    int ren = rename("AccountsInformationDuplicate.txt", "AccountsInformation.txt");
    // printf("Rename : %d\n", ren);
    // printf("WTAF\n");
    pthread_mutex_unlock(&mutex1);
    
    return retval;
}

int check_balance(int account_id)
{
    int retval = -1;
    char* buffer = (char *)malloc(140);
    
    // pthread_mutex_lock(&mutex1);
    
    int fd = open("AccountsInformation.txt", O_RDONLY, 0);
    // printf("fd1 : %d\n", fd1);
    if (fd < 0)
    {
        perror("Could not open the file containing accounts information.\n");
        // return NULL;
    }

    // printf("fd : %d\n", fd);
    int i = 0;
    while (read(fd, &buffer[i], 1) == 1)
    {   
        // printf("%s\n", buffer[i]);
        if (buffer[i] == '\n' || buffer[i] == 0x0)
        {
            buffer[i] = 0;
            // printf("%s\n", buffer);
            char buffer_copy[140];
            // printf("YOLO\n");
            char *token = strtok(buffer, " "); //account ID
            strcpy(buffer_copy, token);
            strcat(buffer_copy, " ");
            // printf("YOLO\n");
            int read_accid = atoi(token);
            token = strtok(NULL, " "); //First Name
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");
            token = strtok(NULL, " "); //Last Name
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");
            token = strtok(NULL, " "); //Account Type
            strcat(buffer_copy, token);
            strcat(buffer_copy, " ");
            token = strtok(NULL, " "); //Amount
            // printf("%s\n", buffer_copy);
        
        
            if (read_accid == account_id)
            {

                int read_amt = atoi(token);
                retval = read_amt;
            }
            
            i = 0;
            continue;
        
        }

        i += 1;
        
    }

    // pthread_mutex_unlock(&mutex1);
    return retval;
}

char * search_account_handler(char *query)
{
    char *send_response  = (char *) malloc(140);
    char *account_buffer = (char *) malloc(140);
    char *account_buffer_copy = (char *) malloc(140);
    char *not_found_response = (char *)malloc(140);
    int found = 0;

    strcpy(send_response, "|| Account Number || First Name || Last Name || Account Type || Balance ||\n\n");
    strcpy(not_found_response, "The entered account number does not exist in the records.\n");
    
    pthread_mutex_lock(&mutex1);

    int fd1 = open("AccountsInformation.txt", O_RDONLY, 0);
    if( fd1 < 0)
    {
        perror("Could not open file containing accounts information.\n");
    }

    int i = 0;
    while(read(fd1, &account_buffer[i], 1) == 1)
    {
        if (account_buffer[i] == '\n' || account_buffer[i] == 0x0)
        {
            account_buffer[i] = 0;
            strcpy(account_buffer_copy, account_buffer);
            
            char * token = strtok(account_buffer, " ");
            if(strcmp(token, query) == 0)
            {
                found += 1;
                strcat(send_response, account_buffer_copy);
                strcat(send_response, "\n");
            }
            
            i = 0;
            continue;

        }

        i += 1;
    }

    pthread_mutex_unlock(&mutex1);
    if(found == 0)
    {
        return not_found_response;
    }
    else if(found == 1)
    {
        return send_response;
    }
    else
    {
        strcat(send_response, "\nThe above are the details of the joint account holders\n\n");
        return send_response;
    }
    
}

void modify_account_handler(char *query)
{
    char *buffer_login = (char *)malloc(140);
    char *buffer_response_login = (char *)malloc(140);
    char *buffer_copy_login = (char *)malloc(140);
    char *token_query_username = (char *)malloc(140);

    char *token_query = strtok(query, " "); //Modify
    token_query = strtok(NULL, " "); //Username
    strcpy(token_query_username, token_query);
    token_query = strtok(NULL, " "); // Previous Password
    token_query = strtok(NULL, " "); // New Password

    pthread_mutex_lock(&mutex2);

    int fd3 = open("LoginInformation.txt", O_RDONLY, 0);
    // printf("fd3 : %d\n", fd3);
    if (fd3 < 0)
    {
        perror("Could not open the file containing login information.\n");
        // return NULL;
    }

    int fd4 = open("LoginInformationDuplicate.txt", O_WRONLY | O_CREAT, 0777);
    // printf("fd4 : %d\n", fd4);
    if (fd4 < 0)
    {
        perror("Could not open the file containing duplicate login information.\n");
        // return NULL;
    }

    int i = 0;
    while (read(fd3, &buffer_login[i], 1) == 1)
    {
        if (buffer_login[i] == '\n' || buffer_login[i] == 0x0)
        {
            buffer_login[i] = 0;
            strcpy(buffer_copy_login, buffer_login);
            strcat(buffer_copy_login, "\n");

            char *token_login = strtok(buffer_login, " "); //Username
            strcpy(buffer_response_login, token_login);
            strcat(buffer_response_login, " ");

            
            // printf("Token comp : %d\n", strcmp(token_login, token_query_username));
            
            if(strcmp(token_login, token_query_username) == 0)
            {   
                // printf("Inside\n");
                // printf("Token login : %s\n", token_login);
                // printf("Token Query : %s\n", token_query);
                // printf("Should be new pass : %s\n", token_query);

                strcat(buffer_response_login, token_query); //New Password
                strcat(buffer_response_login, " ");
                // printf("Resp after new pass : %s\n", buffer_response_login);

                token_login = strtok(NULL, " "); //Password

                token_login = strtok(NULL, " "); //User Type
                strcat(buffer_response_login, token_login);
                strcat(buffer_response_login, " ");

                token_login = strtok(NULL, " "); //Account Number
                strcat(buffer_response_login, token_login);
                strcat(buffer_response_login, "\n");

                // printf("%s\n", buffer_response_login);

                write(fd4, buffer_response_login, strlen(buffer_response_login));
            }
            else
            {
                // printf("%s\n", buffer_copy_login);
                write(fd4, buffer_copy_login, strlen(buffer_copy_login));
            }
            
            i = 0;
            continue;
        }

        i += 1;
    }

    close(fd3);
    close(fd4);

    int rem = remove("LoginInformation.txt");
    // printf("Remove : %d\n", rem);
    int ren = rename("LoginInformationDuplicate.txt", "LoginInformation.txt");
    // printf("Rename : %d\n", ren);
    pthread_mutex_unlock(&mutex2);
}

void delete_account_handler(char *query)
{
    char *buffer_login = (char *)malloc(140);
    char *buffer_account = (char *)malloc(140);

    char *buffer_copy_login = (char *)malloc(140);
    char *buffer_copy_account = (char *)malloc(140);

    char *token_query = strtok(query, " ");

    token_query = strtok(NULL, " "); //Account Number
    int account_id = atoi(token_query);

    pthread_mutex_lock(&mutex3);

    int fd1 = open("AccountsInformation.txt", O_RDONLY, 0);
    // printf("fd1 : %d\n", fd1);
    if (fd1 < 0)
    {
        perror("Could not open the file containing accounts information.\n");
        // return NULL;
    }

    int fd2 = open("AccountsInformationDuplicate.txt", O_WRONLY | O_CREAT, 0777);
    // printf("fd1 : %d\n", fd1);
    if (fd2 < 0)
    {
        perror("Could not open the file containing duplicate accounts information.\n");
        // return NULL;
    }

    int i = 0;
    while (read(fd1, &buffer_account[i], 1) == 1)
    {
        // printf("%s\n", buffer[i]);
        if (buffer_account[i] == '\n' || buffer_account[i] == 0x0)
        {
            buffer_account[i] = 0;
            strcpy(buffer_copy_account, buffer_account);
            strcat(buffer_copy_account, "\n");
            // printf("%s\n", buffer);
            // char buffer_copy[140];
            // printf("YOLO\n");
            
            char *token_account = strtok(buffer_account, " "); //Account ID
            int read_accid = atoi(token_account);

            
            if (read_accid != account_id)
            {
                write(fd2, buffer_copy_account, strlen(buffer_copy_account));
            }

            i = 0;
            continue;
        }

        i += 1;
    }

    close(fd1);
    close(fd2);

    int rem = remove("AccountsInformation.txt");
    // printf("Remove : %d\n", rem);
    int ren = rename("AccountsInformationDuplicate.txt", "AccountsInformation.txt");
    // printf("Rename : %d\n", ren);

    pthread_mutex_unlock(&mutex3);
    
    // printf("Before mutex2\n");
    
    pthread_mutex_lock(&mutex2);

    int fd3 = open("LoginInformation.txt", O_RDONLY, 0);
    // printf("fd3 : %d\n", fd3);
    if (fd3 < 0)
    {
        perror("Could not open the file containing login information.\n");
        // return NULL;
    }

    int fd4 = open("LoginInformationDuplicate.txt", O_WRONLY | O_CREAT, 0777);
    // printf("fd4 : %d\n", fd4);
    if (fd4 < 0)
    {
        perror("Could not open the file containing duplicate login information.\n");
        // return NULL;
    }

    i = 0;
    while (read(fd3, &buffer_login[i], 1) == 1)
    {
        if (buffer_login[i] == '\n' || buffer_login[i] == 0x0)
        {
            buffer_login[i] = 0;
            strcpy(buffer_copy_login, buffer_login);
            strcat(buffer_copy_login, "\n");

            char *token_login = strtok(buffer_login, " "); //Username
            token_login = strtok(NULL, " "); //Password
            token_login = strtok(NULL, " "); //User Type
            token_login = strtok(NULL, " "); //Account Number
            int accid = atoi(token_login);

            if(accid != account_id)
            {   
                    write(fd4, buffer_copy_login , strlen(buffer_copy_login));
            }

            i = 0;
            continue;
        }

        i += 1;
    }
    
    // printf("Begore fd3 close\n");
    close(fd3);
    // printf("After fd3 close\n");
    close(fd4);
    // printf("Begore fd4 close\n");

    int rem1 = remove("LoginInformation.txt");
    // printf("After remove : %d\n", rem1);
    // printf("Remove : %d\n", rem);
    int ren1 = rename("LoginInformationDuplicate.txt", "LoginInformation.txt");
    // printf("After rename : %d\n", ren1);
    // printf("Rename : %d\n", ren);
    pthread_mutex_unlock(&mutex2);

}

void add_account_handler(char *query)
{   
    int acc_number;
    int amount;
    
    char * buffer_copy_login = (char *) malloc(140);
    char * buffer_copy_account = (char *) malloc(140);

    char *token = strtok(query," ");

    token = strtok(NULL, " "); //Username
    strcat(buffer_copy_login, token); 
    strcat(buffer_copy_login, " ");

    token = strtok(NULL, " ");
    strcat(buffer_copy_login, token); //Password
    strcat(buffer_copy_login, " ");

    token = strtok(NULL, " ");
    strcat(buffer_copy_login, token); //User Type
    strcat(buffer_copy_login, " ");

    token = strtok(NULL, " ");
    acc_number = atoi(token);
    strcat(buffer_copy_login, token); //Account Number
    strcat(buffer_copy_login, "\n");

    strcat(buffer_copy_account, token);
    strcat(buffer_copy_account, " ");

    pthread_mutex_lock(&mutex2);

    int fd1 = open("LoginInformation.txt", O_WRONLY, 0);
    // printf("fd1 : %d\n", fd1);
    if (fd1 < 0)
    {
        perror("Could not open the file containing login information.\n");
        // return NULL;
    }

    lseek(fd1, 0L, SEEK_END);
    write(fd1, buffer_copy_login, strlen(buffer_copy_login));

    pthread_mutex_unlock(&mutex2);

    token = strtok(NULL, " "); //First Name
    strcat(buffer_copy_account, token);
    strcat(buffer_copy_account, " ");

    token = strtok(NULL, " "); //Last Name
    strcat(buffer_copy_account, token);
    strcat(buffer_copy_account, " ");

    token = strtok(NULL, " "); //Account Type
    strcat(buffer_copy_account, token);
    strcat(buffer_copy_account, " ");

    token = strtok(NULL, " "); //Initial Deposit
    amount = atoi(token);
    strcat(buffer_copy_account, token);
    strcat(buffer_copy_account, "\n");

    pthread_mutex_lock(&mutex3);

    int fd2 = open("AccountsInformation.txt", O_WRONLY, 0);
    // printf("fd1 : %d\n", fd1);
    if (fd2 < 0)
    {
        perror("Could not open the file containing accounts information.\n");
        // return NULL;
    }

    lseek(fd2, 0L, SEEK_END);
    write(fd2, buffer_copy_account, strlen(buffer_copy_account));

    pthread_mutex_unlock(&mutex3);
    // deposit_withdraw_handler(acc_number, 1, amount);
}

int individual_handler(int request_type)
{   
    int fd1;
    int fd2;
    int i;
    char * buffer = (char *) malloc(140);

    int rem;
    int ren;

    int found = 0;
    int retval;

    switch(request_type)
    {
        case 1:
            
            pthread_mutex_lock(&mutex1);

            fd1 = open("AdminRequests.txt", O_RDONLY, 0);
            // printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT, 0777);
            // printf("fd2 : %d\n", fd2);
            if (fd2 < 0)
            {
                perror("Could not create duplicate file containing query requests to Admin.\n");
                // return NULL;
            }

            i = 0;
            while (read(fd1, &buffer[i], 1) == 1)
            {
                if (buffer[i] == '\n' || buffer[i] == 0x0)
                {
                    buffer[i] = 0;
                    
                    if (buffer[0] == 'A')
                    {
                        found += 1;
                        add_account_handler(buffer);
                    }
                    else
                    {
                        strcat(buffer,"\n");
                        write(fd2, buffer, strlen(buffer));
                    }
                    
                    i = 0;
                    continue;
                }
                i+=1;
            }

            if (found == 0)
            {
                retval = 0;
            }
            else
            {
                retval = 1;
            }
            
            close(fd1);
            close(fd2);

            rem = remove("AdminRequests.txt");
            // printf("Remove : %d\n", rem);
            ren = rename("AdminRequestsDuplicate.txt", "AdminRequests.txt");
            // printf("Rename : %d\n", ren);
            // printf("WTAF\n");
            pthread_mutex_unlock(&mutex1);
            break;
    
        case 2:
            pthread_mutex_lock(&mutex1);

            fd1 = open("AdminRequests.txt", O_RDONLY, 0);
            // printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT, 0777);
            // printf("fd2 : %d\n", fd2);
            if (fd2 < 0)
            {
                perror("Could not create duplicate file containing query requests to Admin.\n");
                // return NULL;
            }

            i = 0;
            while (read(fd1, &buffer[i], 1) == 1)
            {
                if (buffer[i] == '\n' || buffer[i] == 0x0)
                {
                    buffer[i] = 0;

                    if (buffer[0] == 'D')
                    {
                        found += 1;
                        delete_account_handler(buffer);
                    }
                    else
                    {
                        strcat(buffer, "\n");
                        write(fd2, buffer, strlen(buffer));
                    }

                    i = 0;
                    continue;
                }
                i+=1;
            }

            if (found == 0)
            {
                retval = 0;
            }
            else
            {
                retval = 1;
            }
            
            close(fd1);
            close(fd2);

            // printf("WTF\n");
            rem = remove("AdminRequests.txt");
            // printf("Remove : %d\n", rem);
            ren = rename("AdminRequestsDuplicate.txt", "AdminRequests.txt");
            // printf("Rename : %d\n", ren);
            // printf("WTAF\n");
            pthread_mutex_unlock(&mutex1);
            break;

        case 3:
            pthread_mutex_lock(&mutex1);

            fd1 = open("AdminRequests.txt", O_RDONLY, 0);
            // printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT, 0777);
            // printf("fd2 : %d\n", fd2);
            if (fd2 < 0)
            {
                perror("Could not create duplicate file containing query requests to Admin.\n");
                // return NULL;
            }

            i = 0;
            while (read(fd1, &buffer[i], 1) == 1)
            {
                if (buffer[i] == '\n' || buffer[i] == 0x0)
                {
                    buffer[i] = 0;

                    if (buffer[0] == 'M')
                    {
                        found += 1;
                        modify_account_handler(buffer);
                    }
                    else
                    {
                        strcat(buffer, "\n");
                        write(fd2, buffer, strlen(buffer));
                    }

                    i = 0;
                    continue;
                }
                i+=1;
            }
            
            if (found == 0)
            {
                retval = 0;
            }
            else
            {
                retval = 1;
            }
            close(fd1);
            close(fd2);

            rem = remove("AdminRequests.txt");
            // printf("Remove : %d\n", rem);
            ren = rename("AdminRequestsDuplicate.txt", "AdminRequests.txt");
            // printf("Rename : %d\n", ren);
            // printf("WTAF\n");
            pthread_mutex_unlock(&mutex1);
            break;

        case 4:
            pthread_mutex_lock(&mutex1);

            fd1 = open("AdminRequests.txt", O_RDONLY, 0);
            // printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT, 0777);
            // printf("fd2 : %d\n", fd2);
            if (fd2 < 0)
            {
                perror("Could not create duplicate file containing query requests to Admin.\n");
                // return NULL;
            }

            i = 0;
            while (read(fd1, &buffer[i], 1) == 1)
            {
                if (buffer[i] == '\n' || buffer[i] == 0x0)
                {
                    buffer[i] = 0;

                    if (buffer[0] == 'S')
                    {
                        found += 1;
                        search_account_handler(buffer);
                    }
                    else
                    {
                        strcat(buffer, "\n");
                        write(fd2, buffer, strlen(buffer));
                    }

                    i = 0;
                    continue;
                }
                i+=1;
            }
            
            if (found == 0)
            {
                retval = 0;
            }
            else
            {
                retval = 1;
            }
            close(fd1);
            close(fd2);

            rem = remove("AdminRequests.txt");
            // printf("Remove : %d\n", rem);
            ren = rename("AdminRequestsDuplicate.txt", "AdminRequests.txt");
            // printf("Rename : %d\n", ren);
            // printf("WTAF\n");
            pthread_mutex_unlock(&mutex1);
            break;
    }

    return retval;
}

void admin_handler(char *username, char *password, int unique_account_id, int clientfd)
{
    int track_session = 1;
    int retval;
    send_to_client(clientfd, "Do you want to:\n1. Execute Pending Add Queries\n2. Execute Pending Delete Queries\n3. Execute Pending Modify Queries\n4. Search for a specific account details\n5. Execute All Queries\n6. Exit\nPlease enter number of your choice: \n\n");

    char *response = (char *)malloc(140);
    
    char *acc_id = (char *)malloc(140);
    char *search_response = (char *)malloc(140);
    char *combined_response = (char *)malloc(140);
    
    while (track_session == 1)
    {
        int choice;
        response = receive_from_client(clientfd);
        if (response == NULL)
        {
            printf("Client has disconnected abruptly.\n\n");
            pthread_exit(NULL);
        }

        choice = atoi(response);

        switch (choice)
        {
            case 1:
            case 2:
            case 3:
                retval = individual_handler(choice);
                if(retval == 1)
                {
                    send_to_client(clientfd, "Queries successfully executed\nIf you wish to perform some other operation , please enter the option number again : \n\n");
                }
                else
                {
                    if(choice == 1)
                    {
                        send_to_client(clientfd, "No pending Add queries\nIf you wish to perform some other operation , please enter the option number again : \n\n");
                    }
                    
                    else if (choice == 2)
                    {
                        send_to_client(clientfd, "No pending Delete queries\nIf you wish to perform some other operation , please enter the option number again : \n\n");
                    }

                    else 
                    {
                        send_to_client(clientfd, "No pending Modify queries\nIf you wish to perform some other operation , please enter the option number again : \n\n");
                    }

                    // else
                    // {
                    //     send_to_client(clientfd, "No pending Search queries\n");
                    // }
                }
                
                break;
            case 4:
                send_to_client(clientfd, "Enter the account number whose details you want to know: \n");
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                search_response = search_account_handler(acc_id);
                strcat(search_response, "\nIf you wish to perform some other operation , please enter the option number again : \n\n");
                    send_to_client(clientfd, search_response);
                break;
                
            case 5:
                retval = individual_handler(1);
                if(retval == 1)
                {
                    strcpy(combined_response, "All Add Queries Executed\n");
                }
                else
                {
                    strcpy(combined_response, "No Add Queries...\n");
                }
                
                retval = individual_handler(2);
                if (retval == 1)
                {
                    strcat(combined_response, "All Delete Queries Executed\n");
                }
                else
                {
                    strcat(combined_response, "No Delete Queries...\n");
                }
                
                retval = individual_handler(3);
                if (retval == 1)
                {
                    strcat(combined_response, "All Modify Queries Executed\n");
                }
                else
                {
                    strcat(combined_response, "No Modify Queries...\n");
                }
                strcat(combined_response, "\nAll queries have been executed.\nIf you wish to perform some other operation , please enter the option number again : \n\n");

                // individual_handler(4);
                send_to_client(clientfd, combined_response);
                break;
            case 6:
                track_session = 0;
                break;
        }
    }
}

void user_handler(char *username, char* password, int unique_account_id, int clientfd)
{   
    int track_session = 1;
    send_to_client(clientfd, "Do you want to:\n1. Deposit\n2. Withdraw\n3. Balance Enquiry\n4. Password Change\n5. View Details\n6. Delete Account\n7. Exit\nPlease enter number of your choice: \n\n");

    char *response = (char *)malloc(140);
    while(track_session == 1)
    {
        int choice;
        response = receive_from_client(clientfd);
        if (response == NULL)
        {
            printf("Client has disconnected abruptly.\n\n");
            pthread_exit(NULL);
        }

        choice = atoi(response);

        int account_id;
        int operation;
        int amount;
        char *acc_id = (char *)malloc(140);
        char *amt = (char *)malloc(140);

        char *old_password = (char *)malloc(140);
        char *new_password = (char *)malloc(140);
        char *confirm_password = (char *)malloc(140);

        int deposit_withdraw_ret;
        int balance;
        char *balance_str = (char *)malloc(140);

        int query_fd;
        char *query_buff = (char *)malloc(140);

        char *search_response = (char *)malloc(140);
        switch (choice)
        {
            case 1:
                operation = 1;
                
                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                account_id = atoi(acc_id);
                if(account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\nPlease choose the operation to perform again : \n\n");
                    // pthread_exit(NULL);
                    break;  
                }

                send_to_client(clientfd, "Enter amount to deposit: \n");
                amt = receive_from_client(clientfd);
                if (amt == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                amount = atoi(amt);

                deposit_withdraw_ret = deposit_withdraw_handler(account_id, operation, amount);
                if(deposit_withdraw_ret == 0)
                {
                    send_to_client(clientfd, "Money Deposited Successfully\nIf you wish to perform any other operation, please enter the number of your choice again : \n");
                }
                else
                {
                    close_connection(clientfd, "Transaction Failed\nPress 'q' to quit.\n\n");
                    pthread_exit(NULL);
                }
                
                break;
        
            case 2:
                operation = 2;

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                account_id = atoi(acc_id);
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\nPlease choose the operation to perform again : \n\n");
                    // pthread_exit(NULL);
                    break;
                }

                send_to_client(clientfd, "Enter amount to withdraw: \n");
                amt = receive_from_client(clientfd);
                if (amt == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                amount = atoi(amt);

                deposit_withdraw_ret = deposit_withdraw_handler(account_id, operation, amount);
                if (deposit_withdraw_ret == -1)
                {
                    send_to_client(clientfd, "Transaction Failed. Insufficient Balance\nIf you wish to perform any other operation, please enter the number of your choice again : \n");
                }
                else
                {
                    send_to_client(clientfd, "Money Withdrawn Successfully\nIf you wish to perform any other operation, please enter the number of your choice again : \n");
                }
                
                break;

            case 3:
                operation = 3;
                

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                account_id = atoi(acc_id);
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\nPlease choose the operation to perform again : \n\n");
                    break;
                }

                balance = check_balance(account_id);

                if(balance == -1)
                {
                    close_connection(clientfd,"The account no longer exists!\nPress 'q' to quit.\n\n");
                    pthread_exit(NULL);
                }
                else
                {
                    sprintf(balance_str, "%d", balance);
                    strcat(balance_str, " is your balance\nIf you wish to perform any other operation, please enter the number of your choice again : \n");
                    send_to_client(clientfd, balance_str);
                }
                
                break;

            case 4:

                send_to_client(clientfd, "Enter old password: \n");
                old_password = receive_from_client(clientfd);
                if (old_password == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                if(strcmp(old_password, password) == 0)
                {
                
                
                    send_to_client(clientfd, "Enter new password: \n");
                    new_password = receive_from_client(clientfd);
                    if (new_password == NULL)
                    {
                        printf("Client has disconnected abruptly.\n\n");
                        pthread_exit(NULL);
                    }

                    send_to_client(clientfd, "Confirm new password: \n");
                    confirm_password = receive_from_client(clientfd);
                    if (confirm_password == NULL)
                    {
                        printf("Client has disconnected abruptly.\n\n");
                        pthread_exit(NULL);
                    }

                    if(strcmp(new_password, confirm_password) == 0)
                    {
                        pthread_mutex_lock(&mutex1);
                        query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT, 0777);
                        if(query_fd < 0)
                        {
                            printf("Could not open the file containing requests to Admin.\n");
                        }
                        strcpy(query_buff, "Modify ");
                        strcat(query_buff,username);
                        strcat(query_buff, " ");
                        strcat(query_buff, password);
                        strcat(query_buff, " ");
                        strcat(query_buff, new_password);
                        strcat(query_buff, "\n");
                        
                        lseek(query_fd, 0L, SEEK_END);
                        write(query_fd, query_buff, strlen(query_buff));
                        pthread_mutex_unlock(&mutex1);
                        close_connection(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\nWe expect your query to be reviewed within 1 business day.\nPress 'q' to quit.\n\n");
                        pthread_exit(NULL);
                        // track_session = 0;
                        break;
                    }
                    else
                    {
                        send_to_client(clientfd, "Incorrect confirmation password.\nPlease choose the 'Password Change' option and try again!\n");
                        break;
                    }
                    
                }
                else
                {
                    send_to_client(clientfd, "The password you have entered is incorrect.\nPlease choose the 'Password Change' option and try again!\n");
                }
                
                break;

            case 5:
                operation = 5;

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                account_id = atoi(acc_id);
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\nPlease rechoose the option and try again\n");
                    break;
                }
                else
                {
                    search_response = search_account_handler(acc_id);
                    strcat(search_response, "\nIf you wish to perform any other operation, please enter the number of your choice again : \n");
                    send_to_client(clientfd, search_response);
                }
                // pthread_mutex_lock(&mutex1);
                // query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT);
                // if (query_fd < 0)
                // {
                //     printf("Could not open the file containing requests to Admin.\n");
                // }
                // strcpy(query_buff, "Search ");
                // strcat(query_buff, acc_id);
                // strcat(query_buff, "\n");

                // lseek(query_fd, 0L, SEEK_END);
                // write(query_fd, query_buff, strlen(query_buff));
                // pthread_mutex_unlock(&mutex1);
                // send_to_client(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\n");
                break;

            case 6:
                operation = 6;

                send_to_client(clientfd, "Enter your account number: \n");
                char *acc_id = (char *)malloc(140);
                acc_id = receive_from_client(clientfd);
                if (acc_id == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                account_id = atoi(acc_id);
                
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\nPlease choose the operation to perform again : \n\n");
                    break;
                }

                pthread_mutex_lock(&mutex1);
                query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT, 0777);
                if (query_fd < 0)
                {
                    printf("Could not open the file containing requests to Admin.\n");
                }
                strcpy(query_buff, "Delete ");
                strcat(query_buff, acc_id);
                strcat(query_buff, "\n");

                lseek(query_fd, 0L, SEEK_END);
                write(query_fd, query_buff, strlen(query_buff));
                pthread_mutex_unlock(&mutex1);
                close_connection(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\nWe expect your query to be reviewed within 1 business day.\nPress 'q' to quit.\n\n");
                pthread_exit(NULL);
                // track_session = 0;
                break;

            case 7:
                track_session = 0;
                break;

                
        }
    }
}

int* authenticate(char *username, char *password)
{
    int fd;
    int user_type;
    int unique_account_id;
    char *buffer = (char *)malloc(140);
    static int auth_arr[4];
    
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
                        token = strtok(NULL, " ");
                        unique_account_id = atoi(token);
                        break;
                    }

                    else if (!strncmp(token, "Joint", strlen("Joint")))
                    {
                        user_type = 1;
                        token = strtok(NULL, " ");
                        unique_account_id = atoi(token);
                        break;
                    }

                    else if (!strncmp(token, "Admin", strlen("Admin")))
                    {
                        user_type = 2;
                        token = strtok(NULL, " ");
                        unique_account_id = atoi(token);
                        break;
                    }

                    else
                    {
                        user_type = 3;
                        token = strtok(NULL, " ");
                        unique_account_id = atoi(token);
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
    auth_arr[3] = unique_account_id;
    
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
    if(entered_username == NULL)
    {
        printf("Client has disconnected abruptly.\n\n");
        pthread_exit(NULL);
    }
    // printf("%s\n", entered_username);

    send_to_client(clientfd, "Enter Password : ");
    entered_password = receive_from_client(clientfd);
    if (entered_password == NULL)
    {
        printf("Client has disconnected abruptly.\n\n");
        pthread_exit(NULL);
    }
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
    int unique_account_id = auth_values[3];

    if(auth_values[0] == -1)
    {
        send_to_client(clientfd, "No such user exists!\n\nDo you wish to add a new account? (Y/N)\n");
        char * response = receive_from_client(clientfd);
        if (response == NULL)
        {
            printf("Client has disconnected abruptly.\n\n");
            pthread_exit(NULL);
        }

        if(strcmp(response, "Y") == 0)
        {
            char *new_username = (char *) malloc(140);
            char *new_password = (char *) malloc(140);
            char *new_user_type = (char *) malloc(140);
            char *new_acc_no = (char *) malloc(140);
            int new_acc_number;
            char *new_fname = (char *) malloc(140);
            char *new_lname = (char *) malloc(140);
            char *new_acc_type = (char *) malloc(140);
            char *new_initial_dep = (char *) malloc(140);

            send_to_client(clientfd, "Enter the previously entered username: \n");
            new_username = receive_from_client(clientfd);
            if (new_username == NULL)
            {
                printf("Client has disconnected abruptly.\n\n");
                pthread_exit(NULL);
            }

            send_to_client(clientfd, "Enter a password: \n");
            new_password = receive_from_client(clientfd);
            if (new_password == NULL)
            {
                printf("Client has disconnected abruptly.\n\n");
                pthread_exit(NULL);
            }

            send_to_client(clientfd, "Enter User type (Normal/Joint): \n");
            new_user_type = receive_from_client(clientfd);
            if (new_user_type == NULL)
            {
                printf("Client has disconnected abruptly.\n\n");
                pthread_exit(NULL);
            }

            if(strcmp(new_user_type,"Joint") == 0)
            {
                send_to_client(clientfd, "Details regarding the exisiting account...\n\nEnter the Joint Account Number: \n");
                new_acc_no = receive_from_client(clientfd);
                if (new_acc_no == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                send_to_client(clientfd, "Enter Joint Account Type (Savings/Current): \n");
                new_acc_type = receive_from_client(clientfd);
                if (new_acc_type == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                send_to_client(clientfd, "Enter Balance in the Joint Account: \n");
                new_initial_dep = receive_from_client(clientfd);
                if (new_initial_dep == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }
            }

            else
            {
                srand(time(NULL));
                new_acc_number = rand() % 10000 + 1;
                sprintf(new_acc_no, "%d", new_acc_number);
           
                send_to_client(clientfd, "Enter Account type (Savings/Current): \n");
                new_acc_type = receive_from_client(clientfd);
                if (new_acc_type == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }

                send_to_client(clientfd, "Enter Initial Deposit: \n");
                new_initial_dep = receive_from_client(clientfd);
                if (new_initial_dep == NULL)
                {
                    printf("Client has disconnected abruptly.\n\n");
                    pthread_exit(NULL);
                }
            }
            
            
            send_to_client(clientfd, "Enter your First Name: \n");
            new_fname = receive_from_client(clientfd);
            if (new_fname == NULL)
            {
                printf("Client has disconnected abruptly.\n\n");
                pthread_exit(NULL);
            }

            send_to_client(clientfd, "Enter your Last Name: \n");
            new_lname = receive_from_client(clientfd);
            if (new_lname == NULL)
            {
                printf("Client has disconnected abruptly.\n\n");
                pthread_exit(NULL);
            }

            // printf("Before lock\n");
            pthread_mutex_lock(&mutex1);
            // printf("After lock\n");
            int query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT, 0777);
            if (query_fd < 0)
            {
                printf("Could not open the file containing requests to Admin.\n");
            }
            // printf("%d\n", query_fd);
            char *query_buff = (char *)malloc(140);
            strcpy(query_buff, "Add ");
            strcat(query_buff, new_username);
            strcat(query_buff, " ");
            strcat(query_buff, new_password);
            strcat(query_buff, " ");
            strcat(query_buff, new_user_type);
            strcat(query_buff, " ");
            strcat(query_buff, new_acc_no);
            strcat(query_buff, " ");
            strcat(query_buff, new_fname);
            strcat(query_buff, " ");
            strcat(query_buff, new_lname);
            strcat(query_buff, " ");
            strcat(query_buff, new_acc_type);
            strcat(query_buff, " ");
            strcat(query_buff, new_initial_dep);
            strcat(query_buff, "\n");

            // printf("%s\n", query_buff);

            lseek(query_fd, 0L, SEEK_END);
            write(query_fd, query_buff, strlen(query_buff));
            pthread_mutex_unlock(&mutex1);

            strcat(new_acc_no, " is your unique account number for future transactions.\nYour account will be activated within 2 working days.\nThank you for opening an account with us!\nPress 'q' to quit.\n\n");
            close_connection(clientfd, new_acc_no);
        }
        else
        {
            close_connection(clientfd, "Thank you for visiting!\nPress 'q' to quit.\n\n");
        }
        pthread_exit(NULL);
    } 

    if(auth_values[1] == -1)
    {
        close_connection(clientfd, "Incorrect password entered!\nPress 'q' to quit\n\n");
        pthread_exit(NULL);
    }

    switch(auth_values[2])
    {
        case 0:
            // printf("Normal User\n");
            user_handler(username, password, unique_account_id, clientfd);
            close_connection(clientfd, "Thank you for visiting us!\nPlease do come back again.\nPress 'q' to quit.\n\n");
            break;

        case 1:
            // printf("Joint User\n");
            user_handler(username, password, unique_account_id, clientfd);
            close_connection(clientfd, "Thank you for visiting us!\nPlease do come back again.\nPress 'q' to quit.\n\n");
            break;

        case 2:
            // printf("Admin User\n");
            admin_handler(username, password, unique_account_id, clientfd);
            close_connection(clientfd, "Thank you for visiting us!\nPlease do come back again.\nPress 'q' to quit.\n\n");
            break;

        case 3:
            close_connection(clientfd, "You are unauthorized!\nPress 'q' to quit.\n\n");
            pthread_exit(NULL);
            // printf("You are unauthorized!\n");
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
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printf("Error in catching SIGINT\n");
    }
    
    while (1) {
        
    printf("SERVER...\n");
    
    int total_visitors_today = 0;
    
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
    pthread_mutex_init(&mutex2, NULL);
    pthread_mutex_init(&mutex3, NULL);

    pthread_t thread_ids[30];
    
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

        total_visitors_today += 1;

        printf("Connected to client: %s\n", inet_ntoa(client_address.sin_addr));
        printf("Total visitors today : %d\n", total_visitors_today);

        if( pthread_create( &thread_ids[i++], NULL, client_handler, &a) != 0 )
            perror("Could not create thread.\n ");

        // Let's in 15 clients per server session.
        if( i >= 25 )
        {
            i = 0;
            while (i < 25)
            {
                pthread_join(thread_ids[i++], NULL);
            }
            i = 0;  
        }
        
    }
    exit(1);
    }
    return 0;
}

