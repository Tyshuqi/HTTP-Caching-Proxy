#include <iostream>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

class HTTPResponseParser {
private:
    http::response_parser<http::string_body> parsedResponse;

public:
    HTTPResponseParser(const std::string & rawResponse);
    void parse(const std::string& rawResponse);

    std::string getStatus() const;
    bool isChunked() const;
    //std::string getHeader() const;
    std::string getBody() const;
    std::string getHeader(const std::string& name) const;
};