CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

all: main

main: proxy.o main.o HTTPRequestParser.o HTTPResponseParser.o
	$(CXX) $(CXXFLAGS) -o main proxy.o main.o HTTPRequestParser.o HTTPResponseParser.o

proxy.o: proxy.cpp proxy.h HTTPRequestParser.h HTTPResponseParser.h
	$(CXX) $(CXXFLAGS) -c proxy.cpp

main.o: main.cpp proxy.h
	$(CXX) $(CXXFLAGS) -c main.cpp

HTTPRequestParser.o: HTTPRequestParser.cpp HTTPRequestParser.h
	$(CXX) $(CXXFLAGS) -c HTTPRequestParser.cpp

HTTPResponseParser.o: HTTPResponseParser.cpp HTTPResponseParser.h
	$(CXX) $(CXXFLAGS) -c HTTPResponseParser.cpp

clean:
	rm -f *.o main