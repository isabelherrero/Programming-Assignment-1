// All connect 4 game game structures taken from connect4_wrapper.hpp, connect4.c, and connect4.h
// From the instructions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h> 
// For std::string
#include <string>
// For input/output
#include <iostream>

// Use standard namespace to avoid typing std:: everywhere
using namespace std;

// From instructions, previous project and Beej's Guide
// TCP port for communication with clients
#define MAIN_SERVER_TCP_PORT 35619
// Localhost IP address
#define LOCALHOST "127.0.0.1"
// Maximum buffer length for messages, made it four times bigger than 1024 just in case
#define MAXBUFLEN 4096

// Function to encrypt text using Caesar cipher with offset +3
// Used to encrypt username and password before sending to main server
// From instructions and followed examples in tutorialspoint, Geeksforgeeks and spiceworks
string caesarEncrypt(const string& decryptedText) {
    // Create a copy of the input text
    string encryptedResult = decryptedText;

    // Iterate through each character in the string
    for(size_t i = 0; i < encryptedResult.length(); i++) {
        // Get current character
        char currentChar = encryptedResult[i];
        
        // Encrypt lowercase letters and shift forward by 3 with wraparound
        if(currentChar >= 'a' && currentChar <= 'z') {
            encryptedResult[i] = 'a' + (currentChar - 'a' + 3) % 26;
        }
        // Encrypt uppercase letters and shift forward by 3 with wraparound
        else if(currentChar >= 'A' && currentChar <= 'Z') {
            encryptedResult[i] = 'A' + (currentChar - 'A' + 3) % 26;
        }
        // Encrypt digits and shift forward by 3 with wraparound
        else if(currentChar >= '0' && currentChar <= '9') {
            encryptedResult[i] = '0' + (currentChar - '0' + 3) % 10;
        }
        // Special characters remain unchanged
    }
    // Return the encrypted string
    return encryptedResult;
}

// Main function from Beej's Guide and previous project
// Entry point of the client program
int main() {
    // Socket file descriptor for TCP connection
    int sockfd;
    // Server address structure
    struct sockaddr_in server_addr;
    // Buffer for receiving data
    char buf[MAXBUFLEN];
    
    // Print bootup message
    printf("Client is up and running.\n");
    
    // Create TCP socket for communication with main server
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // Print error message
        perror("socket creation failed");
        // Exit with error code
        exit(1);
    }
    
    // Initialize server address structure with zeros from Beej's Guide and previous project
    memset(&server_addr, 0, sizeof(server_addr));
    // Set address family to IPv4
    server_addr.sin_family = AF_INET;
    // Set server IP to localhost
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Set server port
    server_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    
    // Establish TCP connection to the main server from Beej's Guide and previous project
    if(connect(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        // Print error message
        perror("connection failed");
        // Exit with error code
        exit(1);
    }
    
    // Variables to store user input
    // Store username and password
    string unencryptedUsername, unencryptedPassword;
    // Temporary buffer for reading input
    char inputBuffer[256];
    
    // Prompt for username
    printf("Please enter username: ");
    // Flush output buffer to ensure prompt is displayed from geeksforgeeks
    fflush(stdout);

    // Read username from standard input
    if(fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
        // Convert C string to C++ string
        unencryptedUsername = inputBuffer;
        // Remove newline character if present
        if(!unencryptedUsername.empty() && unencryptedUsername[unencryptedUsername.length()-1] == '\n')
            unencryptedUsername.erase(unencryptedUsername.length()-1);
    }
    
    // Prompt for password (optional for guests)
    printf("Please enter password (Enter to skip for guests): ");
    // Flush output buffer to ensure prompt is displayed
    fflush(stdout);

    // Read password from standard input
    if(fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
        // Convert C string to C++ string
        unencryptedPassword = inputBuffer;
        // Remove newline character if present
        if(!unencryptedPassword.empty() && unencryptedPassword[unencryptedPassword.length()-1] == '\n')
            unencryptedPassword.erase(unencryptedPassword.length()-1);
    }
    
    // Determine if this is a guest login (no password entered)
    bool isGuest = unencryptedPassword.empty();
    
    // Encrypt username and password using Caesar cipher
    string encryptedUsername = caesarEncrypt(unencryptedUsername);
    string encryptedPassword = caesarEncrypt(unencryptedPassword);
    
    // Build authentication data string
    string authData;
    // Guest login: send only encrypted username
    if(isGuest) {
        authData = encryptedUsername;
    } else {
        // Member login: send encrypted username and password separated by comma
        authData = encryptedUsername + "," + encryptedPassword;
    }
    
    // Print confirmation message
    printf("%s sent authentication request to main server.\n", unencryptedUsername.c_str());
    // Send authentication data to main server via TCP from Beej's Guide and previous project
    send(sockfd, authData.c_str(), authData.length(), 0);
    
    // Receive authentication response from main server
    int bytesReceived = recv(sockfd, buf, MAXBUFLEN-1, 0);
    
    // Check if receive failed or connection closed
    if(bytesReceived <= 0) {
        // Close the socket
        close(sockfd);
        // Exit with error code
        exit(1);
    }
    // Null terminate the received string
    buf[bytesReceived] = '\0';
    
    // Convert received data to C++ string
    string authenticationResponse(buf);
    
    // Check if authentication failed
    if(authenticationResponse == "FAIL") {
        // Print failure message
        printf("Failed login. Invalid username or password.\n");
        // Close the socket
        close(sockfd);
        // Exit program
        return 0;
    }
    // Check if guest login succeeded
    else if(authenticationResponse == "GUEST_SUCCESS") {
        // Print welcome message for guest
        printf("Welcome guest %s!\n", unencryptedUsername.c_str());
    }
    // Check if member login succeeded
    else if(authenticationResponse.substr(0, 14) == "MEMBER_SUCCESS") {
        // Extract server name from response
        string serverName = authenticationResponse.substr(15);
        // Print welcome message for member with server assignment
        printf("Welcome member %s! You are now connected to Game Server %s.\n", 
               unencryptedUsername.c_str(), serverName.c_str());
    }
    
    // Continuously handle user queries until connection is closed
    while(1) {
        // Print action prompt
        printf("\n----- Start a new action -----\n");
        printf("Please enter request action (view, move): ");
        // Flush output buffer to ensure prompt is displayed
        fflush(stdout);
        
        // Read user action from standard input
        if(!fgets(inputBuffer, sizeof(inputBuffer), stdin)) break;
        
        // Convert input to C++ string
        string userAction = inputBuffer;
        // Remove newline character if present
        if(!userAction.empty() && userAction[userAction.length()-1] == '\n')
            userAction.erase(userAction.length()-1);
        
        // Skip empty input
        if(userAction.empty()) continue;
        
        // Variable to store query to send to server
        string queryMessage;
        
        // Check if user wants to view the game board
        if(userAction == "view") {
            // Print confirmation message
            printf("%s requested to view the current board.\n", unencryptedUsername.c_str());
            // Build VIEW query
            queryMessage = "VIEW";
            // Send VIEW request to main server
            send(sockfd, queryMessage.c_str(), queryMessage.length(), 0);
            
            // Receive board state from main server
            bytesReceived = recv(sockfd, buf, MAXBUFLEN-1, 0);
            // Exit loop if connection closed
            if(bytesReceived <= 0) break;
            // Null terminate the received string
            buf[bytesReceived] = '\0';
            
            // Convert received data to C++ string
            string gameBoard(buf);
            // Print confirmation message
            printf("Current board state received from server.\n");
            
            // Parse and display board
            if(gameBoard.substr(0, 6) == "BOARD:") {
                // Print the board state
                printf("%s\n", gameBoard.substr(6).c_str());
            }
        }
        // Check if user wants to make a move
        else if(userAction.substr(0, 4) == "move") {
            // Check if user is a guest
            if(isGuest) {
                // Print message for guest not allowed to make moves
                printf("Guests cannot make moves.\n");
                // Skip to next iteration
                continue;
            }
            
            // Variable to store column number
            int columnIndex;
            // Prompt for column number
            printf("Please enter column to drop your token (0 to quit): ");
            // Flush output buffer to ensure prompt is displayed
            fflush(stdout);

            // Read column number from standard input
            if(scanf("%d", &columnIndex) != 1) {
                // If input is invalid, clear input buffer and continue from geekforgeeks
                while(getchar() != '\n');
                continue;
            }
            // Clear newline from input buffer
            while(getchar() != '\n');
            
            // Check if user wants to quit
            if(columnIndex == 0) {
                // Print quitting confirmation message
                printf("Quitting...\n");
                // Exit the query loop
                break;
            }
            
            // Print confirmation message
            printf("%s sent move request for column %d.\n", unencryptedUsername.c_str(), columnIndex);
            
            // Build MOVE query with column number
            queryMessage = "MOVE:" + to_string(columnIndex);
            // Send MOVE request to main server
            send(sockfd, queryMessage.c_str(), queryMessage.length(), 0);
            
            // Receive move result from main server
            bytesReceived = recv(sockfd, buf, MAXBUFLEN-1, 0);
            // Exit loop if connection closed
            if(bytesReceived <= 0) break;
            // Null terminate the received string
            buf[bytesReceived] = '\0';
            
            // Convert received data to C++ string
            string moveResponse(buf);
            
            // Check if move was invalid
            if(moveResponse.substr(0, 7) == "INVALID") {
                // Print invalid move message
                printf("Move in column %d is invalid or not allowed.\n", columnIndex);
            }
            // Check if move was accepted
            else if(moveResponse.substr(0, 8) == "ACCEPTED") {
                // Print success message
                printf("Move accepted for column %d. Board updated.\n", columnIndex);
                
                // Display updated board
                size_t boardPosition = moveResponse.find("ACCEPTED:");
                if(boardPosition != string::npos) {
                    // Extract board data
                    string boardData = moveResponse.substr(9);
                    // Print the updated board
                    printf("%s\n", boardData.c_str());
                }
            }
        }
        // User entered an invalid action
        else {
            printf("Invalid action. Please enter 'view' or 'move'.\n");
        }
    }
    // Close the TCP socket
    close(sockfd);
    return 0;
}