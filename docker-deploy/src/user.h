#ifndef USER_H
#define USER_H

#include <string>
#include <ctime>

class User{
private:
    int ID;
    std::time_t TIME;
    std::string IPFROM;
    std::string REQUEST;
    int socket_fd;

public:
    User(int id, int socket_fd_, std::string ip): ID(id), IPFROM(ip), socket_fd(socket_fd_){
        TIME = std::time(nullptr);

    }
    const int getID() const{
        return ID;
    }
    const std::string getTIMEStr() const{
        std::tm * timeUTC = std::gmtime(&TIME);
        return std::asctime(timeUTC);
    }
    const std::string getIP() const{
        return IPFROM;
    }
    const std::string getREQUEST() const{
        return REQUEST;
    }
    const int getSocketFd() const{
        return socket_fd;
    }
    void setID(int ID_){
        ID = ID_;
    }
    void setTIME(std::time_t TIME_){
        TIME = TIME_;
    }
    void setIP(std::string IPFROM_){
        IPFROM = IPFROM_;
    }
    void setREQUEST(std::string REQUEST_){
        REQUEST = REQUEST_;
    }
    const std::time_t getTIME() const{
        std::tm * timeUTC = std::gmtime(&TIME);
        return std::mktime(timeUTC);
    }
};

#endif