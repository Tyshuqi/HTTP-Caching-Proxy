#include "HTTPResponseParser.h"

namespace beast = boost::beast;
namespace http = beast::http;

HTTPResponseParser::HTTPResponseParser(const std::string & rawResponse){
    parse(rawResponse);
}


void HTTPResponseParser::parse(const std::string& rawResponse) {
    boost::beast::error_code errorCode;
    parsedResponse.put(boost::asio::buffer(rawResponse), errorCode);
    if (errorCode) {
        std::cerr << "Error parsing response: " << errorCode.message() << std::endl;
        return;
    }
}

std::string HTTPResponseParser::getStatus() const {
    return std::to_string(parsedResponse.get().result_int());
}

std::string HTTPResponseParser::getBody() const {
    return parsedResponse.get().body();
}

std::string HTTPResponseParser::getHeader(const std::string& name) const {
        auto& headers = parsedResponse.get().base();
        auto it = headers.find(name);
        if (it != headers.end()) {
            return it->value().to_string();
        } else {
            return "";
        }
    }

bool HTTPResponseParser::isChunked() const{
    return parsedResponse.chunked();
}

/*int main() {
    std::string httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 24 Feb 2022 12:00:00 GMT\r\n"
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

    HTTPResponseParser responseParser(httpResponse);

    std::cout << "Status Code: " << responseParser.getStatus() << std::endl;

    std::cout << "Body: " << responseParser.getBody() << std::endl;

    std::cout << "Date: " << responseParser.getHeader("Date") << std::endl;

    std::cout << "CacheCtrl Header: " << responseParser.getHeader("Cache-Control") << std::endl;

    std::cout << "Expires: " << responseParser.getHeader("Expires") << std::endl;

    std::cout << "Is Chunked: " << (responseParser.isChunked() ? "Yes" : "No") << std::endl;

    return 0;
}*/

