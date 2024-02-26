CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

all: main

main: proxy.o main.o HTTPRequestParser.o HTTPResponseParser.o helperFns.o
	$(CXX) $(CXXFLAGS) -o main proxy.o main.o HTTPRequestParser.o HTTPResponseParser.o helperFns.o

proxy.o: proxy.cpp proxy.h HTTPRequestParser.h HTTPResponseParser.h helperFns.h user.h HTTPCache.h
	$(CXX) $(CXXFLAGS) -c proxy.cpp

main.o: main.cpp proxy.h helperFns.h
	$(CXX) $(CXXFLAGS) -c main.cpp

HTTPRequestParser.o: HTTPRequestParser.cpp HTTPRequestParser.h
	$(CXX) $(CXXFLAGS) -c HTTPRequestParser.cpp

HTTPResponseParser.o: HTTPResponseParser.cpp HTTPResponseParser.h
	$(CXX) $(CXXFLAGS) -c HTTPResponseParser.cpp

helperFns.o: helperFns.cpp HTTPResponseParser.h HTTPRequestParser.h proxy.h user.h
	$(CXX) $(CXXFLAGS) -c helperFns.cpp

clean:
	rm -f *.o main