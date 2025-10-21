#include<cstring>

#include"InetAddress.h"

InetAddress::InetAddress(std::string ip,uint16_t port)
{
    memset(&addr_,0,sizeof(addr_));
    addr_.sin_family=AF_INET;
    addr_.sin_addr.s_addr=::inet_addr(ip.c_str());
    addr_.sin_port=htons(port);
}

InetAddress::InetAddress(const sockaddr_in&addr)
    :addr_(addr)
{
}

InetAddress::~InetAddress()
{
}

void InetAddress::setSockAddr(const sockaddr_in&addr)
{
    addr_=addr;    
}

std::string InetAddress::toIp()const
{
    char buf[64]={0};
    inet_ntop(AF_INET,&addr_.sin_addr.s_addr,buf,sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort()const
{
    char buf[64]={0};
    inet_ntop(AF_INET,&addr_.sin_addr.s_addr,buf,sizeof(buf));
    size_t end=strlen(buf);
    snprintf(buf+end,sizeof(buf)-end,":%u",ntohs(addr_.sin_port));
    return buf;
}

uint16_t InetAddress::toPort()const
{
    return ntohs(addr_.sin_port);
}


sockaddr_in* InetAddress::getSockAddr()
{
    return &addr_;
}