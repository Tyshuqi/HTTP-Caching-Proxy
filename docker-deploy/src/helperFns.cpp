#include "helperFns.h"
#include "HTTPResponseParser.h"
#include "HTTPRequestParser.h"
#include "proxy.h"
#include "user.h"

std::unordered_map<std::string, std::string> cache;
std::ofstream logfile("/var/log/erss/proxy.log");
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;

std::string getFirstLine(std::string &msg)
{
    size_t firstLineEnd = msg.find("\r\n");
    std::string firstLine = msg.substr(0, firstLineEnd);
    return firstLine;
}

int parseMaxAge(const std::string &cacheControlStr)
{
    int maxAge = -1;
    size_t pos = cacheControlStr.find("max-age=");
    if (pos != std::string::npos)
    {
        try
        {
            maxAge = std::stoi(cacheControlStr.substr(pos + 8));
        }
        catch (const std::invalid_argument &)
        {
            maxAge = -1;
        }
    }
    return maxAge;
}

int parseMaxStale(const std::string &CCStr)
{
    int maxStale = 0;
    size_t pos = CCStr.find("max-stale=");
    if (pos != std::string::npos)
    {
        try
        {
            maxStale = std::stoi(CCStr.substr(pos + 10));
        }
        catch (const std::invalid_argument &)
        {
            maxStale = 0;
        }
    }
    return maxStale;
}

int parseMinFresh(const std::string &CCStr)
{
    int minFresh = 0;
    size_t pos = CCStr.find("min-fresh=");
    if (pos != std::string::npos)
    {
        try
        {
            minFresh = std::stoi(CCStr.substr(pos + 10));
        }
        catch (const std::invalid_argument &)
        {
            minFresh = 0;
        }
    }
    return minFresh;
}

std::string calculateExpiration(const std::string &dateStr, const std::string &cacheControlStr, const std::string &CCStr)
{
    std::tm date = parseDate(dateStr);
    int maxAge = parseMaxAge(cacheControlStr);
    int sMaxAge = parseSMaxAge(cacheControlStr);
    int maxStale = parseMaxStale(CCStr);
    int minFresh = parseMinFresh(CCStr);

    if (maxAge >= 0 || sMaxAge >= 0)
    {
        int effectiveMaxAge = (sMaxAge >= 0) ? sMaxAge : maxAge;
        // std::cout << "effect " << effectiveMaxAge << std::endl;
        std::time_t expirationTime = std::mktime(&date) + effectiveMaxAge + maxStale - minFresh;
        return formatDate(expirationTime);
    }
    else
    {
        return "Cache-Control doesn't have max-age or s-maxage";
    }
}

std::string calculateExpirationWithoutReq(const std::string &dateStr, const std::string &cacheControlStr)
{
    std::tm date = parseDate(dateStr);
    int maxAge = parseMaxAge(cacheControlStr);
    int sMaxAge = parseSMaxAge(cacheControlStr);

    if (maxAge >= 0 || sMaxAge >= 0)
    {
        int effectiveMaxAge = (sMaxAge >= 0) ? sMaxAge : maxAge;
        // std::cout << "effect " << effectiveMaxAge << std::endl;
        std::time_t expirationTime = std::mktime(&date) + effectiveMaxAge;
        return formatDate(expirationTime);
    }
    else
    {
        return "Cache-Control doesn't have max-age or s-maxage";
    }
}

std::string getExpiredTime(const std::string& resStr){
    HTTPResponseParser responseParser(resStr);
    std::string CC = responseParser.getHeader("Cache-Control");
    std::string Expires = responseParser.getHeader("Expires");
    std::string Date = responseParser.getHeader("Date");
    if(CC != ""){
        return calculateExpiration(Date, CC, "");
    }
    else if(Expires != ""){
        return Expires;
    }
    else{
        return "Cannot get expired time";
    }
}

std::tm parseDate(const std::string& dateStr) {
        std::tm date = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&date, "%a, %d %b %Y %H:%M:%S GMT");
        //std::cout << "Parsed date: " << std::put_time(&date, "%a, %d %b %Y %H:%M:%S GMT") << std::endl;
        return date;
    }

int parseSMaxAge(const std::string &cacheControlStr)
{
    int sMaxAge = -1;
    size_t pos = cacheControlStr.find("s-maxage=");
    if (pos != std::string::npos)
    {
        try
        {

            sMaxAge = std::stoi(cacheControlStr.substr(pos + 9));
            std::cout << sMaxAge << std::endl;
        }
        catch (const std::invalid_argument &)
        {
            sMaxAge = -1;
        }
    }
    return sMaxAge;
}

std::string formatDate(std::time_t time)
{
    std::tm *timeinfo = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%a, %d %b %Y %H:%M:%S GMT");
    return oss.str();
}

bool compareCurrAndExpires(const std::string &expirationTime, const std::time_t& currentTime)
{
    //std::time_t currentTime = std::time(nullptr);

    //std::tm* utcTimeInfo = std::gmtime(&currentTime);
    //std::time_t currentUTCTime = std::mktime(utcTimeInfo);
    //std::cout << "Current Time (UTC): " << std::asctime(utcTimeInfo);

    std::tm expirationDate = {};
    std::istringstream ss(expirationTime);
    ss >> std::get_time(&expirationDate, "%a, %d %b %Y %H:%M:%S GMT");
    std::time_t expirationTimestamp = std::mktime(&expirationDate);

    //std::cout << "Current Time: " << std::ctime(&currentTime) << std::endl;
    //std::cout << "Expiration Time: " << expirationTime << std::endl;
    return currentTime <= expirationTimestamp;
}

bool isNotExpired(const std::string &rawResponse, const std::string &rawRequest, const std::time_t& currentTime)
{
    HTTPResponseParser responseParser(rawResponse);
    HTTPRequestParser requestParser(rawRequest);
    std::string Expires = responseParser.getHeader("Expires");
    if (responseParser.getHeader("Cache-Control") != "")
    {
        return compareCurrAndExpires(calculateExpiration(responseParser.getHeader("Date"), responseParser.getHeader("Cache-Control"), requestParser.getHeader("Cache-Control")), currentTime);
    }
    else if (responseParser.getHeader("Expires") != "")
    {
        return compareCurrAndExpires(Expires, currentTime);
    }
    else
    {
        return false;
    }
}

bool isNotExpiredWithoutReq(const std::string &rawResponse)
{
    HTTPResponseParser responseParser(rawResponse);
    std::string Expires = responseParser.getHeader("Expires");
    if (responseParser.getHeader("Cache-Control") != "")
    {
        return compareCurrAndExpires(calculateExpirationWithoutReq(responseParser.getHeader("Date"), responseParser.getHeader("Cache-Control")), std::time(nullptr));
    }
    else if (responseParser.getHeader("Expires") != "")
    {
        return compareCurrAndExpires(Expires, std::time(nullptr));
    }
    else
    {
        return false;
    }
}

std::string addIfNoneMatch(const std::string &request)
{
    HTTPRequestParser parser(request);
    std::string ifNoneMatchLine = "If-None-Match:" + parser.getHeader("ETag") + "\r\n";
    std::string ans = request;
    ans.insert(ans.size() - 2, ifNoneMatchLine);
    return ans;
}

std::string addIfModifiedSince(const std::string &request)
{
    HTTPRequestParser parser(request);
    std::string ifModSinceLine = "If-Modified-Since:" + parser.getHeader("Last-Modified") + "\r\n";
    std::string ans = request;
    ans.insert(ans.size() - 2, ifModSinceLine);
    return ans;
}

bool strHasSubstr(const std::string &str, const std::string &subStr)
{
    if (str.find(subStr) != std::string::npos)
    {
        return true;
    }
    return false;
}

void *processRequest(void *user_)
{
    Proxy proxy(12345);
    User *user = (User *)user_;
    char rawRequest[65536];
    int numbytes;
    if ((numbytes = recv(user->getSocketFd(), rawRequest, 65536 - 1, 0)) == -1)
    {
        std::cerr << " No Request" << std::endl;
        exit(EXIT_FAILURE);
    }
    rawRequest[numbytes] = '\0';
    std::string requestStr(rawRequest);
    user->setREQUEST(getFirstLine(requestStr));
    pthread_mutex_lock(&threadLock);
    logfile << user->getID() << ": \"" << user->getREQUEST() << "\" "
            << "from " << user->getIP() << " @ " << user->getTIMEStr();
    pthread_mutex_unlock(&threadLock);
    HTTPRequestParser *parse = new HTTPRequestParser(requestStr);
    std::string hostport = parse->getHeader("Host");
    size_t colonPos = hostport.find(':');
    std::string parseHost;
    std::string parsePort;
    if (colonPos != std::string::npos)
    {
        parseHost = hostport.substr(0, colonPos);  // Get the substring before the colon
        parsePort = hostport.substr(colonPos + 1); // Get the substring after the colon
    }
    else
    {
        parseHost = hostport;
        parsePort = "80";
    }

    std::string method = parse->getMethod();
    int server_fd = proxy.setupClient(parseHost.c_str(), parsePort.c_str());

    if (method == "CONNECT")
    {
        proxy.processConnect(user->getSocketFd(), server_fd, user);
    }

    // POST
    else if (method == "POST")
    {
        proxy.processPost(user->getSocketFd(), server_fd, requestStr, user, parseHost);
    }

    // GET
    else if (method == "GET")
    {
        proxy.processGet(user->getSocketFd(), server_fd, parseHost, parsePort, requestStr, user);
    }

    close(user->getSocketFd());
    delete user;
    pthread_exit(nullptr);
}

void runProxy()
{

    Proxy proxy(12345);
    int self_socket_fd = proxy.setupServer("12345");
    int id = 0;
    int client_fd;
    while (true)
    {
        std::string client_ip;
        client_fd = proxy.acceptUser(self_socket_fd, client_ip);
        User *user = new User(id, client_fd, client_ip);
        pthread_t newThread;
        id++;
        pthread_create(&newThread, nullptr, processRequest, user);
        pthread_detach(newThread);
    }
}
