all: servermain client

servermain: servermain.cpp
	g++ -std=c++11 -o servermain servermain.cpp

client: client.cpp
	g++ -std=c++11 -o client client.cpp

clean:
	rm -f servermain client
