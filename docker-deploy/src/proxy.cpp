#include "proxy.h"
#include "HTTPRequestParser.h"
#include "HTTPResponseParser.h"
#include "helperFns.h"
#include "user.h"

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 65536
#define MAXMAPSIZE 1000  // Cache size

using namespace std;

int Proxy::setupServer(const char *port)
{
    int self_socket_fd;               // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo; // *p;
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

int Proxy::acceptUser(int self_socket_fd, std::string &client_ip)
{
    char s[INET6_ADDRSTRLEN];
    int client_fd;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size = sizeof(their_addr);
    //*********** accept **********//
    client_fd = accept(self_socket_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (client_fd == -1)
    {
        cerr << "server: accept error" << endl;
    }
    const char *ip = get_in_addr((struct sockaddr *)&their_addr, s, sizeof(s));
    //printf("server: got connection from %s\n", ip);
    client_ip = ip;
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
        // exit(EXIT_FAILURE);
        return -1;
    }
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        cerr << "client: socket" << endl;
        // exit(EXIT_FAILURE);
        return -1;
    }
    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        close(sockfd);
        cerr << "client: connect" << endl;
        // exit(EXIT_FAILURE);
        return -1;
    }
    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}

// If multi thread, stll need a parameter "id"
void Proxy::processConnect(int client_fd, int server_fd, User *user)
{
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": Responding \"HTTP/1.1 200 OK\"" << endl;
    pthread_mutex_unlock(&threadLock);
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
            // exit(EXIT_FAILURE);
            return;
        }

        if (FD_ISSET(client_fd, &read_fds))
        {
            char buf[MAXDATASIZE];
            if ((numbytes = recv(client_fd, buf, MAXDATASIZE - 1, 0)) <= 0)
            {
                pthread_mutex_lock(&threadLock);
                logfile << user->getID() << ": Tunnel closed" << endl;
                pthread_mutex_unlock(&threadLock);
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
                pthread_mutex_lock(&threadLock);
                logfile << user->getID() << ": Tunnel closed" << endl;
                pthread_mutex_unlock(&threadLock);
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

void Proxy::processGet(int client_fd, int server_fd, string hostname, string port, string request, User *user)
{
    HTTPRequestParser parsedReq(request);
    string etag = parsedReq.getETag();
    string lastmodified = parsedReq.getLastModified();
    string cacheKey = parsedReq.getRequestURI();
    string reqCC = parsedReq.getHeader("Cache-Control");
    auto it = cache.find(cacheKey);
    // if we can find this request in cache
    if (it != cache.end())
    {
        string cached_response = it->second; // Access the value
        bool hasNoCache = (strHasSubstr(request, "no-cache") == true || strHasSubstr(cached_response, "no-cache") == true);
        bool notExpired = isNotExpired(cached_response, request); // Check expire or not
        // Intend to check whether there is "no-cache" in cachedResponse
        HTTPResponseParser parsedCachedRes(cached_response);
        string cached_response_cc = parsedCachedRes.getHeader("Cache-Control");
        // Not Expired, send cached response
        if (notExpired && !hasNoCache)
        {
            pthread_mutex_lock(&threadLock);
            logfile << user->getID() << ": in cache, valid" << endl;
            logfile << user->getID() << ": Responding \"" << getFirstLine(cached_response) << "\"" << endl;
            pthread_mutex_unlock(&threadLock);
            send(client_fd, cached_response.c_str(), cached_response.size(), 0);
            return;
        }
        // Expired, check Etag
        else
        {
            if (hasNoCache)
            {
                pthread_mutex_lock(&threadLock);
                logfile << user->getID() << ": in cache, requires validation" << endl;
                pthread_mutex_unlock(&threadLock);
            }
            else
            {
                pthread_mutex_lock(&threadLock);
                logfile << user->getID() << ": in cache, but expired at " << getExpiredTime(cached_response) << endl;
                pthread_mutex_unlock(&threadLock);
            }

            // Etag exists
            if (!etag.empty())
            {
                string request_add_INM = addIfNoneMatch(request);
                pthread_mutex_lock(&threadLock);
                logfile << user->getID() << ": Requesting \"" << getFirstLine(request_add_INM) << "\" from " << hostname << endl;
                pthread_mutex_unlock(&threadLock);
                send(server_fd, request_add_INM.c_str(), request_add_INM.size(), 0);
            }
            // Etag not exists, check Last-Modified
            else
            {
                // Last-Modified exists
                if (!lastmodified.empty())
                {
                    string request_add_IMS = addIfModifiedSince(request);
                    pthread_mutex_lock(&threadLock);
                    logfile << user->getID() << ": Requesting \"" << getFirstLine(request_add_IMS) << "\" from " << hostname << endl;
                    pthread_mutex_unlock(&threadLock);
                    send(server_fd, request_add_IMS.c_str(), request_add_IMS.size(), 0);
                }
                // Both ETag and Last-modified not exists, send request to origin server
                else
                {
                    pthread_mutex_lock(&threadLock);
                    logfile << user->getID() << ": Requesting \"" << getFirstLine(request) << "\" from " << hostname << endl;
                    pthread_mutex_unlock(&threadLock);
                    send(server_fd, request.c_str(), request.size(), 0);
                }
            }
        }
    }
    // if we cannot find the request in cache
    else
    {
        pthread_mutex_lock(&threadLock);
        logfile << user->getID() << ": not in cache" << endl;
        logfile << user->getID() << ": Requesting \"" << getFirstLine(request) << "\" from " << hostname << endl;
        pthread_mutex_unlock(&threadLock);
        send(server_fd, request.c_str(), request.size(), 0);
    }
    // Receive response from origin server
    char rawRes[MAXDATASIZE];
    int resLen = recv(server_fd, rawRes, sizeof(rawRes), 0);
    rawRes[resLen] = '\0';
    string resStr(rawRes);
    HTTPResponseParser parsedRes(resStr);
    string status = parsedRes.getStatus();
    string resCC = parsedRes.getHeader("Cache-Control");
    string expires = parsedRes.getHeader("Expires");
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": Received \"" << getFirstLine(resStr) << "\" from " << hostname << endl;
    pthread_mutex_unlock(&threadLock);
    // Check 304 or 200
    // 304, use cached response
    if (status == "304")
    {
        string same_cached_response = it->second; // Access the value
        pthread_mutex_lock(&threadLock);
        logfile << user->getID() << ": Responding \"" << getFirstLine(same_cached_response) << "\"" << endl;
        pthread_mutex_unlock(&threadLock);
        send(client_fd, same_cached_response.c_str(), same_cached_response.size(), 0);
        return;
    }
    else
    {                                     // response is chunked
        string completeResponse = resStr; // To assemble chunked response, Initialize complete response with what we've received so far
        if(parsedRes.isChunked()){
            pthread_mutex_lock(&threadLock);
            logfile << user->getID() << "Responding " << getFirstLine(resStr) << "\"" << endl;
            pthread_mutex_unlock(&threadLock);
            send(client_fd, rawRes, resLen, 0);
            char chunkedRes[MAXDATASIZE];
            while (true)
            {
                int chunkedResLen = recv(server_fd, chunkedRes, sizeof(chunkedRes), 0);
                if (chunkedResLen <= 0)
                {
                    break;
                }
                completeResponse.append(chunkedRes, chunkedResLen);
                chunkedRes[chunkedResLen] = '\0';
                send(client_fd, chunkedRes, chunkedResLen, 0);
            }
        }
        // Not chunked
        else{ 
            pthread_mutex_lock(&threadLock);
            logfile << user->getID() << ": Responding \"" << getFirstLine(resStr) << "\"" << endl;
            pthread_mutex_unlock(&threadLock);
            send(client_fd, rawRes, resLen, 0);
        }
        // After receiving whole reponse, decide whether cache it or not
        // Clean expired response
        if (cache.size() > MAXMAPSIZE)
        {
            for (unordered_map<string, string>::iterator it = cache.begin(); it != cache.end(); ++it)
            {
                if (!isNotExpiredWithoutReq(it->second))
                {
                    cache.erase(it);
                }
            }
        }

        if (resCC.find("no-store") == std::string::npos &&
            resCC.find("private") == std::string::npos &&
            (resCC != "" || expires != "") &&
            status == "200" &&
            reqCC.find("no-store") == std::string::npos){ 
                pthread_mutex_lock(&threadLock);   
                cache[cacheKey] = completeResponse; 
                pthread_mutex_unlock(&threadLock); 
        }
    }
}

void Proxy::processPost(int client_fd, int server_fd, string requestStr, User *user, string hostname)
{

    // Forward the POST request to the server
    if (send(server_fd, requestStr.c_str(), requestStr.length(), 0) <= 0)
    {
        cerr << "Failed to send POST request to server" << endl;
        close(server_fd);
        return;
    }
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": Requesting \"" << getFirstLine(requestStr) << "\" from " << hostname << endl;
    pthread_mutex_unlock(&threadLock);
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
    string resp = response;
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": Received \"" << getFirstLine(resp) << "\" from " << hostname << endl;
    pthread_mutex_unlock(&threadLock);
    // Forward the response back to the client
    if (send(client_fd, response, numbytes, 0) <= 0)
    {
        cerr << "Failed to send response back to client" << endl;
    }
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": Responding \"" << getFirstLine(resp) << "\"" << endl;
    pthread_mutex_unlock(&threadLock);
    // Close the server connection
    close(server_fd);
}