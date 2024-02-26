#include "helperFns.h"
#include "HTTPResponseParser.h"
#include "HTTPRequestParser.h"
#include "proxy.h"
#include "user.h"

std::string calculateExpiration(const std::string& dateStr, const std::string& cacheControlStr) {
        std::tm date = parseDate(dateStr);
        int maxAge = parseMaxAge(cacheControlStr);
        int sMaxAge = parseSMaxAge(cacheControlStr);

        if (maxAge >= 0 || sMaxAge >= 0) {
            int effectiveMaxAge = (sMaxAge >= 0) ? sMaxAge : maxAge;
            //std::cout << "effect " << effectiveMaxAge << std::endl;
            std::time_t expirationTime = std::mktime(&date) + effectiveMaxAge;
            return formatDate(expirationTime);
        } else {
            return "Cache-Control doesn't have max-age or s-maxage";
        }
    }

std::tm parseDate(const std::string& dateStr) {
        std::tm date = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&date, "%a, %d %b %Y %H:%M:%S GMT");
        //std::cout << "Parsed date: " << std::put_time(&date, "%a, %d %b %Y %H:%M:%S GMT") << std::endl;
        return date;
    }

int parseMaxAge(const std::string& cacheControlStr) {
        int maxAge = -1;
        size_t pos = cacheControlStr.find("max-age=");
        if (pos != std::string::npos) {
            try {
                maxAge = std::stoi(cacheControlStr.substr(pos + 8));
            } catch (const std::invalid_argument&) {
                maxAge = -1;
            }
        }
        return maxAge;
    }

int parseSMaxAge(const std::string& cacheControlStr) {
        int sMaxAge = -1;
        size_t pos = cacheControlStr.find("s-maxage=");
        if (pos != std::string::npos) {
            try {
                
                sMaxAge = std::stoi(cacheControlStr.substr(pos + 9));
                std::cout << sMaxAge << std::endl;
            } catch (const std::invalid_argument&) {
                sMaxAge = -1;
            }
        }
        return sMaxAge;
    }

std::string formatDate(std::time_t time) {
        std::tm* timeinfo = std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(timeinfo, "%a, %d %b %Y %H:%M:%S GMT");
        return oss.str();
    }

bool compareCurrAndExpires(const std::string& expirationTime) {
    std::time_t currentTime = std::time(nullptr);

    std::tm expirationDate = {};
    std::istringstream ss(expirationTime);
    ss >> std::get_time(&expirationDate, "%a, %d %b %Y %H:%M:%S GMT");
    std::time_t expirationTimestamp = std::mktime(&expirationDate);

    return currentTime <= expirationTimestamp;
}

bool isNotExpired(const std::string& rawResponse){
    HTTPResponseParser responseParser(rawResponse);
    std::string Expires = responseParser.getHeader("Expires");
    if(responseParser.getHeader("Cache-Control") != ""){
        return compareCurrAndExpires(calculateExpiration(responseParser.getHeader("Date"), responseParser.getHeader("Cache-Control")));
    }
    else if(responseParser.getHeader("Expires") != ""){
        return compareCurrAndExpires(Expires);
    }
    else{
        return false;
    }
}

std::string addIfNoneMatch(const std::string& request){
    HTTPRequestParser parser(request);
    std::string ifNoneMatchLine = "If-None-Match:" + parser.getHeader("ETag") + "\r\n";
    std::string ans = request;
    ans.insert(ans.size() - 2, ifNoneMatchLine);
    return ans;
}

std::string addIfModifiedSince(const std::string& request){
    HTTPRequestParser parser(request);
    std::string ifModSinceLine = "If-Modified-Since:" + parser.getHeader("Last-Modified") + "\r\n";
    std::string ans = request;
    ans.insert(ans.size() - 2, ifModSinceLine);
    return ans;
}

void * processRequest(void * user_){
    Proxy proxy(12345);
    User * user = (User *) user_;
    char rawRequest[65536];
    int numbytes;
    if ((numbytes = recv(user->getSocketFd() , rawRequest, 65536 - 1, 0)) == -1)
    {
        std::cerr << " No Request" << std::endl;
        exit(EXIT_FAILURE);
    }
    rawRequest[numbytes] = '\0';
    std::string requestStr(rawRequest);
    //std::cout << "whole request: " << requestStr << std::endl;
    HTTPRequestParser *parse = new HTTPRequestParser(requestStr);
    std::string hostport = parse->getHeader("Host");
    //std::cout << "Host: " << hostport << std::endl; 
    size_t colonPos = hostport.find(':');
    std::string parseHost;
    std::string parsePort;
    if(colonPos != std::string::npos){
        parseHost = hostport.substr(0, colonPos);  // Get the substring before the colon
        parsePort = hostport.substr(colonPos + 1); // Get the substring after the colon
    }
    else{
        parseHost = hostport;
        parsePort = "80";
    }

    std::string method = parse->getMethod();
    
    int server_fd = proxy.setupClient(parseHost.c_str(), parsePort.c_str());

    if (method == "CONNECT"){
        std::cout << "go into CONNECT" << std::endl;
        proxy.processConnect(user->getSocketFd(), server_fd);
        std::cout << "CONNECT end" << std::endl;
    }

    // POST
    else if (method == "POST"){
        std::cout << "go into POST" << std::endl;
        proxy.processPost(user->getSocketFd(), server_fd, requestStr);
        std::cout << "POST end" << std::endl;
    }
    
    // GET
    else if (method == "GET"){
        std::cout << "go into GET" << std::endl;
        proxy.processGet(user->getSocketFd(), server_fd, parseHost, parsePort, requestStr);
        std::cout << "GET end" << std::endl;
    }

    close(user->getSocketFd());
    return nullptr;
}

void runProxy(){
    std::unordered_map<std::string, std::string> cache;
    Proxy proxy(12345);
    int self_socket_fd = proxy.setupServer("12345");
    int id = 0;
    int client_fd;
    while(true){
        
        client_fd = proxy.acceptUser(self_socket_fd);
        User * user = new User(id, client_fd);
        pthread_t newThread;
        id++;
        std::cout << "id: " << id << std::endl;
        pthread_create(&newThread, nullptr, processRequest, user);
    }
}


/*int main() {
    std::string httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 24 Feb 2024 19:22:00 GMT\r\n"
        "Server: ExampleServer/1.0\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 12345\r\n"
        "Expires: Thu, 24 Feb 2022 13:00:00 GMT\r\n"
        "Cache-Control: max-age=3600, public\r\n"
        
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Example Page</title>\n"
        "</head>\n"
        "<body>\n"
        "    <!-- Page content goes here -->\n"
        "</body>\n"
        "</html>";
    
    std::string rawRequest = "GET /example/resource HTTP/1.1\r\n"
                             "Host: example.com\r\n"
                             "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:100.0) Gecko/20100101 Firefox/100.0\r\n"
                             "ETag: \"abcdef1234567890\"\r\n"
                             "Last-Modified: Sat, 01 Jan 2022 12:00:00 GMT\r\n"
                             "Connection: keep-alive\r\n\r\n";

    HTTPResponseParser responseParser(httpResponse);

    std::string result = calculateExpiration(responseParser.getHeader("Date"), responseParser.getHeader("Cache-Control"));
    //std::cout << "exp time: " << result << std::endl;
    bool isNotEx = isNotExpired(httpResponse);
    //std::cout << "is not expired: " << isNotEx << std::endl;
    std::string match = addIfNoneMatch(rawRequest);
    std::string modify = addIfModifiedSince(rawRequest);
    std::cout << "match:" << match << std::endl;
    std::cout << "modify:" << modify << std::endl;
    return 0;
}*/