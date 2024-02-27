#ifndef PROXY_H
#define PROXY_H


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
#include "user.h"

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
    int acceptUser(int self_socket_fd, std::string & client_ip);
    int setupClient(const char* host, const char* port);
    
    void processConnect(int client_fd, int server_fd, User * user);
    void processPost(int client_fd, int server_fd, std::string requestStr, User * user);
    void processGet(int client_fd, int server_fd, std::string host, std::string port, std::string request, User * user);
};

#endif