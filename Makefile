all: serverA serverB servermain client

serverA: serverA.cpp
	g++ -std=c++11 -o serverA serverA.cpp

serverB: serverB.cpp
	g++ -std=c++11 -o serverB serverB.cpp

servermain: servermain.cpp
	g++ -std=c++11 -o servermain servermain.cpp

client: client.cpp
	g++ -std=c++11 -o client client.cpp

clean:
	rm -f serverA serverB servermain client
