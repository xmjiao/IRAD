///
/// \file
/// \brief TCP/IP Network Utilities Interface
/// \ingroup irad_group
///

#include <sys/types.h>
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#elif _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <vector>
#include <algorithm>

namespace IRAD {
  namespace Sys {

    /// TCP/IP Network Utilities
    namespace Net {

	//Paul Vixie inet_pton code for windows:
#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

		

int inet_pton4(const char *src, void *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    int ch;
    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}

int inet_pton6(const char *src, void *dst)
{
    static const char xdigits[] = "0123456789abcdef";
    uint8_t tmp[NS_IN6ADDRSZ];

    uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
    uint8_t *endp = tp + NS_IN6ADDRSZ;
    uint8_t *colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
    {
        if (*++src != ':')
            return 0;
    }

    const char *curtok = src;
    int saw_xdigit = 0;
    uint32_t val = 0;
    int ch;
    while ((ch = tolower(*src++)) != '\0')
    {
        const char *pch = strchr(xdigits, ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!saw_xdigit)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return 0;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, (char*) tp) > 0)
        {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return 0;
    }
    if (saw_xdigit)
    {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (uint8_t) (val >> 8) & 0xff;
        *tp++ = (uint8_t) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;

        if (tp == endp)
            return 0;

        for (int i = 1; i <= n; i++)
        {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);

    return 1;
}


int inet_pton(int af, const char *src, void *dst)
{
    switch (af)
    {
    case AF_INET:
        return inet_pton4(src, dst);
    case AF_INET6:
        return inet_pton6(src, dst);
    default:
        return -1;
    }
}


      class Server {
      private:
	int                _server_descriptor;
	struct sockaddr_in _server_addy;
	std::vector<int>   _client_descriptors;
	std::vector<struct sockaddr_in> _client_addys; 
      public:
	Server() : _server_descriptor(-1)
	{
	  std::memset(&_server_addy,0,sizeof(struct sockaddr_in));
	  _client_descriptors.resize(0);
	  _client_addys.resize(0);
	};
	struct sockaddr_in &Address() { return (_server_addy); };
	const struct sockaddr_in Address() const { return (_server_addy); };
	std::vector<int> &ClientDescriptors() { return(_client_descriptors); };
	const std::vector<int> &ClientDescriptors() const { return(_client_descriptors); };
	int &Descriptor() { return(_server_descriptor); };
	int Descriptor() const { return(_server_descriptor); };
	std::vector<struct sockaddr_in> &ClientAddresses() { return (_client_addys); };
	const std::vector<struct sockaddr_in> &ClientAddresses() const { return(_client_addys); };
	int SimpleInit(int port = 0)
	{
	  _server_descriptor = socket(AF_INET,SOCK_STREAM,0);
	  if(_server_descriptor < 0){
	    perror("Sys::Net::Server::SimpleInit::socket");
	    return(-1);
	  }
	  if(port <= 0)
	    port = 8044; // should not collide with much ... if anything
	  _server_addy.sin_family = AF_INET;
	  _server_addy.sin_addr.s_addr = htonl(INADDR_ANY);
	  _server_addy.sin_port = htons(port);
	  int retval = bind(_server_descriptor,reinterpret_cast<const sockaddr *>(&_server_addy),
			    sizeof(struct sockaddr_in));
	  if(retval < 0){
	    perror("Sys::Net::Server::SimpleInit::bind");
	    return(-1);
	  }
	  if(listen(_server_descriptor,1)){
	    perror("Sys::Net::Server::SimpleInit::listen");
	    return(-1);
	  }
	  return(0);
	};
	int Accept()
	{
	  struct sockaddr_in client_addy;
	  socklen_t addysize = sizeof(struct sockaddr_in);
	  int new_descriptor = accept(_server_descriptor,reinterpret_cast<sockaddr *>(&client_addy),
				      &addysize);
	  if(new_descriptor < 0){
	    if((errno != EAGAIN) || (errno != EWOULDBLOCK))
	      perror("Sys::Net::Server::accept");
	  }
	  else{
	    _client_descriptors.push_back(new_descriptor);
	    _client_addys.push_back(client_addy);
	  }
	  return(new_descriptor);
	};
	int CloseAll()
	{
	  std::vector<int>::iterator fdi = _client_descriptors.begin();
	  while(fdi != _client_descriptors.end())
	    close(*fdi++);
	  return(0);
	};
	~Server(){ CloseAll(); };
      };
    
      class Client
      {
      private:
	int _descriptor;
	struct sockaddr_in _server_addy;
      public:
	Client() : _descriptor(-1)
	{
	  std::memset(&_server_addy,0,sizeof(struct sockaddr_in));
	};
	int &Descriptor() { return(_descriptor); };
	int Descriptor() const { return(_descriptor); };
	struct sockaddr_in &ServerAddress() { return(_server_addy); };
	const struct sockaddr_in ServerAddresss() const { return(_server_addy); };
	int Connect(const std::string &server_address,int port = 0)
	{
	  if(port <= 0)
	    port = 8044;
	  _descriptor = socket(AF_INET,SOCK_STREAM,0);
	  if(_descriptor < 0){
	    perror("Sys::Net::Client::Connect::socket");
	    return(-1);
	  }
	  _server_addy.sin_family = AF_INET;
	  _server_addy.sin_port   = htons(port);
	  int retval = inet_pton(AF_INET,server_address.c_str(),&_server_addy.sin_addr); 
	  if(retval < 0){
	    perror("Sys::Net::Client::Connect::inet_pton");
	    return(-1);
	  }
	  else if(retval == 0){
	    std::cerr << "Sys::Net::Client::Connect::inet_pton: Invalid address: " 
		      << server_address << std::endl;
	    return(-1);
	  }
	  if(connect(_descriptor,reinterpret_cast<const sockaddr *>(&_server_addy),
		     sizeof(struct sockaddr_in)) < 0){
	    perror("Sys::Net::Client::Connect::connect");
	    return(-1);
	  }
	  return(0);
	};
	int Close() { if(_descriptor > 0) close(_descriptor); return(0);};
	~Client() { Close(); };
      };
    };
  };
};
