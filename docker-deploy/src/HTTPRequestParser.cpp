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

    void parse(const std::string& rawRequest) {
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

public:

    HTTPRequestParser(const std::string& rawRequest) : method(""), requestURI(""), protocolVersion(0), parsedRequest(boost::beast::http::request<boost::beast::http::string_body>()) {
        parse(rawRequest);
    }

    std::string getMethod() const {
        return method;
    }

    std::string getRequestURI() const {
        return requestURI;
    }

    unsigned int getProtocolVersion() const {
        return protocolVersion;
    }   

    std::string getHeader(const std::string& headerName) const {
        try{
            return parsedRequest.at(headerName).to_string();
        } catch (std::exception& e) {
            return "";
        }
    }

    std::string getBody() const {
        return parsedRequest.body();
    }


};

int main() {
    std::string rawRequest = "POST / HTTP/1.1\r\n"
        "Host: localhost:8000\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.9; rv:50.0) Gecko/20100101 Firefox/50.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "Content-Type: multipart/form-data; boundary=---------------------------8721656041911415653955004498\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "12345\r\n";

    
    HTTPRequestParser parser = HTTPRequestParser(rawRequest);
    
    assert(parser.getMethod() == "POST");
    assert(parser.getRequestURI() == "/");
    assert(parser.getProtocolVersion() == 11);
    assert(parser.getHeader("Host") == "localhost:8000");
    assert(parser.getHeader("User-Agent") == "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.9; rv:50.0) Gecko/20100101 Firefox/50.0");
    assert(parser.getHeader("Accept-Language") == "en-US,en;q=0.5");
    assert(parser.getHeader("Accept-Encoding") == "gzip, deflate");
    assert(parser.getHeader("Connection") == "keep-alive");
    assert(parser.getHeader("Upgrade-Insecure-Requests") == "1");
    assert(parser.getHeader("Content-Type") == "multipart/form-data; boundary=---------------------------8721656041911415653955004498");
    assert(parser.getHeader("Content-Length") == "5");
    std::cout << "body: " << parser.getBody() << std::endl;
    std::cout << "All assertions passed successfully!" << std::endl;
    return 0;
}
