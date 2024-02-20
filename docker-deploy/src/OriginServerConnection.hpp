#ifndef ORIGIN_SERVER_CONNECTION_HPP
#define ORIGIN_SERVER_CONNECTION_HPP

#include <boost/asio.hpp>
#include <iostream>

class OriginServerConnection {
public:
    OriginServerConnection(boost::asio::io_context& ioContext, const std::string& targetHost, const std::string& targetPort, const std::string& rawRequest);
    void asyncConnect();

private:
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver::results_type endpoints_;
    std::string rawRequest_;

    void handleConnect(const boost::system::error_code& error);
    void handleWriteRequest(const boost::system::error_code& error, std::size_t bytesTransferred);
};

#endif
