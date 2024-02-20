#include <iostream>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

class HTTPResponseParser {
private:
    http::response_parser<http::string_body> parsedResponse;

public:
    void parse(const std::string& rawResponse) {
        boost::beast::error_code errorCode;
        parsedResponse.put(boost::asio::buffer(rawResponse), errorCode);
        if (errorCode) {
            std::cerr << "Error parsing response: " << errorCode.message() << std::endl;
            return;
        }
    }

    std::string getStatus() const {
        return std::to_string(parsedResponse.get().result_int());
    }

    std::string getBody() const {
        return parsedResponse.get().body();
    }
};

int main() {
    std::string rawResponse = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 12\r\n"
                              "\r\n"
                              "Hello, World";

    HTTPResponseParser parser;
    parser.parse(rawResponse);

    std::cout << "Status Code: " << parser.getStatus() << std::endl;
    std::cout << "Response Body: " << parser.getBody() << std::endl;

    return 0;
}
