all: error_detection
	./error_detection

error_detection: error_detection.cpp
	g++ -std=c++11 -o error_detection error_detection.cpp

clean:
	rm -f error_detection
