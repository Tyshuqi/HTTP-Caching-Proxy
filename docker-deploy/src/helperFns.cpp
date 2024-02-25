#include "helperFns.h"


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

int main() {
    std::string httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 24 Feb 2024 19:22:00 GMT\r\n"
        "Server: ExampleServer/1.0\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 12345\r\n"
        "Expires: Thu, 24 Feb 2022 13:00:00 GMT\r\n"
        //"Cache-Control: max-age=3600, public\r\n"
        
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

    HTTPResponseParser responseParser(httpResponse);

    std::string result = calculateExpiration(responseParser.getHeader("Date"), responseParser.getHeader("Cache-Control"));
    //std::cout << "exp time: " << result << std::endl;
    bool isNotEx = isNotExpired(httpResponse);
    std::cout << "is not expired: " << isNotEx << std::endl;
    return 0;
}