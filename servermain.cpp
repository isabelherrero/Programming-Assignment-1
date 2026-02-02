// For all standard C++ libraries, got it from Geeksforgeeks
#include <bits/stdc++.h>

// Header files from instructions, reused from previous project part 1
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
#include <sys/wait.h>

// Used from previous project part 1
using namespace std;

// UPD port definition for game server A
#define PORT_A 30619
// UPD port definition for game server B
#define PORT_B 31619
// UPD port definition for game server C
#define PORT_C 32619
// UPD port definition for main server
#define PORT_MAIN 33619
// Max buffer size 1024
#define BUFFERSIZE 1024

// Localhost IP address, loopback address from instructions and Beej's Guide
// ipstr from Beej's Guide
// Declares and initializes a global constant string variable that stores the local IP address
static string ipstr = "127.0.0.1";

// Helper function to help send a UDP message (payload) to the specified port on localhost
// int sock comes from Beej's Guide - represents UDP socket
// int port - the destination port number where I want to send the message
// const string &payload - message I'm sending
static void sendTo(int sock, int port, const string &payload){
    // sockaddr_in - a struct defined in <netinet/in.h> to represent an IPv4 address
    // destAddr - to store the destination address and initiliaze at 0, idea from Beej's Guide but renamed variable rather using their_addr
    sockaddr_in destAddr{}; 
    // sin_family from Beej's Guide - struct that specifies the address family
    // AF_INET from Beej's Guide - constant from <sys/socket.h> that means IPv4 internet address
    destAddr.sin_family=AF_INET; 
    // sin_addr.s_addr from Beej's Guide - holds the destination IP address in 32-bit binary form
    // INADDR_LOOPBACK from Beej's Guide - constant for the IP address 127.0.0.1 (localhost), defined in <netinet/in.h>
    // htonl() from Beej's Guide - converts a 32-bit integer from <arpa/inet.h>
    destAddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK); 
    // sin_port from Beej's Guide - field of the sockaddr_in struct that holds the destination port number
    // htons() from Beej's Guide - converts a 16-bit integer from <arpa/inet.h>
    destAddr.sin_port=htons(port);
    // Send the message to the specified destination
    // sendto() from Beej's Guide - send a UDP datagram to a destination address
    // payload.c_str() - converts string message to C-style string
    // payload.size() - message length
    // 0 - flags argument
    // (sockaddr*)&destAddr - pointer to destination address
    // sizeof(destAddr) - size of the destination address in bytes
    sendto(sock, payload.c_str(), payload.size(), 0, (sockaddr*)&destAddr, sizeof(destAddr));
}

// trim() helper function to modify the original string from CPlusPlus
// Just in case there if extra spaces, commas, etc. even though assumptions said that there is no white spaces
// because last project part had issues that if data wasn't precise, then it wouldn't read it properly
static void trim(string &inputString){
    // while(!s.empty() - loop if string not empty
    // s.back() from CPlusPlus - return last character of string and check its for:
        // newline character
        // carriage return
        // space
        // commas
    // s.pop_back() from Geeksforgeeks - removes last character from string
    while(!inputString.empty() && (inputString.back()=='\n'||inputString.back()=='\r'||inputString.back()==' '||inputString.back()==',')) inputString.pop_back();
    // Defines an index to start checking from the beginning of string
    size_t i=0; 
    // Counts how many leading spaces there are at the front of string
    while(i < inputString.size() && inputString[i] == ' ') i++;
    // If found leading spaces, then deletes first i characters to remove the leading spaces
    if(i) inputString.erase(0,i);
}

// Helper function to split a string 's' into tokens separated by delimiter 'd' from Geekforgeeks
// char d - delimiter characters like commans, spaces and semicolons
static vector<string> split(const string &inputString, char delimiter){
    // Store all substrings after splitting
    vector<string> tokens;
    // Build each substring before adding to list
    string currentToken; 
    // Iterates through each character in the string
    for(char currentChar: inputString) { 
        // If the current character is the delimiter
        if(currentChar == delimiter) { 
            // Save the token
            tokens.push_back(currentToken); 
            // Reset it for the next token
            currentToken.clear(); 
        } 
    else 
    // Add character to current token
    currentToken.push_back(currentChar); 
    }
    // Add final token
    tokens.push_back(currentToken); 
    // Return all tokens
    return tokens;
}

int main(){
    // Bind UDP socket for main server from Beej's Guide
    // AF_INET - use IPv4 addresses
    // SOCK_DGRAM - use UDP 
    // 0 - protocol chosen based on socket type
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    // sockaddr_in - a struct defined in <netinet/in.h> to represent an IPv4 address and initiliaze at 0 from Beej's Guide 
    sockaddr_in addr{};
    // Specifies that this socket will use IPv4 addressing from Beej's Guide
    addr.sin_family=AF_INET; 
    // Sets the IP address that the main server will use from Beej's Guide
    // htonl() - converts a 32-bit value from host byte order to network byte order from Beej's Guide
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK); 
    // Sets the port number that the main server will listen on from Beej's Guide
    // htons() - converts a 16-bit integer from host byte order to network byte order from Beej's Guide
    addr.sin_port=htons(PORT_MAIN);
    // Associates the socket (sock) with the specified local IP address and port (addr) from Beej's Guide
    bind(sock, (sockaddr*)&addr, sizeof(addr));

    // Prints the required startup message to the terminal from instructions
    // Confirms that the main server’s UDP socket has successfully been created and bound to its port
    cout << "Main server is up and running.\n";

    // Phase 1: request player lists from servers A, B, and C using previous helper function sendTo()
    // "LIST?" - the request string asking for the player list of each server
    sendTo(sock, PORT_A, "LIST?");
    sendTo(sock, PORT_B, "LIST?");
    sendTo(sock, PORT_C, "LIST?");

    // Creates a hash map called playerServerMap that stores the mapping between the player names and their game server from Geeksforgeeks and CPlusPlus
    unordered_map<string, char> playerServerMap;

    // This variable keeps track of how many game servers have sent their messages so far and increment each time a valid player list is received
    int playerListsReceived = 0;
    // Creates a character array buffe from Beej's Guide
    char buf[BUFFERSIZE];
    // Loop runs until the main server receives 3 valid responses one from each game server
    // Each time a message is successfully received and processed, playerListsReceived is incremented by one
    while(playerListsReceived < 3) {
        // Creates a socket address structure to store information about who sent the message from Beej's Guide
        sockaddr_in gameSeverAddr{}; 
        // Stores the size of the address structure from Beej's Guide
        socklen_t gameSeverAddrLen = sizeof(gameSeverAddr);
        // recvfrom() from Beej's Guide - UDP receive function
        // It waits for a message to arrive on the socket (sock) and writes the received data into buf
        // It also fills in gameSeverAddr with the sender’s IP and port
        // Returns the number of bytes received (bytesReceived), or -1 if there was an error from Beej's Guide
        ssize_t bytesReceived = recvfrom(sock, buf, BUFFERSIZE - 1, 0, (sockaddr*)&gameSeverAddr, &gameSeverAddrLen);
        // Checks if recvfrom() received a valid message, if it is 0 or negative then an error or no bytes were received
        if(bytesReceived <= 0) continue;
        // Adds a null terminator from Beejs' Guide
        buf[bytesReceived] = '\0'; 
        // Converts buf into a C++ string
        string receivedMessage(buf); 
        // Removes extra whitespace, commas, or newline characters from the beginning or end of the message
        trim(receivedMessage);

        // Extracts the UDP source port number from the sockaddr_in structure from Beej's Guide recvfrom() example
        // gameSeverAddr.sin_port -  contains the sender’s port number, but it’s stored in network byte order
        // ntohs() from Beej's Guide - converts the port number from network byte order
        int fromPort = ntohs(gameSeverAddr.sin_port);
        // Figures out which game server sent the message based on the port number
        char gameServerID = (fromPort == PORT_A) ? 'A': (fromPort == PORT_B) ? 'B' : 'C';

        // Checks whether the received message starts with the text "PLAYERS,"
        if(receivedMessage.rfind("PLAYERS,", 0) == 0) {
            // Prints out message on terminal
            cout << "Main server has received the player list from Game server " << gameServerID
                 << " using UDP over port " << PORT_MAIN << "\n";

            // Split the received message into tokens separated by commas
            auto messageTokens = split(receivedMessage, ',');
            // Create a vector to store all player names extracted from this message
            vector<string> playerNames;
            // Loop over each player name in the message
            for (size_t i=1; i < messageTokens.size(); ++i) { 
                // Extract the current player name token
                string currentPlayerName = messageTokens[i]; 
                // Remove any spaces, newline, or carriage return characters from the name
                trim(currentPlayerName); 
                // Add to the player list
                playerNames.push_back(currentPlayerName); 
            }

            // Loop through each player name received from the current game server
            for (auto &currentPlayerName : playerNames) 
                // Map this player's name to the game server that sent it
                playerServerMap[currentPlayerName] = gameServerID;
            // Increase the count of how many game server
            playerListsReceived++;
        }
    }

    {
        // Print the header for game server A's player list
        cout << "Server A\n";
        // Control comma placement between player names
        bool first = true;

        // Loop through all player–server pairs stored in playerServerMap
        for (auto &playerServerPair : playerServerMap) 
            // Check if this player belongs to game server A
            if (playerServerPair.second == 'A') {
                // If this is the first player printed, don't print a comma
                // oOtherwise, print ", " before the name to separate entries
                cout << (first ? "" : ", ") << playerServerPair.first; 
                first=false;
        }
        // End the line after listing all server A players
        cout << "\nServer B\n";
        // Reset the comma flag for the next server's list
        first  =true; 
        
        // Loop through all player and print server B's players
        for (auto &playerServerPair : playerServerMap) 
            if (playerServerPair.second == 'B') {
                cout << (first ? "" : ", ") << playerServerPair.first; 
                first = false;
        }
        // End the line and print Server C's header
        cout << "\nServer C\n";

        first=true; 
        // Loop through all player and print server C's players
        for (auto &playerServerPair : playerServerMap) 
            if (playerServerPair.second == 'C'){
                cout << (first ? "" : ", ") << playerServerPair.first; 
                first=false;
        }
        // Print a newline after all three server lists for spacing
        cout << "\n";
    }

    // Infinite loop to continuously handle player queries from the user from Geeksforgeeks
    for (;;) {
        // Ask the user to enter a player's name
        cout << "Enter player name:\n";
        // Read the entire line of user input
        string queriedPlayerName; 
        if (!getline(cin, queriedPlayerName))
            // Exit the loop if input stream closes 
            break; 
            // Remove any trailing spaces, newlines, or commas from the input
        trim(queriedPlayerName);
        // If the input is empty, skip this iteration and ask again
        if (queriedPlayerName.empty()) 
            continue;

        // Look up the entered player name in the player-server map
        auto playerEntry = playerServerMap.find(queriedPlayerName);
        // If the player name is not found in the map, print the error message and restart the loop
        if (playerEntry == playerServerMap.end()) {
            cout << queriedPlayerName << " does not show up in Game servers\n";
            continue;
        }
        // Ask the user to enter which column they want to drop a token into
        cout << "Enter column to drop a token in (1,2,3,4,5,6,7):\n";
        // Create a string variable to store the user's input for the column number
        string columnInputString;
        // Read the entire line of user input into columnInputString
        if (!getline(cin,columnInputString)) 
            // If input fails break out of the main query loop
            break; 
        // Clean up any spaces, newline, or carriage return characters around the input
        trim(columnInputString);
        // If the user just pressed Enter without typing a number, skip this iteration and go back to asking for a new player name
        if (columnInputString.empty()) 
            continue;
        // Convert the string input
        int selectedColumn = stoi(columnInputString);

        // Extract the game server ID associated with the player
        char gameServerID = playerEntry->second;
        // Determine which UDP port to contact based on the game server ID
        int targetPort = (gameServerID == 'A') ? PORT_A : (gameServerID =='B') ? PORT_B : PORT_C;
        // Print a message showing which game server the queried player belongs to
        cout << queriedPlayerName << " shows up in server " << gameServerID << "\n";
        // Print a message confirming that the main server has sent the query request
        cout << "The Main Server has sent query for " << queriedPlayerName 
             << " player and dropping a token in column " << selectedColumn 
             << " to server " << gameServerID 
             << " using UDP over port " << PORT_MAIN << "\n";
        // // Build the query message string that will be sent to the game server
        string queryMessage = "QUERY," + queriedPlayerName + "," + to_string(selectedColumn);
        // Send the query message to the game serve using UDP
        sendTo(sock, targetPort, queryMessage);

        // Create a structure to store the address information of the game server
        // Will hold IP address and port info
        sockaddr_in gameSeverAddr{}; 
        // Create a variable to hold the size of the gameServerAddr structure
        socklen_t gameSeverAddrLen = sizeof(gameSeverAddr);
        // Receive a UDP message reply from any of the game servers
        ssize_t bytesReceived = recvfrom(sock, buf, BUFFERSIZE - 1, 0, (sockaddr*)&gameSeverAddr, &gameSeverAddrLen);
        // If no data was received or an error occurred (n <= 0), skip this iteration and keep waiting
        if (bytesReceived <= 0) 
            continue; 
        // Manually add a null terminator at the end of the received message
        buf[bytesReceived]='\0'; 
        // Convert the raw character buffer into a C++ string
        string responseMessage(buf); 
        // Clean up the string by trimming spaces, commas, and newline characters
        trim(responseMessage);

        // Print message on terminal that the main server has successfully received a reply
        cout << "The Main server has received results of " << queriedPlayerName
             << " dropping a token in column " << selectedColumn 
             << " from Game server " << gameServerID << "\n";

        // Check if the game server’s reply message starts with "OK,"
        // rfind("OK,", 0) == 0 means the substring "OK," appears at the very beginning of the string from Geekforgeeks
        // This indicates that the move was successful, and valid data follows in the message
        if (responseMessage.rfind("OK,", 0) == 0) {
            // Split the same server's response by commas into separate parts
            auto responseTokens = split(responseMessage, ',');
            // Extract the slot number where the token was placed
            int slotUsed  = (responseTokens.size() >= 3) ? stoi(responseTokens[2]) : -1;
            // Extract how many slots are left in that column after the move
            int slotsLeft = (responseTokens.size() >= 4) ? stoi(responseTokens[3]) : -1;
            // Display the success message to the user
            cout << "You successfully dropped a token in the game in slot " 
                 << slotUsed << ".\n";
             cout << "There are " << slotsLeft 
                  << " slots left in that column.\n";
}
// If the Game Server sent back "ERR,COLUMN_FULL", it means the requested column is already full and no more tokens can be placed there
else if (responseMessage == "ERR,COLUMN_FULL") {
    cout << "Column " << selectedColumn << " is full.\n";
}
// If the Game Server sent back "ERR,NOT_TURN", it means that it’s not currently this player’s turn to make a move
else if (responseMessage == "ERR,NOT_TURN") {
    cout << "It is not your turn. Try again later.\n";
}
// If the Game Server sent a message that doesn’t match any expected format, treat it as an unexpected or invalid response
else {
    cout << "It is not your turn. Try again later.\n";
}
// After displaying any result or error message, print a separator line to indicate that the main server is ready for another query
cout << "-----Start a new query-----\n";
    }
    // Close UDP socket
    close(sock);
    return 0;
}
