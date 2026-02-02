all: servermain serverA serverB serverC

servermain: servermain.cpp
	g++ -std=c++11 -o servermain servermain.cpp

serverA: serverA.cpp
	g++ -std=c++11 -o serverA serverA.cpp

serverB: serverB.cpp
	g++ -std=c++11 -o serverB serverB.cpp

serverC: serverC.cpp
	g++ -std=c++11 -o serverC serverC.cpp

clean:
	rm -f servermain serverA serverB serverC