#include "HTTPRequestParser.h"

HTTPRequestParser::HTTPRequestParser(const std::string& rawRequest) : method(""), requestURI(""), protocolVersion(0), parsedRequest(boost::beast::http::request<boost::beast::http::string_body>()) {
    parse(rawRequest);
}

void HTTPRequestParser::parse(const std::string& rawRequest) {
    boost::beast::error_code errorCode;
    boost::beast::http::request_parser<boost::beast::http::string_body> parser;
    parser.put(boost::asio::buffer(rawRequest), errorCode);
    if(errorCode) {
        std::cerr << "Error parsing request: " << errorCode.message() << std::endl;
        return;
    }
    parsedRequest = parser.get();
    method = parsedRequest.method_string().to_string();
    requestURI = parsedRequest.target().to_string();
    protocolVersion = parsedRequest.version();

}

std::string HTTPRequestParser::getMethod() const {
    return method;
}

std::string HTTPRequestParser::getRequestURI() const {
    return requestURI;
}

unsigned int HTTPRequestParser::getProtocolVersion() const {
    return protocolVersion;
}   

std::string HTTPRequestParser::getHeader(const std::string& headerName) const {
    try{
        return parsedRequest.at(headerName).to_string();
    } catch (std::exception& e) {
        return "";
    }
}

std::string HTTPRequestParser::getBody() const {
    return parsedRequest.body();
}

int HTTPRequestParser::getMaxStale() const{
    std::string cacheControlHeader = getHeader("Cache-Control");
    if (cacheControlHeader.empty()) {
        return 0; // Cache-Control header not present, no staleness is acceptable
    }

    size_t startPos = cacheControlHeader.find("max-stale");
    if (startPos == std::string::npos) {
        return 0; // max-stale directive not present, also no staleness is acceptable
    }

    size_t equalsPos = cacheControlHeader.find("=", startPos);
    if (equalsPos == std::string::npos) {
        return -1; // max-stale specified without a value, indicating that any level of staleness is acceptable
    }

    size_t endPos = cacheControlHeader.find(",", equalsPos);
    std::string value = cacheControlHeader.substr(equalsPos + 1, endPos - equalsPos - 1);
    try {
        return std::stoi(value);  // Return max-stale value in seconds
    } catch (const std::exception& e) {
        std::cerr << "Error parsing max-stale value: " << e.what() << std::endl;
        return 0; // Error parsing value, treat as if max-stale is not present
    }
}

std::string HTTPRequestParser::getETag() const {
    return getHeader("ETag");
}

std::string HTTPRequestParser::getLastModified() const {
    return getHeader("Last-Modified");
}


/*int main() {
    std::string rawGetRequest = "GET /path/to/resource HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Cache-Control: max-stale=60\r\n"
                                "\r\n";

    HTTPRequestParser parser(rawGetRequest);

    std::cout << "Method: " << parser.getMethod() << std::endl;
    std::cout << "Request URI: " << parser.getRequestURI() << std::endl;
    std::cout << "Protocol Version: " << parser.getProtocolVersion() << std::endl;
    std::cout << "Host Header: " << parser.getHeader("Host") << std::endl;
    std::cout << "Max-Stale: " << parser.getMaxStale() << std::endl;

    return 0;
}*/
