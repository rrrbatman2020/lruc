CXX = g++
CXXFLAGS += -O3 -Wall -DNDEBUG

all: output

output: main.o http_response_parser.o http_file_downloader.o http_request_builder.o http_connection.o tcp_connection.o error.o
	$(CXX) $(CXXFLAGS) -std=c++17 main.o http_response_parser.o http_file_downloader.o http_request_builder.o tcp_connection.o http_connection.o error.o -o lruc

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c main.cpp

http_response_parser.o: http_response_parser.h http_response_parser.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c http_response_parser.cpp

http_request_builder.o: http_request_builder.h http_request_builder.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c http_request_builder.cpp

http_file_downloader.o: http_file_downloader.h http_file_downloader.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c http_file_downloader.cpp

http_connection.o: http_connection.h http_connection.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c http_connection.cpp

tcp_connection.o: tcp_connection.h tcp_connection.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c tcp_connection.cpp

error.o: error.h error.cpp
	$(CXX) $(CXXFLAGS) -std=c++17 -c error.cpp

clean:
	rm -rf *.o lruc
