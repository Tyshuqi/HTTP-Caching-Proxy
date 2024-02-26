#ifndef USER_H
#define USER_H

#include <string>

class User{
private:
    int id;
    int socket_fd;
    //std::string ip;

public:
    User(int id_, int socket_fd_): id(id_), socket_fd(socket_fd_){}
    const int getId() const{
        return id;
    }
    const int getSocketFd() const{
        return socket_fd;
    }
    //const std::string getIp() const{
      //  return ip;
    //}
};

#endif