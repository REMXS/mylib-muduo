#pragma once

#include<arpa/inet.h>
#include<netinet/in.h>
#include<string>


class InetAddress
{
private:
    sockaddr_in addr_;

public:
    explicit InetAddress(std::string ip="127.0.0.1",uint16_t port=0);
    explicit InetAddress(const sockaddr_in&addr);
    ~InetAddress();

    void setSockAddr(const sockaddr_in&addr);

    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort()const;

    sockaddr_in* getSockAddr();
};

