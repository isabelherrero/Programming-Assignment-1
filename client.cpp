// For input/output stream (cout, cin)
#include <iostream>
#include <string>
// For memset()
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Port number of the server as a string
#define SERVER_PORT "24619"
// Server IP address
#define SERVER_ADDR "127.0.0.1" 
// Max buffer size 1024
#define BUFFERSIZE 1024 

using namespace std;

int main() {
    // Tells the client the program is running
    cout << "Client is up and running." << endl;

    // Infinite loop to allow multiple queries without restarting the client and can only be killed by Ctrl+C
    while (true) {
        // Declare string to hold player name input
        string playerName;
        // Ask user for player name
        cout << "Enter Player Name: ";
        // Reads the input line
        getline(cin, playerName);

        // Create socket for TCP connection
        // addrinfo struct for specifying criteria and result pointer from Beej's Guide
        struct addrinfo hints{}, *res;
        // Socket file descriptor from Beej's Guide
        int sockfd;

        // Zero out the hints structure from Beej's Guide
        memset(&hints, 0, sizeof hints);
        // Allow IPv4 or IPv6 from Beej's Guide
        hints.ai_family = AF_UNSPEC;
        // TCP socket from Beej's Guide
        hints.ai_socktype = SOCK_STREAM;

        // Get address info for the server address and port from Beej's Guide
        if (getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &res) != 0) {
            // Error checking to see if the address lookup fails
            // Got it from instructions and from Beej's Guide
            perror("getaddrinfo");
            // Exit the program with failure code from project instructions
            exit(1);
        }

        // Make a socket using the parameters returned by getaddrinfo from Beej's Guide
        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            // Error checking if socket creation failed from Beej's Guide and instructions
            perror("socket");
            // Exit the program with failure code from project instructions
            exit(1);
        }

        // Connect the socket to the server using the address info from Beej's Guide
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            // Error checking to see if connection fails
            perror("connect");
            // Close the socket before exiting to prevent any more reads and writes to the socket from Beej's Guide
            close(sockfd);
            // Exit the program with failure code from project instructions
            exit(1);
        }

        // Get the client’s dynamically assigned port number (client ID)
        // Create a struct to hold client socket address information from Beej's Guide
        struct sockaddr_in my_addr{};
        // Stores the size of my_addr struct because getsockname will need this info
        // socklen_t and sizeoff() came from Beej's Guide
        socklen_t len = sizeof(my_addr);
        // Gets the local socket address and stores it in my_addr and len will update the size of the address returned
        // Got idea from Beej's Guide but used getsockname instead of getpeername because its more fitting
        getsockname(sockfd, (struct sockaddr*)&my_addr, &len);
        // Convert port from network byte order to host byte order and store the result in clientID
        // ntohs comes from Beej's Guide
        int clientID = ntohs(my_addr.sin_port);

        // Send the player name string and number of bytes to the server from Beej's Guide
        send(sockfd, playerName.c_str(), playerName.size(), 0);

        // Output to the terminal that player name sent along with client’s port (ID)
        cout << "Client (ID: " << clientID << ") has sent Player " << playerName
             << " to Main Server using TCP over port " << clientID << "." << endl;

        // Initializes buffer to zero from Beej's Guide
        char buf[BUFFERSIZE]{};
        // Calls recv() to receive the data from the server over the socket from Beej's Guide
        int numbytes = recv(sockfd, buf, BUFFERSIZE - 1, 0);
        // Make a string with buf contents
        string serverReply(buf);

        // Checks if it indicates the player was not found or found
        // npos from Geeksforgeeks
        if (serverReply.find("Not found") != string::npos) {
            // If the server reply is not found and then it prints not found
            cout << playerName << " not found." << endl;
        } else {
            // If player is found then it outputs the result
            cout << "Client has received results from Main Server:\n" << serverReply << endl;
        }

        // Output to begin asking for new player name
        cout << "-----Start a new query-----" << endl;

        // Close socket connection from Beej's Guide
        close(sockfd);
        // The system allocates memory to store the list of addrinfo structures from Beej's Guide
        freeaddrinfo(res);
    }

    // End program but this isn't reached because it is set as an infinite loop
    return 0;
}
