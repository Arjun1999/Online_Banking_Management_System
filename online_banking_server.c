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
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

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

    int retval = 0;

    pthread_mutex_lock(&mutex1);
    
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
            strcat(buffer_copy, token); 
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

void search_account_handler(char *buffer)
{

}

void modify_account_handler(char *buffer)
{

}

void delete_account_handler(char *buffer)
{

}

void add_account_handler(char *buffer)
{
    
    char * buffer_copy_login = (char *) malloc(140);
    char * buffer_copy_account = (char *) malloc(140);

    char *token = strtok(buffer," ");

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
            printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT, 0777);
            printf("fd2 : %d\n", fd2);
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
                i++;
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
            printf("Remove : %d\n", rem);
            ren = rename("AdminRequestsDuplicate.txt", "AdminRequests.txt");
            printf("Rename : %d\n", ren);
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

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT);
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
                i++;
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

        case 3:
            pthread_mutex_lock(&mutex1);

            fd1 = open("AdminRequests.txt", O_RDONLY, 0);
            // printf("fd1 : %d\n", fd1);
            if (fd1 < 0)
            {
                perror("Could not open the file containing query requests to Admin.\n");
                // return NULL;
            }

            fd2 = open("AdminRequestsDuplicate.txt", O_WRONLY | O_CREAT);
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
                i++;
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
                i++;
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
    send_to_client(clientfd, "Do you want to:\n1. Execute Pending Add Queries\n2. Execute Pending Delete Queries\n3. Execute Pending Modify Queries\n4. Execute Pending Search Queries\n5. Execute All Queries\n6. Exit\nPlease enter number of your choice: \n\n");

    char *response = (char *)malloc(140);
    while (track_session == 1)
    {
        int choice;
        response = receive_from_client(clientfd);
        choice = atoi(response);

        switch (choice)
        {
            case 1:
            case 2:
            case 3:
            case 4:
                retval = individual_handler(choice);
                if(retval == 1)
                {
                    send_to_client(clientfd, "Queries successfully executed\n");
                }
                else
                {
                    if(choice == 1)
                    {
                        send_to_client(clientfd, "No pending Add queries\n");
                    }
                    
                    else if (choice == 2)
                    {
                        send_to_client(clientfd, "No pending Delete queries\n");
                    }

                    if (choice == 3)
                    {
                        send_to_client(clientfd, "No pending Modify queries\n");
                    }

                    if (choice == 4)
                    {
                        send_to_client(clientfd, "No pending Search queries\n");
                    }
                }
                
                break;
            case 5:
                individual_handler(1);
                individual_handler(2);
                individual_handler(3);
                individual_handler(4);
                send_to_client(clientfd, "All queries have been executed\n");
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
        switch (choice)
        {
            case 1:
                operation = 1;
                
                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);
                if(account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\n");
                    break;  
                }

                send_to_client(clientfd, "Enter amount to deposit: \n");
                amt = receive_from_client(clientfd);
                amount = atoi(amt);

                deposit_withdraw_ret = deposit_withdraw_handler(account_id, operation, amount);
                if(deposit_withdraw_ret == 0)
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
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\n");
                    break;
                }

                send_to_client(clientfd, "Enter amount to withdraw: \n");
                amt = receive_from_client(clientfd);
                amount = atoi(amt);

                deposit_withdraw_ret = deposit_withdraw_handler(account_id, operation, amount);
                if (deposit_withdraw_ret == -1)
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
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\n");
                    break;
                }

                balance = check_balance(account_id);

                if(balance == -1)
                {
                    send_to_client(clientfd,"The account no longer exists!\n");
                }
                else
                {
                    sprintf(balance_str, "%d", balance);
                    strcat(balance_str, " is your balance\n");
                    send_to_client(clientfd, balance_str);
                }
                
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
                        pthread_mutex_lock(&mutex1);
                        query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT);
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
                        send_to_client(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\n");
                        break;
                    }
                    else
                    {
                        send_to_client(clientfd, "Incorrect confirmation password.\n");
                        break;
                    }
                    
                }
                else
                {
                    send_to_client(clientfd, "The password you have entered is incorrect.\n");
                }
                
                break;

            case 5:
                operation = 5;

                send_to_client(clientfd, "Enter your account number: \n");
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\n");
                    break;
                }

                pthread_mutex_lock(&mutex1);
                query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT);
                if (query_fd < 0)
                {
                    printf("Could not open the file containing requests to Admin.\n");
                }
                strcpy(query_buff, "Search ");
                strcat(query_buff, acc_id);
                strcat(query_buff, "\n");

                lseek(query_fd, 0L, SEEK_END);
                write(query_fd, query_buff, strlen(query_buff));
                pthread_mutex_unlock(&mutex1);
                send_to_client(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\n");
                break;

            case 6:
                operation = 6;

                send_to_client(clientfd, "Enter your account number: \n");
                char *acc_id = (char *)malloc(140);
                acc_id = receive_from_client(clientfd);
                account_id = atoi(acc_id);
                
                if (account_id != unique_account_id)
                {
                    send_to_client(clientfd, "Incorrect Account ID!\n");
                    break;
                }

                pthread_mutex_lock(&mutex1);
                query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT);
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
                send_to_client(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\n");
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
    int unique_account_id = auth_values[3];

    if(auth_values[0] == -1)
    {
        send_to_client(clientfd, "No such user exists!\n\nDo you wish to add a new account? (Y/N)\n");
        char * response = receive_from_client(clientfd);
        
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

            send_to_client(clientfd, "Enter a password: \n");
            new_password = receive_from_client(clientfd);

            send_to_client(clientfd, "Enter User type (Normal/Joint/Admin): \n");
            new_user_type = receive_from_client(clientfd);

            if(strcmp(new_user_type,"Joint") == 0)
            {
                send_to_client(clientfd, "Enter the joint account number: \n");
                new_acc_no = receive_from_client(clientfd);
            }

            else
            {
                srand(time(NULL));
                new_acc_number = rand() % 10000 + 1;
                sprintf(new_acc_no, "%d", new_acc_number);
            }
            
            
            send_to_client(clientfd, "Enter First Name: \n");
            new_fname = receive_from_client(clientfd);

            send_to_client(clientfd, "Enter Last Name: \n");
            new_lname = receive_from_client(clientfd);

            send_to_client(clientfd, "Enter Account type (Savings/Current): \n");
            new_acc_type = receive_from_client(clientfd);
            
            send_to_client(clientfd, "Enter initial deposit: \n");
            new_initial_dep = receive_from_client(clientfd);

            printf("Before lock\n");
            pthread_mutex_lock(&mutex1);
            printf("After lock\n");
            int query_fd = open("AdminRequests.txt", O_WRONLY | O_CREAT, 0777);
            if (query_fd < 0)
            {
                printf("Could not open the file containing requests to Admin.\n");
            }
            printf("%d\n", query_fd);
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

            printf("%s\n", query_buff);

            lseek(query_fd, 0L, SEEK_END);
            write(query_fd, query_buff, strlen(query_buff));
            pthread_mutex_unlock(&mutex1);
            send_to_client(clientfd, "Your query has been successfully added to the query list for the Admin to be reviewed.\n");
        }
        else
        {
            send_to_client(clientfd, "Thank you for visiting!\n");
        }
        pthread_exit(NULL);
    } 

    if(auth_values[1] == -1)
    {
        send_to_client(clientfd, "Incorrect password entered!\n");
        pthread_exit(NULL);
    }

    switch(auth_values[2])
    {
        case 0:
            printf("Normal User\n");
            user_handler(username, password, unique_account_id, clientfd);
            break;

        case 1:
            printf("Joint User\n");
            user_handler(username, password, unique_account_id, clientfd);
            break;

        case 2:
            printf("Admin User\n");
            admin_handler(username, password, unique_account_id, clientfd);
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
    pthread_mutex_init(&mutex2, NULL);
    pthread_mutex_init(&mutex3, NULL);

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

