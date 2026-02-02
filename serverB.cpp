// for all standard C++ libraries, got it from Geeksforgeeks
#include <bits/stdc++.h>

// header files from instructions, reused from previous project part 1
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

// used from previous project part 1
using namespace std;

#define SERVER_ID 'B'
// UPD port definition for game server B
#define PORT_B 31619 
// UPD port definition for main server
#define PORT_MAIN 33619
// Max buffer size 1024
#define BUFFERSIZE 1024

// Structure that stores the game state
struct GameState {
    // Stores names of both players
    string playerOneName, playerTwoName;  
    // 0 means it's player 1's turn, 1 means player 2's turn          
    int nextTurn = 0; 
    // Tracks available slots in each of the 7 columns         
    int availableSlotsPerColumn[7];            
};

// Create one global gameSate variable for this server
static GameState gameState;

// Helper function to remove unnecessary characters
static void trim(string &inputString){
    // Remove unwanted characters from the end of the string
    while(!inputString.empty() && (inputString.back()=='\n' || inputString.back()=='\r' || inputString.back()==' ' || inputString.back()==',')) inputString.pop_back();
    // Remove spaces from the beginning of the string
    size_t i = 0; 
    while(i < inputString.size() && (inputString[i] == ' ')) i++; 
    if(i) inputString.erase(0, i);
}


// Helper function to split strings by a given delimiter
static vector<string> split(const string &inputString, char delimiter){
    // Stores separated pieces of text
    vector<string> tokens; 
    // Temporarily stores current token
    string currentToken; 
    // Go through each character in the string
    for(char currentChar: inputString) { 
        // If the character matches the delimiter
        if (currentChar == delimiter) { 
            // Add the token to the vector
            tokens.push_back(currentToken); 
            // Clear current token for next one
            currentToken.clear(); 
        } 
        // Add character to the current token
        else currentToken.push_back(currentChar); 
    }
    // Add the last token after the loop ends
    tokens.push_back(currentToken); 
    // Return the vector of separated tokens
    return tokens;
}

// Reads the data file and loads initial game information
static void loadFile(const string &fileName){
    // Initialize all columns with 6 available slots
    for(int i = 0; i < 7; i++) gameState.availableSlotsPerColumn[i] = 6;
    // Start with player 1's turn
    gameState.nextTurn = 0;

    // Open the data file for reading
    ifstream fin(fileName);
    string fileLine;
    // Read the first line which contains the player names
    getline(fin, fileLine);
    trim(fileLine);
    auto playerNames = split(fileLine, ',');
    if(playerNames.size() >= 2) {
        // First player
        gameState.playerOneName = playerNames[0]; 
        trim(gameState.playerOneName);
        // Second player
        gameState.playerTwoName = playerNames[1]; 
        trim(gameState.playerTwoName);
    }

    // Counter to determine which player's turn is next
    int moveIndex = 0;

    // Read the rest of the file for move data
    while(getline(fin, fileLine)){
        trim(fileLine);
        // Split line by commas
        auto moveTokens = split(fileLine, ',');
        if(moveTokens.size() >= 3) {
            // Column number
            int columnIndex = stoi(moveTokens[1]);
            // Remaining slots
            int availableSlots = stoi(moveTokens[2]);
            // Check valid column index
            if(0 <= columnIndex && columnIndex < 7) {
                // Update slot availability
                gameState.availableSlotsPerColumn[columnIndex] = availableSlots;
            }
            // Increment move count
            moveIndex++;
        }
    }
     // Decide whose turn is next (if move count is even, player 1's turn, else player 2's)
    gameState.nextTurn = (moveIndex % 2 == 0) ? 0 : 1;
}

int main(){
     // Load the data file for server B
    loadFile("dataB.txt");

    // Create UDP socket for server B from Beej's Guide
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{}; 
    // Use IPv4
    addr.sin_family = AF_INET;
    // Localhost address
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    // Server B’s port
    addr.sin_port = htons(PORT_B);
    // Bind socket to this address
    bind(sock, (sockaddr*)&addr, sizeof(addr));

    // Print startup message
    cout << "Server A is up and running using UDP on port " << PORT_B << "\n";

    // Main loop to receive and handle messages from main server
    char buf[BUFFERSIZE];
    for(;;) {
        sockaddr_in senderAddress{}; 
        socklen_t senderAddressLen = sizeof(senderAddress);
        ssize_t bytesReceived = recvfrom(sock, buf, BUFFERSIZE - 1, 0, (sockaddr*)&senderAddress, &senderAddressLen);
        // Skip if no data received
        if(bytesReceived <= 0) 
            continue;
        // Null-terminate message
        buf[bytesReceived] = '\0';
        // Convert to string
        string receivedMessage(buf); 
        trim(receivedMessage);

        // Handle LIST? request from main server
        if(receivedMessage=="LIST?"){
            // Build and send the player list message
            string responseMessage = "PLAYERS," + gameState.playerOneName + "," + gameState.playerTwoName;
            sendto(sock, responseMessage.c_str(), responseMessage.size(), 0, (sockaddr*)&senderAddress, senderAddressLen);
            cout << "Server B has sent a player list to Main Server\n";
            // Wait for next message
            continue;
        }

        // Handle query request
        if(receivedMessage.rfind("QUERY,", 0) == 0) {
            auto queryTokens = split(receivedMessage, ',');
            if(queryTokens.size() >= 3) {
                // Extract player name
                string queriedPlayerName = queryTokens[1]; 
                trim(queriedPlayerName);
                // Column number (1–7)
                int columnIndex = stoi(queryTokens[2]); 
                // Convert to 0 based index
                int selectedColumnIndex = columnIndex-1;

                cout << "Server B has received a request for player " << queriedPlayerName
                     << " to drop a token in column " << columnIndex << "\n";

                // Check if the player belongs to this server
                bool isplayer1Name = (queriedPlayerName == gameState.playerOneName);
                bool isplayer2Name = (queriedPlayerName == gameState.playerTwoName);
                if(!(isplayer1Name || isplayer2Name)){
                    // Invalid player
                    string err = "ERR,NOT_OURS";
                    sendto(sock, err.c_str(), err.size(), 0, (sockaddr*)&senderAddress, senderAddressLen);
                    continue;
                }

                 // Verify if it's this player's turn
                 // 0 for player1 and 1 for player2
                int expectedTurn = gameState.nextTurn; 
                if((isplayer1Name && expectedTurn != 0) || (isplayer2Name && expectedTurn != 1) ){
                    string err = "ERR,NOT_TURN";
                    cout << "Server B found 0 available slots in column " << columnIndex << ".\n";
                    sendto(sock, err.c_str(), err.size(), 0, (sockaddr*)&senderAddress, senderAddressLen);
                    cout << "Server B has sent the results to Main Server\n";
                    continue;
                }

                // Check if column is full
                if(selectedColumnIndex  <0 || selectedColumnIndex >= 7 || gameState.availableSlotsPerColumn[selectedColumnIndex] == 0){
                    string err = "ERR,COLUMN_FULL";
                    sendto(sock, err.c_str(), err.size(), 0, (sockaddr*)&senderAddress, senderAddressLen);
                    cout << "Server B has sent the results to Main Server\n";
                    continue;
                }

                // Compute the slot number being used
                // Slot index (top to bottom)
                int slotUsed = 7 - gameState.availableSlotsPerColumn[selectedColumnIndex];
                // Update remaining slots from Geeksforgeeks
                gameState.availableSlotsPerColumn[selectedColumnIndex] -= 1; 
                // Switch turns from CPlusPlus            
                gameState.nextTurn ^= 1;

                cout << "Server B found " << gameState.availableSlotsPerColumn[selectedColumnIndex] 
                     << " available slots in column " << columnIndex << ".\n";

                // Build OK response message
                string successResponseMessage = "OK," + to_string(columnIndex) + "," 
                    // Actual slot filled
                    + to_string(slotUsed) + ","   
                    // Remaining slots
                    + to_string(gameState.availableSlotsPerColumn[selectedColumnIndex]); 

                // Send back to Main Server
                sendto(sock, successResponseMessage.c_str(), successResponseMessage.size(), 0, (sockaddr*)&senderAddress, senderAddressLen);
                cout << "Server B has sent the results to Main Server\n";
            }
        }
    }
    // Close socket
    close(sock);
    return 0;
}