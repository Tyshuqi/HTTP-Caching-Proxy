#include <string>
#include <iostream>
#include <sstream>
#include <boost/beast/http.hpp>


class readAndParseRequest {
private:
    std::string requestString;
    boost::beast::http::request<boost::beast::http::string_body> request;
public:
    readAndParseRequest(const std::string & _requestString);
    void parse();
    std::string getMethod();
    std::string getTarget();
    std::string getVersion();
};

        