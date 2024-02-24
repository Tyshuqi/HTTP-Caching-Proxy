#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <boost/beast/http.hpp>

class HTTPRequestParser {
private:
    std::string method;
    std::string requestURI;
    unsigned int protocolVersion;
    boost::beast::http::request<boost::beast::http::string_body> parsedRequest;

    void parse(const std::string& rawRequest);

public:

    HTTPRequestParser(const std::string& rawRequest);

    std::string getMethod() const;

    std::string getRequestURI() const;

    unsigned int getProtocolVersion() const;

    std::string getHeader(const std::string& headerName) const;

    std::string getBody() const;

    int getMaxStale() const;

};