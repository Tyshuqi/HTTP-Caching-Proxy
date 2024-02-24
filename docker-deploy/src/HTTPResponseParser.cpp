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

bool HTTPResponseParser::isChunked() const{
    return parsedResponse.chunked();
}
