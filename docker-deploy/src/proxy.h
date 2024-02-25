#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "HTTPCache.h"

class Proxy{
public:
    int port_num;


    Proxy():port_num(0){};
    Proxy(int Portnum):port_num(Portnum){};
    ~Proxy(){};

    // Return ip
    const char* get_in_addr(struct sockaddr *sa, char* s, size_t maxlen) {
        if (sa->sa_family == AF_INET) { // IPv4
            struct sockaddr_in *addr_in = (struct sockaddr_in *)sa;
            return inet_ntop(AF_INET, &addr_in->sin_addr, s, maxlen);
        } else { // IPv6
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)sa;
            return inet_ntop(AF_INET6, &addr_in6->sin6_addr, s, maxlen);
        }
    }

    int setupServer(const char* port);
    int setupClient(const char* host, const char* port);
    
    void processConnect(int client_fd, int server_fd);
    void processPost(int client_fd, int server_fd, std::string requestStr);
    void processGet(int client_fd, int server_fd, std::string host, std::string port, std::string request);
    void processRequest(const char* port);

    void parseCacheControlAndExpires(const std::string &cacheControl, const std::string &expiresHeader, CacheEntry &response);

};