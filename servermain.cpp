// All connect 4 game game structures taken from connect4_wrapper.hpp, connect4.c, and connect4.h
// From the instructions
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
// Reused from previous project parts
// For signal handling
#include <signal.h>
// For std::map
#include <map>
// For std::string
#include <string>
// For file input and output
#include <fstream>
// For string streams
#include <sstream>
// For std::vector
#include <vector>

// Use standard namespace to avoid typing std:: everywhere
using namespace std;

// From instructions, previous project and Beej's Guide
// UDP port for communication with game servers
#define MAIN_SERVER_UDP_PORT 34619
// TCP port for communication with clients
#define MAIN_SERVER_TCP_PORT 35619
// Game Server A's UDP port
#define SERVERA_UDP_PORT 31619
// Game Server B's UDP port
#define SERVERB_UDP_PORT 32619
// Localhost IP address
#define LOCALHOST "127.0.0.1"
// Maximum number of pending connections in listen queue
#define BACKLOG 10
// Maximum buffer length for messages, made it four times bigger than 1024 just in case
#define MAXBUFLEN 4096

// Idea from previous project from Geeksforgeeks and cplusplus
// Map to store encrypted username and encrypted password pairs for login.txt
map<string, string> loginCredentials;
// Maps player name to server A or B
map<string, string> playerToServer;

// Function to decrypt text using Caesar cipher with offset -3
// Used to decrypt usernames from clients to match with player names from game servers
// From instructions and followed examples in tutorialspoint, Geeksforgeeks and spiceworks
string caesarDecrypt(const string& encryptedText) {
    // Create a copy of the input text
    string decryptedResult = encryptedText; 
    
    // Iterate through each character in the string
    for(size_t i = 0; i < decryptedResult.length(); i++) {
        // Get current character
        char currentChar = decryptedResult[i]; 
        
        // Decrypt lowercase letters
        // Shift back by 3 with wraparound
        if(currentChar >= 'a' && currentChar <= 'z') {
            decryptedResult[i] = 'a' + (currentChar - 'a' - 3 + 26) % 26;
        }
        // Decrypt uppercase letters 
        // Shift back by 3 with wraparound
        else if(currentChar >= 'A' && currentChar <= 'Z') {
            decryptedResult[i] = 'A' + (currentChar - 'A' - 3 + 26) % 26;
        }
        // Decrypt digits
        // Shift back by 3 with wraparound
        else if(currentChar >= '0' && currentChar <= '9') {
            decryptedResult[i] = '0' + (currentChar - '0' - 3 + 10) % 10;
        }
        // Special characters remain unchanged as the instructions say so
    }
    // Return the decrypted string
    return decryptedResult;
}

// Function to load encrypted logins from login.txt file used from previous project
void loadLoginFile() {
    // Open the login.txt file for reading
    ifstream loginFile("login.txt");
    
    // Variable to store each line from file
    string currentLine;
    // Read file line by line
    while(getline(loginFile, currentLine)) {
        // Remove any whitespace or carriage returns from the line used from previous project
        while(!currentLine.empty() && (currentLine.back() == '\r' || currentLine.back() == '\n' || currentLine.back() == ' ')) {
            currentLine.pop_back();
        }
        
        // Find the comma separator between username and password
        // From cplusplus and tutotialspoint
        size_t commaPosition = currentLine.find(',');

        // If comma found, split the line into username and password
        if(commaPosition != string::npos) {
            // Extract the username before the comma
            string encryptedUsername = currentLine.substr(0, commaPosition);
            // Extract the password after the comma
            string encryptedPassword = currentLine.substr(commaPosition + 1);
            // Store in a map
            loginCredentials[encryptedUsername] = encryptedPassword;
        }
    }
    // Close the file
    loginFile.close();
}

// Function to query both game servers for their player lists via UDP
void queryGameServers(int udp_sock) {
    // Address structures for both servers from Beej's Guide
    struct sockaddr_in serverA_addr, serverB_addr;
    // Buffer to receive responses from Beej's Guide
    char buf[MAXBUFLEN];

    // Query Server A from Beej's Guide and using previous project
    // Initialize Server A's address structure with zeros
    memset(&serverA_addr, 0, sizeof(serverA_addr));
    // Set address family to IPv4
    serverA_addr.sin_family = AF_INET;
    // Set IP address to localhost
    serverA_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Set port number and convert to network byte order
    serverA_addr.sin_port = htons(SERVERA_UDP_PORT);
    
    // Create request message for player list
    string playerListRequest = "PLAYER_LIST";

    // Send PLAYER_LIST request to Server A via UDP from Beej's Guide and previous project
    sendto(udp_sock, playerListRequest.c_str(), playerListRequest.length(), 0,
           (const struct sockaddr *)&serverA_addr, sizeof(serverA_addr));
    
    // Receive response from Server A from Beej's Guide and previous project
    // Length of address structure
    socklen_t addressLength = sizeof(serverA_addr);
    // Receive data
    int bytesReceived = recvfrom(udp_sock, buf, MAXBUFLEN, 0, 
                     (struct sockaddr *)&serverA_addr, &addressLength);
    // Null terminate the received string
    buf[bytesReceived] = '\0';
    
    // Store the received player list
    string playerListA(buf);

    // Print the confirmation message
    printf("Main server has received the player list from Server A using UDP on %d.\n", MAIN_SERVER_UDP_PORT);
    
    // Parse player list for Server A from previous project
    // Create string stream from player list
    stringstream ss(playerListA);
    // Variable to store individual player names
    string playerName;
    // Vector to store all players for Server A
    vector<string> playersServerA;

    // Split the player list by commas from geeksforgeeks and previous project
    while(getline(ss, playerName, ',')) {
        // Trim whitespace from player name
        while(!playerName.empty() && (playerName.back() == ' ' || playerName.back() == '\r' || playerName.back() == '\n')) {
            playerName.pop_back();
        }
        // Trim whitespace from player name
        while(!playerName.empty() && playerName.front() == ' ') {
            playerName.erase(0, 1);
        }
        // If player name is not empty after trimming, store it
        if(!playerName.empty()) {
            // Map player to Server A
            playerToServer[playerName] = "A";
            // Add to list of Server A players
            playersServerA.push_back(playerName);
        }
    }
    
    // Query Server B from Beej's Guide and using previous project
    // Initialize Server B's address structure with zeros
    memset(&serverB_addr, 0, sizeof(serverB_addr));
    // Set address family to IPv4
    serverB_addr.sin_family = AF_INET;
    // Set IP address to localhost
    serverB_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Set port number
    serverB_addr.sin_port = htons(SERVERB_UDP_PORT);
    
    // Send PLAYER_LIST request to Server B via UDP from Beej's Guide and previous project
    sendto(udp_sock, playerListRequest.c_str(), playerListRequest.length(), 0,
           (const struct sockaddr *)&serverB_addr, sizeof(serverB_addr));
    
    // Receive response from Server B from Beej's Guide and previous project
    // Length of address structure
    addressLength = sizeof(serverB_addr);
    // Receive data
    bytesReceived = recvfrom(udp_sock, buf, MAXBUFLEN, 0, 
                 (struct sockaddr *)&serverB_addr, &addressLength);
    // Null terminate the received string
    buf[bytesReceived] = '\0';
    
    string playerListB(buf);
    printf("Main server has received the player list from Server B using UDP on %d.\n", 
           MAIN_SERVER_UDP_PORT);
    
    // Parse player list for Server B
    // Reset string stream with new player list
    ss.str(playerListB);
    // Clear any error flags
    ss.clear();
    vector<string> playersServerB;
    // Vector to store all players for Server B
    
    // Split the player list by commas
    while(getline(ss, playerName, ',')) {
        // Trim whitespace from player name
        while(!playerName.empty() && (playerName.back() == ' ' || playerName.back() == '\r' || playerName.back() == '\n')) {
            playerName.pop_back();
        }
        // Trim whitespace from player name
        while(!playerName.empty() && playerName.front() == ' ') {
            playerName.erase(0, 1);
        }
        // If player name is not empty after trimming, store it
        if(!playerName.empty()) {
            // Map player to Server B
            playerToServer[playerName] = "B";
            // Add to list of Server B players
            playersServerB.push_back(playerName);
        }
    }
    
    // Print Server A's players from previous project
    printf("Server A: ");
    for(size_t i = 0; i < playersServerA.size(); i++) {
        // Add comma between names
        if(i > 0) printf(", ");
        // Print player name
        printf("%s", playersServerA[i].c_str());
    }
    // Print Server B's players
    printf("\nServer B: ");
    for(size_t i = 0; i < playersServerB.size(); i++) {
        // Add comma between names
        if(i > 0) printf(", ");
        // Print player name
        printf("%s", playersServerB[i].c_str());
    }
    // New line after Server B's players
    printf("\n");
}

// Signal handler for SIGCHLD to reap zombie child processes from Beej's Guide, Geeksforgeeks, UNIX&LINUX, stack overflow and previous project
// Called automatically when a child process terminates
void sigchld_handler(int s) {
    // Save errno to restore later
    int saved_errno = errno;

    // Wait for all terminated child processes
    // WNOHANG returns immediately if no child has exited
    while(waitpid(-1, NULL, WNOHANG) > 0);

    // Restore errno
    errno = saved_errno;
}

// Function to handle communication with a single client from Beej's Guide
// Parameters: client_sock - TCP socket connected to the client and udp_sock - UDP socket for communication with game servers
void handleClient(int client_sock, int udp_sock) {
    // Buffer for receiving/sending data
    char buf[MAXBUFLEN];
    
    // Receive authentication data from client from Beej's Guide
    int bytesReceived = recv(client_sock, buf, MAXBUFLEN-1, 0);

    // Check if receive failed or connection closed from Beej's Guide
    if(bytesReceived <= 0) {
        // Close the socket
        close(client_sock);
        // Exit child process
        exit(0);
    }

    // Null terminate the received string
    buf[bytesReceived] = '\0';
    
    // Parse the authentication data
    // Convert to C++ string
    string authData(buf);
    // Find the comma
    size_t commaPosition = authData.find(',');
    
    // Variables to store authentication info
    // Encrypted credentials from client
    string encryptedUsername, encryptedPassword;
    // Decrypted username for server lookup
    string decryptedUsername;
    // Flag to indicate guest login
    bool isGuest = false;
    
    // Check if this is a guest login (no password, no comma)
    if(commaPosition == string::npos) {
        // Guest login
        // Username only (encrypted)
        encryptedUsername = authData;
        // Decrypt for display
        decryptedUsername = caesarDecrypt(encryptedUsername);
        // Mark as guest
        isGuest = true;
        // Print confirmation message
        printf("Received guest login for %s over TCP. Accepted guest login. Response sent.\n", 
               encryptedUsername.c_str());
        
        // Send success response to client
        string responseToClient = "GUEST_SUCCESS";
        // From Beej's Guide and previous project
        send(client_sock, responseToClient.c_str(), responseToClient.length(), 0);
    } else {

        // Member login
        // Split authentication data into username and password
        // Extract username (encrypted)
        encryptedUsername = authData.substr(0, commaPosition);
        // Extract password (encrypted)
        encryptedPassword = authData.substr(commaPosition + 1);
        // Decrypt username for server lookup
        decryptedUsername = caesarDecrypt(encryptedUsername);
        
        // Print authentication attempt message
        printf("Received authentication for %s over TCP on %d. ", 
               encryptedUsername.c_str(), MAIN_SERVER_TCP_PORT);
        
        // Check if encrypted credentials match login.txt
        bool authSuccess = false;
        // Username exists
        if(loginCredentials.find(encryptedUsername) != loginCredentials.end() && 
            // Password matches
           loginCredentials[encryptedUsername] == encryptedPassword) {
            authSuccess = true;
        }
        
        // Handle authentication result
        if(authSuccess) {
            // Authentication successful
            printf("Authentication passed. Sent result to client.\n");
            
            // Look up which game server manages this player (using decrypted username)
            // Default to Unknown
            string serverName = "Unknown";
            // Look up by decrypted username
            if(playerToServer.find(decryptedUsername) != playerToServer.end()) {
                // Get Server A or B
                serverName = playerToServer[decryptedUsername];
            }
            
            // Send success response with server assignment to client
            string responseToClient = "MEMBER_SUCCESS:" + serverName;
            send(client_sock, responseToClient.c_str(), responseToClient.length(), 0);
        } else {
            // Authentication failed message
            printf("Authentication failed. Sent result to client.\n");

            // Send failure response to client
            string responseToClient = "FAIL";
            send(client_sock, responseToClient.c_str(), responseToClient.length(), 0);

            // Close connection and exit child process
            close(client_sock);
            exit(0);
        }
    }
    
    // Handle queries from previous project
    // Loop to continuously handle client requests
    while(1) {
        // Receive query from client
        bytesReceived = recv(client_sock, buf, MAXBUFLEN-1, 0);

        // Check if receive failed or connection closed
        // Exit loop if connection closed
        if(bytesReceived <= 0) break;

        // Null terminate the received string
        buf[bytesReceived] = '\0';
        
        // Convert to C++ string
        string queryRequest(buf);
        
        // Check if query is VIEW or MOVE request
        if(queryRequest.substr(0, 4) == "VIEW" || queryRequest.substr(0, 4) == "MOVE") {
            // Determine action type for logging
            string actionRequest = (queryRequest.substr(0, 4) == "VIEW") ? "View" : "Move";
            
            // Print query message
            printf("Received query %s from %s %s. ", 
                   actionRequest.c_str(), 
                   // User type
                   isGuest ? "guest" : "member", 
                   // Encrypted username
                   encryptedUsername.c_str());
            
            // Determine which game server to forward the request to
            // Default to Server A
            string targetServer = "A";

            for(auto& playerEntry : playerToServer) {
                // Look up the correct server based on decrypted username
                if(playerEntry.first == decryptedUsername) {
                    targetServer = playerEntry.second;
                    break;
                }
            }
            
            // Print forwarding message
            printf("Forwarded to Server %s.\n", targetServer.c_str());
            
            // Forward to right game server
            // Set up game server address structure from Beej's Guide
            struct sockaddr_in game_server_addr;
            // Initialize with zeros
            memset(&game_server_addr, 0, sizeof(game_server_addr));
            // IPv4
            game_server_addr.sin_family = AF_INET;
            // Localhost
            game_server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
            // Select port based on target server
            game_server_addr.sin_port = htons(targetServer == "A" ? SERVERA_UDP_PORT : SERVERB_UDP_PORT);
            
            // Send query to game server via UDP from Beej's Guide
            sendto(udp_sock, queryRequest.c_str(), queryRequest.length(), 0,
                   (const struct sockaddr *)&game_server_addr, sizeof(game_server_addr));
            
            // Receive response from game server via UDP from Beej's Guide
            socklen_t addressLength = sizeof(game_server_addr);
            bytesReceived = recvfrom(udp_sock, buf, MAXBUFLEN-1, 0,
                        (struct sockaddr *)&game_server_addr, &addressLength);
            // Null terminate the received string
            buf[bytesReceived] = '\0';
            
            // Print reply message
            printf("Received reply from Server %s over UDP. Sent response to %s over TCP.\n",
                   targetServer.c_str(), isGuest ? "guest" : "member");
            
            // Send response to client
            send(client_sock, buf, bytesReceived, 0);
        }
    }
    // Close client socket
    close(client_sock);
    // Exit child process
    exit(0);
}

// Main function from Beej's Guide and previous project
// Entry point of the program
int main() {
    // Socket file descriptors
    // TCP socket for clients, UDP socket for game servers
    int tcp_sock, udp_sock;
    // Address structures for binding
    struct sockaddr_in tcp_addr, udp_addr;
    // Signal action structure for handling child process termination
    struct sigaction sa;
    
    // Load encrypted username/password from login.txt
    loadLoginFile();
    
    // Create UDP socket
    if((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        // Print error message
        perror("UDP socket creation failed");
        // Exit with error code
        exit(1);
    }
    
    // Initialize UDP address structure with zeros from Beej's Guide and previous project
    memset(&udp_addr, 0, sizeof(udp_addr));
    // IPv4
    udp_addr.sin_family = AF_INET;
    // Bind to localhost
    udp_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Bind to main server UDP port
    udp_addr.sin_port = htons(MAIN_SERVER_UDP_PORT);
    
    // Bind UDP socket to the specified address and port from Beej's Guide
    if(bind(udp_sock, (const struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        // Print error message
        perror("UDP bind failed");
        // Exit with error code
        exit(1);
    }
    
    // Create TCP socket
    if((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // Print error message
        perror("TCP socket creation failed");
        // Exit with error code
        exit(1);
    }
    
    // Set socket option to reuse address from Beej's Guide
    int yes = 1;
    if(setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        // Print error message
        perror("setsockopt");
        // Exit with error code
        exit(1);
    }
    
    // Initialize TCP address structure with zeros from Beej's Guide
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    // IPv4
    tcp_addr.sin_family = AF_INET;
    // Bind to localhost
    tcp_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Bind to main server TCP port
    tcp_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    
    // Bind TCP socket to the specified address and port
    if(bind(tcp_sock, (const struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        // Print error message
        perror("TCP bind failed");
        // Exit with error code
        exit(1);
    }
    
    // Start listening for incoming TCP connections from Beej's Guide
    // BACKLOG defines maximum number of pending connections
    if(listen(tcp_sock, BACKLOG) < 0) {
        // Print error message
        perror("listen failed");
        // Exit with error code
        exit(1);
    }
    
    // Print bootup message
    printf("Main server is up and running.\n");
    
    // Query both game servers to get their player lists
    queryGameServers(udp_sock);
    
    // Configure signal handler to reap zombie child processes from Beej's Guide
    // Set handler function
    sa.sa_handler = sigchld_handler;
    // Initialize signal mask to empty
    sigemptyset(&sa.sa_mask);
    // Restart system calls interrupted by signals
    sa.sa_flags = SA_RESTART;

    // Register the signal handler for SIGCHLD from Beej's Guide
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        // Print error message
        perror("sigaction");
        // Exit with error code
        exit(1);
    }
    
    // Main server loop
    // Accept client connections
    while(1) {
        // Client address structure
        struct sockaddr_in client_addr;
        // Size of address structure
        socklen_t sin_size = sizeof(client_addr);
        
        // Accept incoming client connection from Beej's Guide
        int client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &sin_size);

        // Check if accept failed
        if(client_sock == -1) {
            // Print error message
            perror("accept");
            // Continue to next iteration
            continue;
        }
        
        // Fork a child process to handle this client from Beej's Guide
        if(!fork()) {
            // Child doesn't need the listening socket
            close(tcp_sock);
            // Handle all communication with this client
            handleClient(client_sock, udp_sock);
            // handleClient will exit the child process when done
        }
        // Parent doesn't need the client socket 
        close(client_sock);
        // Loop continues to accept next client connection
    }
    // Clean up sockets 
    close(tcp_sock);
    close(udp_sock);
    // Exit
    return 0;
}