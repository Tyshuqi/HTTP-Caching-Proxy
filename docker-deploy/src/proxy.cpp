#include "proxy.h"
#include "HTTPRequestParser.h"
#include "HTTPResponseParser.h"
#include "helperFns.h"
#include "HTTPCache.h"
#include "user.h"

// #define PORT "12345"  // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 65536

using namespace std;

int Proxy::setupServer(const char *port)
{
    int self_socket_fd;             // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo;   // *p;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        cerr << "server: getaddrinfo:" << gai_strerror(rv) << endl;
        exit(EXIT_FAILURE);
    }

    if ((self_socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        cerr << "server: socket" << endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(self_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        cerr << "server: setsockopt" << endl;
        exit(EXIT_FAILURE);
    }

    if (bind(self_socket_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        cerr << "server: bind" << endl;
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(self_socket_fd, BACKLOG) == -1)
    {
        cerr << "server: listen" << endl;
        exit(EXIT_FAILURE);
    }

    return self_socket_fd;
}

int Proxy::acceptUser(int self_socket_fd){
    char s[INET6_ADDRSTRLEN];
    int client_fd;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size = sizeof(their_addr);
    //*********** accept **********//
    client_fd = accept(self_socket_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (client_fd == -1)
    {
        cerr << "server: accept error" << endl;
        // continue;
    }
    const char *ip = get_in_addr((struct sockaddr *)&their_addr, s, sizeof(s));
    printf("server: got connection from %s\n", ip);

    // close(client_fd);
    // }
    return client_fd;
}

int Proxy::setupClient(const char *host, const char *port)
{
    int sockfd;
    // char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo; //*p;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
    {
        cerr << "client: getaddrinfo:" << gai_strerror(rv) << endl;
        cerr << "host" << host << std::endl;
        cerr << "port" << port << std::endl;
        exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        cerr << "client: socket" << endl;
        exit(EXIT_FAILURE);
    }
    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        close(sockfd);
        cerr << "client: connect" << endl;
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}

// If multi thread, stll need a parameter "id"
void Proxy::processConnect(int client_fd, int server_fd){
    send(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    fd_set read_fds;
    int numbytes;

    while (true)
    {
        FD_ZERO(&read_fds);
        // Add fd to set
        FD_SET(client_fd, &read_fds);
        FD_SET(server_fd, &read_fds);

        // Monitor fds
        int n = select(max(client_fd, server_fd) + 1, &read_fds, NULL, NULL, NULL);
        if (n < 0)
        {
            cerr << "Select error" << endl;
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(client_fd, &read_fds))
        {
            char buf[MAXDATASIZE];
            if ((numbytes = recv(client_fd, buf, MAXDATASIZE - 1, 0)) <= 0)
            {
                cerr << "Tunnel closed" << endl;
                return;
            }
            else
            {
                buf[numbytes] = '\0';
                send(server_fd, buf, numbytes, 0);
            }
        }

        if (FD_ISSET(server_fd, &read_fds))
        {
            char buf[MAXDATASIZE];
            if ((numbytes = recv(server_fd, buf, MAXDATASIZE - 1, 0)) <= 0)
            {
                cerr << "Tunnel closed" << endl;
                return;
            }
            else
            {
                buf[numbytes] = '\0';
                send(client_fd, buf, numbytes, 0);
            }
        }
    }
}

void Proxy::processGet(int client_fd, int server_fd, string host, string port, string request){
    HTTPRequestParser parsedReq(request);
    //int clientMaxStale = parsedReq.getMaxStale();
    string etag = parsedReq.getETag();
    string lastmodified = parsedReq.getLastModified();

    unordered_map<string, string> cache;
    string cacheKey = host + ":" + port;
    auto it = cache.find(cacheKey);
    //bool flag_valid;
    // Found
    if (it != cache.end()) 
    {   
        string cached_response = it->second; // Access the value
        bool flag_valid = isNotExpired(cached_response);  // Check expire or not
        // Intend to check whether there is "no-cache" in cachedResponse
        HTTPResponseParser parsedCachedRes(cached_response);
        string cached_response_cc = parsedCachedRes.getHeader("Cache-Control");  
        // Not Expired, send cached response
        if(flag_valid && cached_response_cc.find("no-cache") == std::string::npos)
        {
            std::cout << "in cache, valid" << std::endl;
            send(client_fd, cached_response.c_str(), cached_response.size(), 0);
        }
        // Expired, check Etag
        else
        {
            std::cout << "in cache, requires validation" << std::endl;
            // Etag exists
            if(!etag.empty())
            {
                string request_add_INM = addIfNoneMatch(request);
                std::cout << "Requesting from server" << std::endl;
                send(server_fd, request_add_INM.c_str(), request_add_INM.size(), 0);
            }
            // Etag not exists, check Last-Modified
            else
            {
                // Last-Modified exists
                if(!lastmodified.empty()){
                    string request_add_IMS = addIfModifiedSince(request);
                    std::cout << "Requesting from server" << std::endl;
                    send(server_fd, request_add_IMS.c_str(), request_add_IMS.size(), 0); 
                }
                // Both ETag and Last-modified not exists, send request to origin server
                else
                {
                    std::cout << "Requesting from server" << std::endl;
                    send(server_fd, request.c_str(), request.size(), 0); 
                }
            }   
        }
    } 
    // Key not found, send request to origin server
    else 
    {
        std::cout << "not in cache" << std::endl;
        std::cout << "Requesting from server" << std::endl;
        send(server_fd, request.c_str(), request.size(), 0);
    }       

    // Receive the message from origin server
    char rawRes[MAXDATASIZE];
    int resLen = recv(server_fd, rawRes, sizeof(rawRes), 0);
    // skip some error handling here
    rawRes[resLen] = '\0';
    string resStr(rawRes);
    HTTPResponseParser parsedRes(resStr);
    string status = parsedRes.getStatus();
    string cacheControl = parsedRes.getHeader("Cache-Control");
    std::cout << "Received from server" << std::endl;
    // Check 304 or 200
    // 304, use cached response
    if(status == "304"){
        string same_cached_response = it->second; // Access the value
        std::cout << "Responding " << std::endl;
        send(client_fd, same_cached_response.c_str(), same_cached_response.size(), 0);
    }
    else
    {
        // response is chunked
        string completeResponse = resStr; // To assemble chunked response, Initialize complete response with what we've received so far
        if(parsedRes.isChunked()){
            std::cout << "Responding " << std::endl;
            send(client_fd, rawRes, resLen, 0);
            char chunkedRes[MAXDATASIZE];
            while(true){
                int chunkedResLen = recv(server_fd, chunkedRes, sizeof(chunkedRes), 0);
                if(chunkedResLen <= 0){
                    break;
                }
                completeResponse.append(chunkedRes, chunkedResLen);
                chunkedRes[chunkedResLen] = '\0';
                std::cout << "Responding " << std::endl;
                send(client_fd, chunkedRes, chunkedResLen, 0);
            }
            // need code to store it in cache
        }
        // Not chunked
        else{ 
            std::cout << "not chunked." << std::endl;
            std::cout << "Responding " << std::endl;
            send(client_fd, rawRes, resLen, 0);
        }
        std::cout << "response: " << completeResponse << std::endl;
        // After receiving whole reponse, decide whether cache it or not
        if (cacheControl.find("no-store") == std::string::npos && 
            cacheControl.find("private") == std::string::npos ){ 
           // cacheControl.find("no-cache") == std::string::npos) 
            cache[cacheKey] = completeResponse; 
        }
    }
}

void Proxy::processPost(int client_fd, int server_fd, string requestStr){

    // Forward the POST request to the server
    if (send(server_fd, requestStr.c_str(), requestStr.length(), 0) <= 0)
    {
        cerr << "Failed to send POST request to server" << endl;
        close(server_fd);
        return;
    }

    // Receive the response from the server
    char response[MAXDATASIZE];
    int numbytes = recv(server_fd, response, MAXDATASIZE - 1, 0);
    if (numbytes <= 0)
    {
        cerr << "Failed to receive response from server" << endl;
        close(server_fd);
        return;
    }
    response[numbytes] = '\0';

    // Forward the response back to the client
    if (send(client_fd, response, numbytes, 0) <= 0)
    {
        cerr << "Failed to send response back to client" << endl;
    }

    // Close the server connection
    close(server_fd);
}