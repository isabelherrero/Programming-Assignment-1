// For input/output
#include <iostream>
// For reading files (list.txt)
#include <fstream>
//For parsing strings with stringstream
#include <sstream>
#include <string>
// For using std::map
#include <map>
// For using std::set
#include <set>
// For using std::vector
#include <vector>
// For memset
#include <cstring>
// For algorithms all_of
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
// For handling signals
#include <signal.h>
#include <sys/wait.h>
// For exit() 
#include <cstdlib>

using namespace std;

// TCP port number for the main server
#define PORT "24619"
// Max number of pending client connections from Beej's Guide
#define BACKLOG 10 
// Max buffer size 1024
#define BUFFERSIZE 1024 

// Structure to store each player's information
struct playerInfo {
    // Name of the opponent in a string
    string opponent;
    // The ID of the game server the player is in as an interger
    int gameServerID;
};

// This makes a map where each player's name is linked to their information that is stored in playerInfo
// Idea from Geeksforgeeks and cplusplus
map<string, playerInfo> playerMap;
// The map connects a game server ID to a set of player's names
map<int, set<string>> gameServerPlayers;
// This stores the game server IDs
set<int> gameServerIDs;

// Handles child process termination to avoid zombies processes from Beej's Guide, Geeksforgeeks, and UNIX&LINUX
// When a child process finishes, it sends a sigchld signal to parent to handle the signal
void sigchld_handler(int s) {
    // Cleans up all finished child processes
    // Avoid dead processes piling up 
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

// Returns a pointer to a general socket address from Beej's Guide
// It can point to either an IPv4 or IPv6 address
void* get_in_addr(struct sockaddr* sa) {
    // Then it tells what kind of address it is AF_INET being IPv4
    if (sa->sa_family == AF_INET)
        // If it is IPv4 then it returns the address part sin_addr
        return &(((struct sockaddr_in*)sa)->sin_addr);
    // If it is IPv6 then it returns sin6_addr
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Reads player and server information from list.txt function
void readListFile() {
    // This open list.txt for reading used from previous project
    ifstream infile("list.txt");

    // Declare variable playerNameLine as a string, will be used for getting the player's name on the line of list.txt
    string playerNameLine;
    // Tracks current game server ID while reading list.txt
    int currentGameID = -1;

    // Start a loop to read list.txt line by line
    while (getline(infile, playerNameLine)) {
        // Removes all possible spaces
        playerNameLine.erase(
            // Rearranges elements by moving all elements that don't meet the criteria to the front
            remove_if(playerNameLine.begin(), playerNameLine.end(), ::isspace),
            // Return kept elements
            playerNameLine.end()
        );
        // First check the line without semicolon because there are player that have no opponent therefore no semicolon after their name
        if (playerNameLine.find(';') == string::npos) {
            // Check if the line is a number that would be the game server id
            // all_of from geeksforgeeks
            if (all_of(playerNameLine.begin(), playerNameLine.end(), ::isdigit)) {
                // Then convert the string to an interger
                currentGameID = stoi(playerNameLine);
                // Adds to the set of game server IDs
                gameServerIDs.insert(currentGameID);
            } else {
                // Single player without opponent
                string singlePlayer = playerNameLine;
                // This means there is no opponent and gets game server ID
                playerMap[singlePlayer] = {"", currentGameID};
                // Then adds player to the server's player list
                gameServerPlayers[currentGameID].insert(singlePlayer);
            }
        } else {
            // Checks the line with semicolon meaning the player has an opponent
            stringstream ss(playerNameLine);
            string player1, player2;
            // Reads for player1 before semicolon
            getline(ss, player1, ';');
            // Reads for player2 after semicolon
            getline(ss, player2, ';');

            // Remove whitespaces between players so can be read
            player1.erase(remove_if(player1.begin(), player1.end(), ::isspace), player1.end());
            player2.erase(remove_if(player2.begin(), player2.end(), ::isspace), player2.end());

            // Add both players to the map and server list
            playerMap[player1] = {player2, currentGameID};
            // Player1 is stored with their opponent and server
            gameServerPlayers[currentGameID].insert(player1);

            // If player2 exists then it adds to list as player1 opponent
            if (!player2.empty()) {
                playerMap[player2] = {player1, currentGameID};
                gameServerPlayers[currentGameID].insert(player2);
            }
        }
    }

    // Close list.txt
    infile.close();

    // Display messages according to instructions
    cout << "Main server is up and running." << endl;
    cout << "Main server has read the player list from list.txt." << endl;

    // Print how many game servers and players are registered in list.txt
    cout << "Total number of Game Servers: " << gameServerIDs.size() << endl;
    for (int id : gameServerIDs) {
        int count = gameServerPlayers[id].size();
        cout << "Game Server " << id << " contains " << count << " player"
             << (count > 1 ? "s" : "") << endl;
    }
}

int main() {
    // Load and display player list
    readListFile();

    // Listening socket and new connection socket
    // This section from Beej's Guide
    int sockfd, new_fd; 
    struct addrinfo hints{}, *servinfo, *p;
    // Client’s address information
    struct sockaddr_storage their_addr{}; 
    socklen_t sin_size;
    struct sigaction sa{};
    int yes = 1;
    // To store IP address string
    char s[INET6_ADDRSTRLEN];

    // Setup hints for getaddrinfo from Beej's Guide
    // Allow IPv4 or IPv6
    hints.ai_family = AF_UNSPEC;
    // TCP socket
    hints.ai_socktype = SOCK_STREAM;
    // Use local IP for server
    hints.ai_flags = AI_PASSIVE;

    // Get address info for localhost and PORT from Beej's Guide
    int rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo);
    if (rv != 0) {
        cerr << "getaddrinfo: " << gai_strerror(rv) << endl;
        return 1;
    }

    // Loop through all the results and bind to the first valid address from Beej's Guide and geekforgeeks
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        // Allow reusing port after program restart from Beej's Guide
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Try to bind the socket from Beej's Guide
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            // Try next possible address
            continue;
        }

        break;
    }

    // If no valid binding was found from Beej's Guide
    if (p == nullptr) {
        cerr << "Main server: failed to bind" << endl;
        return 2;
    }

    // Free linked list memory
    freeaddrinfo(servinfo);

    // Start listening for incoming connections from Beej's Guide
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Set up signal handler to reap dead child processes from Beej's Guide
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Main server loop from Beej's Guide
    // Accept and handle client connections
    while (true) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
        // Try next if accept fails
        if (new_fd == -1) continue;

        // Convert client IP to string from Beej's Guide
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr*)&their_addr),
                  s, sizeof s);

        // Extract client port number that will be used as the client's ID from Beej's Guide and Stack Overflow
        int clientPort = 0;
        if (their_addr.ss_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)&their_addr;
            // nthos from Beej's Guide
            clientPort = ntohs(ipv4->sin_port);
        } else if (their_addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&their_addr;
            clientPort = ntohs(ipv6->sin6_port);
        }

        // Fork a child process to handle each client from Beej's Guide
        if (!fork()) {
            // Child doesn’t need the listening socket
            close(sockfd);

            // Receive player name from client
            char buf[BUFFERSIZE];
            memset(buf, 0, sizeof buf);
            int numbytes = recv(new_fd, buf, sizeof buf - 1, 0);
            if (numbytes == -1) {
                perror("recv");
                exit(1);
            }
            string playerName(buf);

            // Log received request
            cout << "Main server has received the request about Player " << playerName
                 << " from client (ID: " << clientPort << ") using TCP over port " << PORT << endl;

            string reply;

            // Check if player exists
            if (playerMap.find(playerName) != playerMap.end()) {
                int serverID = playerMap[playerName].gameServerID;
                string opponent = playerMap[playerName].opponent;

                if (!opponent.empty()) {
                    reply = playerName + " is associated with Game server " +
                            to_string(serverID) + " and opponent " + opponent + ".";
                    cout << playerName << " shows up in Game server " << serverID
                         << " with opponent " << opponent << endl;
                } else {
                    reply = playerName + " is associated with Game server " +
                            to_string(serverID) + " and no opponent.";
                    cout << playerName << " shows up in Game server " << serverID
                         << " with no opponent" << endl;
                }

                cout << "Main Server has sent searching result to client (ID: " << clientPort
                     << ") using TCP over port " << PORT << endl;
            } else {
                // Player not found in any game server
                stringstream notFoundMsg;
                notFoundMsg << playerName << " does not show up in Game server ";
                for (auto it = gameServerIDs.begin(); it != gameServerIDs.end(); ++it) {
                    notFoundMsg << *it;
                    if (next(it) != gameServerIDs.end()) {
                        notFoundMsg << ", ";
                    }
                }
                cout << notFoundMsg.str() << endl;
                reply = playerName + ": Not found";
                cout << "The Main Server has sent \"Player Name: Not found\" to client (ID: " << clientPort
                     << ") using TCP over port " << PORT << endl;
            }

            // Send reply to client
            if (send(new_fd, reply.c_str(), reply.size(), 0) == -1) {
                perror("send");
            }

            // Close connection
            close(new_fd);
            // Child exits
            exit(0);
        }

        // Parent closes its copy of client socket
        close(new_fd);
    }

    return 0;
}
