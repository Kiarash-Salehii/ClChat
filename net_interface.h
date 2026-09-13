#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <wchar.h>

#if defined _WIN32 && defined _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <iphlpapi.h>
#else
#define _GNU_SOURCE
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

typedef struct net_interface
{
    char name[64];

    unsigned int index;
    
    unsigned char mac[6];

    char ipv4[INET_ADDRSTRLEN];
    unsigned char prefix_length;

    char ipv6[INET6_ADDRSTRLEN];

    char netmask[INET_ADDRSTRLEN];
    char broadcast[INET_ADDRSTRLEN];

    unsigned int mtu;

#ifdef _WIN32
    const wchar_t* friendly_name;
    const wchar_t* description;
#endif

    bool loopback;
    bool running;
    bool up;
} net_interface_t;

int net_get_interfaces(net_interface_t** net_interfaces, size_t* net_interfaces_count);
void destroy_interfaces(net_interface_t* iface, size_t interface_count);
int net_get_lan_ipv4(uint32_t* address, uint32_t* interface_index, uint8_t* prefix_length);
void net_copy_interface(net_interface_t* dst_interface, net_interface_t* src_interface);
int net_get_interface_by_index(net_interface_t* _interface, uint32_t index);

#endif // _NET_INTERFACE_H_