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
// For std::map
#include <map>
// For std::string
#include <string>
// For string streams
#include <sstream>
// For std::vector
#include <vector>
// For file input and output
#include <fstream>

// Use standard namespace to avoid typing std:: everywhere
using namespace std;

// From instructions, previous project and Beej's Guide
// Game Server A's UDP port
#define SERVERA_UDP_PORT 31619
// UDP port for communication with game servers
#define MAIN_SERVER_UDP_PORT 34619
// Localhost IP address
#define LOCALHOST "127.0.0.1"
// Maximum buffer length for messages, made it four times bigger than 1024 just in case
#define MAXBUFLEN 4096

// Struct to store the current game state for Server A from previous project
struct GameState {
    // Name of player 1
    string player1;
    // Name of player 2
    string player2;
    // Connect 4 board that is 6 by 7
    char gameBoard[6][7];
    // Number of moves that have been made in the game so far
    int moveCount;
    
    // Constructor to initialize the game state
    // Initialize moveCount to 0
    GameState() : moveCount(0) {
        // Loop over each row
        for(int i = 0; i < 6; i++)
            // Loop over each column
            for(int j = 0; j < 7; j++)
                // Set each board position to empty space
                gameBoard[i][j] = ' ';
    }
};

// Game state used by this server
GameState gameState;

// Function to load game data from dataA.txt
void loadDataFile() {
    // Open dataA.txt for reading
    ifstream dataFile("dataA.txt");
    
    // String to store each line read from the file
    string currentLine;
    // Read player names from first line
    if(getline(dataFile, currentLine)) {
        // Remove whitespace
        while(!currentLine.empty() && (currentLine.back() == ' ' || currentLine.back() == '\r' || currentLine.back() == '\n')) {
            currentLine.pop_back();
        }
        
        // Find position of comma separating player names
        size_t commaPosition = currentLine.find(',');
        // Player 1 name is before the comma
        gameState.player1 = currentLine.substr(0, commaPosition);
        // Player 2 name is after the comma
        gameState.player2 = currentLine.substr(commaPosition + 1);
        
        // Trim whitespace from player names
        while(!gameState.player1.empty() && gameState.player1.back() == ' ') {
            gameState.player1.pop_back();
        }
        while(!gameState.player2.empty() && gameState.player2.back() == ' ') {
            gameState.player2.pop_back();
        }
    }
    
    // Read moves from remaining lines
    while(getline(dataFile, currentLine)) {
        // Variables to hold column and height from file
        int boardColumn, stackHeight;
        // Parse line in format "Drop,col,height"
        if(sscanf(currentLine.c_str(), "Drop,%d,%d", &boardColumn, &stackHeight) == 2) {
            // Convert height to row index
            int boardRow = 6 - stackHeight;
            // Determine whose token it is
            // Even moveCount -> 'X'
            // Odd moveCount  -> 'O'
            char playerToken = (gameState.moveCount % 2 == 0) ? 'X' : 'O';
            // Place token in corresponding board cell
            gameState.gameBoard[boardRow][boardColumn-1] = playerToken;
            // Increment total move count
            gameState.moveCount++;
        }
    }
    // Close data file
    dataFile.close();
}

// Function to save current game data back to dataA.txt from previous project
void saveDataFile() {
    // Open dataA.txt for writing
    ofstream dataFile("dataA.txt");
    // Write player1 name, comma, player2 name, and newline
    dataFile << gameState.player1 << "," << gameState.player2 << "\n";
    
    // Reconstruct moves from board
    // Vector to store move lines as strings
    vector<string> recordedMoves;
    // Loop over each column from previous project
    for(int columnIndex = 0; columnIndex < 7; columnIndex++) {
        // Track how many tokens are in this column
        int stackHeight = 0;
        // Scan from bottom row to top
        for(int rowIndex = 5; rowIndex >= 0; rowIndex--) {
            // If there is a token in this cell
            if(gameState.gameBoard[rowIndex][columnIndex] != ' ') {
                // Increase height
                stackHeight++;
                // String stream to build move string
                stringstream ss;
                // Format: Drop,column,height
                ss << "Drop," << (columnIndex+1) << "," << stackHeight;
                // Store move description in vector
                recordedMoves.push_back(ss.str());
            }
        }
    }
    
    // Loop over all reconstructed moves
    for(const auto& moveEntry : recordedMoves) {
        // Write each move as a separate line
        dataFile << moveEntry << "\n";
    }
    // Close the file
    dataFile.close();
}

// Function to build a string representation of the current board from cplusplus, gamedev.net, and daniweb
string getBoardString() {
    // String stream to build the board string
    stringstream ss;
    // Header row with column numbers
    ss << " 1 2 3 4 5 6 7\n";
    // Loop over each row
    for(int rowIndex = 0; rowIndex < 6; rowIndex++) {
        // Start row with left border
        ss << "|";
        // Loop over each column
        for(int columnIndex = 0; columnIndex < 7; columnIndex++) {
            // If cell is empty, show '.'
            // Otherwise, show the token 'X' or 'O'
            char cellSymbol = (gameState.gameBoard[rowIndex][columnIndex] == ' ') ? '.' : gameState.gameBoard[rowIndex][columnIndex];
            // Add cell and right border
            ss << cellSymbol << "|";
        }
        // End of row
        ss << "\n";
    }
    // Return full board as a single string
    return ss.str();
}

// Function to check if the last move resulted in a win from cplusplus and stackoverflow
bool checkWin() {
    // Determine last player's token:
    // If moveCount is even now, last move was 'O'
    // If moveCount is odd now, last move was 'X'
    char lastPlayerTokeb = (gameState.moveCount % 2 == 0) ? 'O' : 'X';
    
    // Loop over all rows
    for(int rowIndex = 0; rowIndex < 6; rowIndex++) {
        // Loop over all columns
        for(int columnIndex = 0; columnIndex < 7; columnIndex++) {
            // Skip cells that don't match last token
            if(gameState.gameBoard[rowIndex][columnIndex] != lastPlayerTokeb) continue;
            
            // Horizontal
            // Ensure we have space for 4 horizontally
            // Check cell to the right 
            if(columnIndex+3 < 7 && gameState.gameBoard[rowIndex][columnIndex+1] == lastPlayerTokeb &&
               // Check two cells to the right
               // Check three cells to the right
               gameState.gameBoard[rowIndex][columnIndex+2] == lastPlayerTokeb && gameState.gameBoard[rowIndex][columnIndex+3] == lastPlayerTokeb)
                // Found 4 in a row horizontally
                return true;
            
            // Vertical
            // Ensure we have space for 4 vertically
            // Check cell below
            if(rowIndex+3 < 6 && gameState.gameBoard[rowIndex+1][columnIndex] == lastPlayerTokeb && 
                // Check two cells below
                // Check three cells below
               gameState.gameBoard[rowIndex+2][columnIndex] == lastPlayerTokeb && gameState.gameBoard[rowIndex+3][columnIndex] == lastPlayerTokeb)
                // Found 4 in a row vertically
                return true;
            
            // Diagonal down-right
            // Ensure we have space down-right
            // Check one down, one right
            if(rowIndex+3 < 6 && columnIndex+3 < 7 && gameState.gameBoard[rowIndex+1][columnIndex+1] == lastPlayerTokeb && 
                // Check two down, two right
                // Check three down, three right
               gameState.gameBoard[rowIndex+2][columnIndex+2] == lastPlayerTokeb && gameState.gameBoard[rowIndex+3][columnIndex+3] == lastPlayerTokeb)
                // Found 4 in a row diagonal
                return true;
            
            // Diagonal down-left
            // Ensure we have space down-left
            // Check one down, one left
            if(rowIndex+3 < 6 && columnIndex-3 >= 0 && gameState.gameBoard[rowIndex+1][columnIndex-1] == lastPlayerTokeb && 
                // Check two down, two left
                // Check three down, three left
               gameState.gameBoard[rowIndex+2][columnIndex-2] == lastPlayerTokeb && gameState.gameBoard[rowIndex+3][columnIndex-3] == lastPlayerTokeb)
                // Found 4 in a row diagonal
                return true;
        }
    }
    // No winning pattern found
    return false;
}

// Function to apply a move to the board in the given column from cplusplus and stackoverflow
int makeMove(int boardColumn) {
    // If column is out of range, return error
    if(boardColumn < 1 || boardColumn > 7) return -1;
    // Convert 1-based column index to 0-based
    boardColumn--;
    
    // If top cell is not empty, column is full
    if(gameState.gameBoard[0][boardColumn] != ' ') return -1;
    
    // Start from bottom row and go up
    for(int rowIndex = 5; rowIndex >= 0; rowIndex--) {
        // Find the first empty cell in this column
        if(gameState.gameBoard[rowIndex][boardColumn] == ' ') {
            // Determine which player's token to drop
            char playerToken = (gameState.moveCount % 2 == 0) ? 'X' : 'O';
            // Place token in the board
            gameState.gameBoard[rowIndex][boardColumn] = playerToken;
            // Increase move count
            gameState.moveCount++;
            // Save updated game state to file
            saveDataFile();
            // Return the row index where token was placed
            return rowIndex;
        }
    }
    return -1;
}

// Main function: entry point of Server A from Beej's Guide and previous project
int main() {
    // Socket file descriptor for UDP
    int sockfd;
    // Address structures for this server and main server
    struct sockaddr_in servaddr, mainserver_addr;
    // Buffer for receiving and sending messages
    char buf[MAXBUFLEN];
    
    // Create UDP socket from Beej's Guide and previous project
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        // Print error if socket creation fails
        perror("socket creation failed");
        // Exit with error code
        exit(1);
    }
    
    // Zero out the server address structure from Beej's Guide and previous project
    memset(&servaddr, 0, sizeof(servaddr));
    // Use IPv4
    servaddr.sin_family = AF_INET;
    // Bind to localhost IP
    servaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Bind to Server A's UDP port
    servaddr.sin_port = htons(SERVERA_UDP_PORT);
    
    // Bind the socket to the address and port
    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        // Print error if bind fails
        perror("bind failed");
        // Exit with error code
        exit(1);
    }
    
    // Print that Server A is running and its port
    printf("The Server A is up and running using UDP on port %d.\n", SERVERA_UDP_PORT);
    
    // Load game data
    // Read initial game state from dataA.txt
    loadDataFile();
    
    // Main server address
    // Zero out main server address structure
    memset(&mainserver_addr, 0, sizeof(mainserver_addr));
    // Use IPv4
    mainserver_addr.sin_family = AF_INET;
    // Main server is also on localhost
    mainserver_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    // Main server's UDP port
    mainserver_addr.sin_port = htons(MAIN_SERVER_UDP_PORT);
    
    // Infinite loop to handle incoming requests
    while(1) {
        // Address of the client
        struct sockaddr_in client_addr;
         // Length of the client address structure
        socklen_t addressLength = sizeof(client_addr);
        
        // Receive a UDP message from main server from Beej's Guide and previous project
        int bytesReceived = recvfrom(sockfd, buf, MAXBUFLEN, 0, (struct sockaddr *)&client_addr, &addressLength);
        // Check for receive error
        if(bytesReceived < 0) {
            // Print error message
            perror("recvfrom failed");
            // Continue to next iteration to keep server running
            continue;
        }

        // Null terminate the received data
        buf[bytesReceived] = '\0';
        
        // Convert received buffer to C++ string
        string clientRequestMessage(buf);
        // String to hold response message
        string serverReplyMessage;
        
        // If request is for player list
        if(clientRequestMessage == "PLAYER_LIST") {
            // Send player list
            // Build response as "player1,player2"
            serverReplyMessage = gameState.player1 + "," + gameState.player2;
            // Send response via UDP from Beej's Guide and previous project
            sendto(sockfd, serverReplyMessage.c_str(), serverReplyMessage.length(), 0, 
                   (const struct sockaddr *)&client_addr, addressLength);
            // Log action
            printf("Server A has sent the player list to Main Server.\n");
        }
        // If request is a VIEW request
        else if(clientRequestMessage.substr(0, 4) == "VIEW") {
            // Log receipt
            printf("Server A has received a view request to send current board state.\n");
            // Build response starting with "BOARD:" and board string
            serverReplyMessage = "BOARD:" + getBoardString();
            // Determine whose currentPlayerToken it is next
            char currentPlayerToken = (gameState.moveCount % 2 == 0) ? 'X' : 'O';
            // Append text about current player's turn
            serverReplyMessage += "\nCurrent turn: Player " + string(1, currentPlayerToken);
            // Send board state back to main server
            sendto(sockfd, serverReplyMessage.c_str(), serverReplyMessage.length(), 0, 
                   (const struct sockaddr *)&client_addr, addressLength);
            // Log action
            printf("Server A sends the current board state to Main Server.\n");
        }
        // If request is a MOVE request
        else if(clientRequestMessage.substr(0, 4) == "MOVE") {
            // Column to drop token into
            int boardColumn;
            // Parse column from request string
            sscanf(clientRequestMessage.c_str(), "MOVE:%d", &boardColumn);
            // Log the move
            printf("Server A has received a move request for column %d.\n", boardColumn);
            
            // Attempt to make the move
            int moveResultCode = makeMove(boardColumn);
            // If move is invalid
            if(moveResultCode == -1) {
                // Build invalid move message
                serverReplyMessage = "INVALID:Move to column " + to_string(boardColumn) + " is invalid.";
                // Log invalid move
                printf("Move to column %d is invalid in Server A.\n", boardColumn);
            // Move was accepted
            } else {
                // Build accepted response with new board
                serverReplyMessage = "ACCEPTED:" + getBoardString();
                // Check if this move caused a win
                if(checkWin()) {
                    // Append win message if there is a winner
                    serverReplyMessage += "\nWIN:Player wins!";
                }
                // Log success
                printf("Server A accepted move in column %d. Board state updated.\n", boardColumn);
            }
            
            // Send move result and updated board to main server
            sendto(sockfd, serverReplyMessage.c_str(), serverReplyMessage.length(), 0, 
                   (const struct sockaddr *)&client_addr, addressLength);
            // Log sending response
            printf("Server A has sent move result and updated board to Main Server.\n");
        }
    }
    
    // Close the UDP socket
    close(sockfd);
    return 0;
}