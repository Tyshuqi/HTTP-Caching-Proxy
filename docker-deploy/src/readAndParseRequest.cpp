#include "readAndParseRequest.hpp"

readAndParseRequest::readAndParseRequest(const std::string & _requestString) : requestString(_requestString) {}

void readAndParseRequest::parse() {
    std::stringstream ss(requestString);
    boost::beast::http::read(ss, request);
}

std::string readAndParseRequest::getMethod() {
    return request.method_string().to_string();
}

std::string readAndParseRequest::getTarget() {
    return request.target().to_string();
}

std::string readAndParseRequest::getVersion() {
    return request.version().to_string();
}