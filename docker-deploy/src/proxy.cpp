#include "proxy.h"
#include "HTTPRequestParser.h"
#include "HTTPResponseParser.h"

// #define PORT "12345"  // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 65536

using namespace std;

int Proxy::setupServer(const char *port)
{
    int sockfd, client_fd;              // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo;   // *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
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

    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        cerr << "server: socket" << endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        cerr << "server: setsockopt" << endl;
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        cerr << "server: bind" << endl;
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1)
    {
        cerr << "server: listen" << endl;
        exit(EXIT_FAILURE);
    }

    // while (1)
    // { // main accept() loop
    sin_size = sizeof their_addr;
    //*********** accept **********//
    client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
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
    int sockfd, numbytes;
    // char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo; //*p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
    {
        cerr << "client: getaddrinfo:" << gai_strerror(rv) << endl;
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
void Proxy::processConnect(int client_fd, int server_fd)
{
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
            if ((numbytes = recv(client_fd, buf, MAXDATASIZE - 1, 0)) == -1)
            {
                cerr << "CONNNECT METHOD: recv original client failed" << endl;
                exit(EXIT_FAILURE);
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
            if ((numbytes = recv(server_fd, buf, MAXDATASIZE - 1, 0)) == -1)
            {
                cerr << "CONNNECT METHOD: recv original server failed" << endl;
                exit(EXIT_FAILURE);
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
    int clientMaxStale = parsedReq.getMaxStale();
    unordered_map<string, string> cache;
    string cacheKey = host + ":" + port;
    // if we can find the response in cache
    if(cache.isValid(cacheKey, clientMaxStale)){
        string cachedResponse = cache.get(cacheKey);
        send(client_fd, cachedResponse.c_str(), cachedResponse.size(), 0);
    }
    else{ // if there is no response in cache
        // send request to origin server
        send(server_fd, request.c_str(), request.size(), 0); 
        // then, get ready to receive the message from origin server
        char rawRes[MAXDATASIZE];
        int resLen = recv(server_fd, rawRes, sizeof(rawRes), 0);
        // skip some error handling here
        rawRes[resLen] = '\0';
        string resStr(rawRes);
        HTTPResponseParser parsedRes(resStr);
        // if the response is chunked
        if(parsedRes.isChunked()){
            send(client_fd, rawRes, resLen, 0);
            char chunkedRes[MAXDATASIZE];
            while(true){
                int chunkedResLen = recv(server_fd, chunkedRes, sizeof(chunkedRes), 0);
                if(chunkedResLen <= 0){
                    break;
                }
                chunkedRes[chunkedResLen] = '\0';
                send(client_fd, chunkedRes, chunkedResLen, 0);
            }
            // need code to store it in cache
        }
        else{ // if the response is not chunked
            send(client_fd, rawRes, resLen, 0);
            // need code to store it in cache
        }
    }

}

void Proxy::processPost(int client_fd, int server_fd, string requestStr)
{

    // Forward the POST request to the server
    if (send(server_fd, requestStr.c_str(), requestStr.length(), 0) == -1)
    {
        cerr << "Failed to send POST request to server" << endl;
        close(server_fd);
        return;
    }

    // Receive the response from the server
    char response[MAXDATASIZE];
    int numbytes = recv(server_fd, response, MAXDATASIZE - 1, 0);
    if (numbytes == -1)
    {
        cerr << "Failed to receive response from server" << endl;
        close(server_fd);
        return;
    }
    response[numbytes] = '\0';

    // Forward the response back to the client
    if (send(client_fd, response, numbytes, 0) == -1)
    {
        cerr << "Failed to send response back to client" << endl;
    }

    // Close the server connection
    close(server_fd);
}

void Proxy::processRequest(const char *port){
    int client_fd = setupServer(port);
    unordered_map<string, string> cache;
    char rawRequest[MAXDATASIZE];
    int numbytes;
    if ((numbytes = recv(client_fd, rawRequest, MAXDATASIZE - 1, 0)) == -1)
    {
        cerr << " No Request" << endl;
        exit(EXIT_FAILURE);
    }
    rawRequest[numbytes] = '\0';
    string requestStr(rawRequest);

    HTTPRequestParser *parse = new HTTPRequestParser(requestStr);
    string hostport = parse->getHeader("Host");
    size_t colonPos = hostport.find(':');
    string parseHost = hostport.substr(0, colonPos);  // Get the substring before the colon
    string parsePort = hostport.substr(colonPos + 1); // Get the substring after the colon

    string method = parse->getMethod();

    int server_fd = setupClient(parseHost.c_str(), parsePort.c_str());

    if (method == "CONNECT"){
        processConnect(client_fd, server_fd);
    }

    // POST
    else if (method == "POST"){
        processPost(client_fd, server_fd, requestStr);
    }
    
    // GET
     else if (method == "GET"){
        processGet(client_fd, server_fd, parseHost, parsePort, requestStr);
    }

    close(client_fd);
}

void Proxy::runProxy(){
    unordered_map<string, string> cache;
}