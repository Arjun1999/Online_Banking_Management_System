# Online_Banking_Management_System
A multithreaded client-server architecture that replicates the behaviour of an online banking system.

I. Database :

The database is in the form of ".txt" files. It comprises of the following 3 files :

1. LoginInformation.txt : 
    - Contains the authorization details of the users.
    - Details are in the form -> || Username || Password || User Type || Unique Account Number ||

2. AccountsInformation.txt
    - Contains information regarding the accounts of customers.
    - Information is in the form -> || Unique Account Number || First Name || Last Name || Account Type || Balance || 

3. AdminRequests.txt
    - Contains queries to be processed by admin.
    - Each query has a different format. The 3 queries are :- adding an account, deleting an account and modifying an account (which has been limited to just password change).


II. Features implemented in the project :

1. A login system with 3 types of login - Normal User, Joint Account User, Admin

2. If a User is not registered at Login Prompt, he/she is given an option to create a normal/joint account. An add query is generated to be approved by Admin.

3. After a Normal/Joint User logs in, they are provided with the following menu :

(i)     Deposit :           Deposit  certain amount to your account.
(ii)    Withdraw :          Withdraw certain amount from your account.
(iii)   Balance Enquiry :   Provides current balance in the account.
(iv)    Password Change :   Generates a query for password change to be approved by Admin.
(v)     View Details :      View details of user's account number specified by him/her.
(vi)    Delete Account :    Generates a query for deletion of account to be approved by Admin.
(vii)   Exit :              Exit the menu.

4. After an Admin logs in , the following menu is provided :

(i)     Execute pending Add queries
(ii)    Execute pending Delete queries.
(iii)   Execute pending Modify queries.
(iv)    Search for a specifc account details.
(v)     Execute All Queries
(vi)    Exit

5. The concepts applied throughout the project include Socket Programming , Process Management, File Management, Mutex Locking, Multithreading and Inter Process Communication Mechanisms.