Zachariah Barraza - CISC450 Programming Project 2

Files:
    - Makefile, server.c, client.c

Compilation Instructions:
    - make client 
    - make server

Configuration Files:
    - N/A

Running Instructions:
    - First, generate the server executable using "make server" which generate the executable for the server, then generate the executable for the client by typing "make client".
    - Second, have two terminals open to the same folder which you can access the executables, there are no command line arguments for the executables. 
    - In order to run the program, first have two terminals open with access to the executables then first run "./server" on one of the terminals the program 
      will prompt the user to enter a timeout value between 1 and 10 which the user will input an int between that values, it will also prompt the user
      to enter a packet loss ratio value between 0 and 1 which is a float for the program to simulate packet loss. 
    - On the other terminal run "./client" which will then prompt the user to input a file name which the user will type in a file name that is in 
      the project folder or an absolute path to such file which this file that is to be inputted is going to be sent between the two in order 
      to generate packets and to create an out.txt file, there is another input that is necessary for the client which is the ACK loss ratio which will require the 
      client to enter the value for ack loss between 0 and 1 which will simulate the ACK loss. To which concludes what is necessary for the input of both programs to properly
      run the client and server programs.
    - The result of the code through the file out.txt can be viewed through means of "cat out.txt" to view contents or opened with
      any other text editors.

