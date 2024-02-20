#include "OriginServerConnection.hpp"

OriginServerConnection::OriginServerConnection(boost::asio::io_context& ioContext, const std::string& targetHost, const std::string& targetPort, const std::string& rawRequest)
    : resolver_(ioContext), socket_(ioContext), rawRequest_(rawRequest) {
    endpoints_ = resolver_.resolve(targetHost, targetPort);
}

void OriginServerConnection::asyncConnect() {
    boost::asio::async_connect(socket_, endpoints_, std::bind(&OriginServerConnection::handleConnect, this, std::placeholders::_1));
}

void OriginServerConnection::handleConnect(const boost::system::error_code& error) {
    if (!error) {
        std::cout << "Connected to Origin server" << std::endl;

        boost::asio::async_write(socket_, boost::asio::buffer(rawRequest_), std::bind(&OriginServerConnection::handleWriteRequest, this, std::placeholders::_1, std::placeholders::_2));
    } else {
        std::cerr << "Error connecting to Origin server: " << error.message() << std::endl;
    }
}

void OriginServerConnection::handleWriteRequest(const boost::system::error_code& error, std::size_t bytesTransferred) {
    if (!error) {
        std::cout << "Request sent to Origin server" << std::endl;
    } else {
        std::cerr << "Error sending request to Origin server: " << error.message() << std::endl;
    }
}
