#include "net_interface.h"

static uint32_t ipv4_broadcast(
    struct sockaddr_in* address,
    uint8_t prefix_length)
{
    uint32_t ip = ntohl(*(uint32_t*)(&address->sin_addr));

    uint32_t mask;

    if (prefix_length == 0)
        mask = 0;
    else
        mask = 0xFFFFFFFFu << (32 - prefix_length);

    return htonl(ip | ~mask);
}

static int netmask_to_prefix(const struct in_addr* netmask)
{
    uint32_t mask = ntohl(netmask->s_addr);

    int prefix = 0;

    while (mask & 0x80000000u) {
        prefix++;
        mask <<= 1;
    }

    return prefix;
}

// retruns the number of net interfaces found.
// retruns -1 when failed
#ifdef _WIN32
int net_get_interfaces(net_interface_t** net_interfaces, size_t* net_interfaces_count)
{
    if (net_interfaces_count == 0) return 0;
    if (net_interfaces == 0) return 0;

    ULONG size = 0;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST |
        GAA_FLAG_SKIP_MULTICAST |
        GAA_FLAG_SKIP_DNS_SERVER |
        GAA_FLAG_INCLUDE_PREFIX;

    DWORD result = GetAdaptersAddresses(
        AF_UNSPEC,
        flags,
        NULL,
        NULL,
        &size
    );

    if (result != ERROR_BUFFER_OVERFLOW) {
        const char* err;
        switch (result)
        {
        case ERROR_ADDRESS_NOT_ASSOCIATED:
            err = "ERROR_ADDRESS_NOT_ASSOCIATED";
            break;
        case ERROR_BUFFER_OVERFLOW:
            err = "ERROR_BUFFER_OVERFLOW";
            break;
        case ERROR_INVALID_PARAMETER:
            err = "ERROR_INVALID_PARAMETER";
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            err = "ERROR_NOT_ENOUGH_MEMORY";
            break;
        case ERROR_NO_DATA:
            err = "ERROR_NO_DATA";
            break;
        default:
            err = "(UNKNOWN)";
            break;
        }
        printf("GetAdaptersAddresses failed: %s\n", err);
        return -1;
    }

    IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(size);
    if (!addresses)
    {
        assert(!addresses);
        return -1;
    }

    result = GetAdaptersAddresses(
        AF_UNSPEC,
        flags,
        NULL,
        addresses,
        &size
    );

    if (result != NO_ERROR) {
        const char* err;
        switch (result)
        {
        case ERROR_ADDRESS_NOT_ASSOCIATED:
            err = "ERROR_ADDRESS_NOT_ASSOCIATED";
            break;
        case ERROR_BUFFER_OVERFLOW:
            err = "ERROR_BUFFER_OVERFLOW";
            break;
        case ERROR_INVALID_PARAMETER:
            err = "ERROR_INVALID_PARAMETER";
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            err = "ERROR_NOT_ENOUGH_MEMORY";
            break;
        case ERROR_NO_DATA:
            err = "ERROR_NO_DATA";
            break;
        default:
            err = "(UNKNOWN)";
            break;
        }
        printf("GetAdaptersAddresses failed: %s\n", err);
        free(addresses);
        return -1;
    }

    size_t _net_interfaces_count = 0;
    net_interface_t* _net_interfaces = 0;

    for (IP_ADAPTER_ADDRESSES* adapter = addresses;
        adapter;
        adapter = adapter->Next)
    {
        _net_interfaces_count++;
    }

    _net_interfaces = (net_interface_t*)malloc(_net_interfaces_count * sizeof(net_interface_t));

    int i = 0;
    for (IP_ADAPTER_ADDRESSES* adapter = addresses;
        adapter;
        adapter = adapter->Next)
    {
        memset(_net_interfaces + i, 0, sizeof(net_interface_t));
        for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress;
            unicast;
            unicast = unicast->Next)
        {
            switch (unicast->Address.lpSockaddr->sa_family)
            {
            case AF_INET:
            {
                struct sockaddr_in* addr =
                    (struct sockaddr_in*)unicast->Address.lpSockaddr;
                inet_ntop(AF_INET,
                    &addr->sin_addr,
                    _net_interfaces[i].ipv4,
                    sizeof(_net_interfaces[i].ipv4));

                uint32_t broadcast_ip = ipv4_broadcast(addr, unicast->OnLinkPrefixLength);
                inet_ntop(AF_INET,
                    &broadcast_ip,
                    _net_interfaces[i].broadcast,
                    sizeof(_net_interfaces[i].broadcast));

                ULONG netmask;
                ConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &netmask);
                inet_ntop(AF_INET, &netmask, _net_interfaces[i].netmask, sizeof(_net_interfaces[i].netmask));

                _net_interfaces[i].prefix_length = unicast->OnLinkPrefixLength;
                break;
            }
            case AF_INET6:
            {
                struct sockaddr_in6* addr =
                    (struct sockaddr_in6*)unicast->Address.lpSockaddr;
                inet_ntop(AF_INET6, &addr->sin6_addr, _net_interfaces[i].ipv6, sizeof(_net_interfaces[i].ipv6));
                break;
            }
            }
        }

        memcpy(_net_interfaces[i].name, adapter->AdapterName, strlen(adapter->AdapterName) + 1);
        _net_interfaces[i].index = adapter->IfIndex;
        memcpy(_net_interfaces[i].mac, adapter->PhysicalAddress, sizeof(_net_interfaces[i].mac));
        _net_interfaces[i].mtu = adapter->Mtu;



        if (adapter->OperStatus == IfOperStatusUp)
        {
            _net_interfaces[i].running = true;
        }
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        {
            _net_interfaces[i].loopback = true;
        }

        size_t friendly_name_len = wcslen(adapter->FriendlyName);
        size_t description_len = wcslen(adapter->Description);

        wchar_t* friendly_name = malloc((friendly_name_len + 1) * sizeof(wchar_t));
        if (!friendly_name)
        {
            assert(friendly_name);
        }
        wchar_t* description = malloc((description_len + 1) * sizeof(wchar_t));
        if (!description)
        {
            assert(description);
        }

        wmemcpy(friendly_name, adapter->FriendlyName, friendly_name_len);
        wmemcpy(description, adapter->Description, description_len);

        friendly_name[friendly_name_len] = 0;
        description[description_len] = 0;

        _net_interfaces[i].friendly_name = friendly_name;
        _net_interfaces[i].description = description;

        i++;
    }

    *net_interfaces = _net_interfaces;
    *net_interfaces_count = _net_interfaces_count;

    free(addresses);
}
#else
int net_get_interfaces(net_interface_t** net_interfaces, size_t* net_interfaces_count)
{
    if (net_interfaces_count == 0) return 0;
    if (net_interfaces == 0) return 0;

    struct ifaddrs* ifaces;

    if (getifaddrs(&ifaces) == -1) {
        perror("getifaddrs");
        return -1;
    }

    size_t _net_interfaces_count = 0;
    net_interface_t* _net_interfaces = 0;

    for (struct ifaddrs* ifa = ifaces;
        ifa != NULL;
        ifa = ifa->ifa_next)
    {
        _net_interfaces_count++;
    }

    _net_interfaces = (net_interface_t*)malloc(_net_interfaces_count * sizeof(net_interface_t));

    int i = 0;
    for (struct ifaddrs* ifa = ifaces;
        ifa != NULL;
        ifa = ifa->ifa_next)
    {
        memset(_net_interfaces + i, 0, sizeof(net_interface_t));
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_flags & IFF_UP)
        {
            _net_interfaces[i].up = true;
        }
        if (ifa->ifa_flags & IFF_RUNNING)
        {
            _net_interfaces[i].running = true;
        }
        if (ifa->ifa_flags & IFF_LOOPBACK)
        {
            _net_interfaces[i].loopback = true;
        }

        int sock = socket(AF_INET, SOCK_DGRAM, 0);

        struct ifreq ifr = { 0 };

        snprintf(ifr.ifr_name,
            sizeof(ifr.ifr_name),
            "%s",
            ifa->ifa_name);

        if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
            _net_interfaces[i].mtu = ifr.ifr_mtu;
        }

        close(sock);

        memcpy(_net_interfaces[i].name, ifa->ifa_name, strlen(ifa->ifa_name) + 1);

        _net_interfaces[i].index = if_nametoindex(ifa->ifa_name);


        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            char address[INET_ADDRSTRLEN];

            struct sockaddr_in* addr =
                (struct sockaddr_in*)ifa->ifa_addr;

            inet_ntop(AF_INET,
                &addr->sin_addr,
                address,
                sizeof(address));

            memcpy(_net_interfaces[i].ipv4, address, strlen(address) + 1);

            struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;
            memcpy(_net_interfaces[i].mac, s->sll_addr, s->sll_halen);

            if (ifa->ifa_netmask) {
                struct sockaddr_in* mask =
                    (struct sockaddr_in*)ifa->ifa_netmask;

                char netmask[INET_ADDRSTRLEN];

                inet_ntop(AF_INET,
                    &mask->sin_addr,
                    netmask,
                    sizeof(netmask));

                memcpy(_net_interfaces[i].netmask, netmask, strlen(netmask) + 1);

                _net_interfaces[i].prefix_length = netmask_to_prefix(mask);
            }

            if (ifa->ifa_broadaddr) {
                struct sockaddr_in* broadcast =
                    (struct sockaddr_in*)ifa->ifa_broadaddr;

                char address[INET_ADDRSTRLEN];

                inet_ntop(AF_INET,
                    &broadcast->sin_addr,
                    address,
                    sizeof(address));

                memcpy(_net_interfaces[i].broadcast, address, strlen(address) + 1);
            }
        }

        else if (family == AF_INET6) {
            char address[INET6_ADDRSTRLEN];

            struct sockaddr_in6* addr =
                (struct sockaddr_in6*)ifa->ifa_addr;

            inet_ntop(AF_INET6,
                &addr->sin6_addr,
                address,
                sizeof(address));

            memcpy(_net_interfaces[i].ipv6, address, strlen(address) + 1);
        }

        i++;
    }

    *net_interfaces = _net_interfaces;
    *net_interfaces_count = _net_interfaces_count;
    freeifaddrs(ifaces);
}
#endif

void destroy_interfaces(net_interface_t* iface, size_t interface_count)
{
    if (!iface) return;
#ifdef _WIN32
    for (int i = 0; i < interface_count; i++)
    {
        free(iface[i].friendly_name);
        free(iface[i].description);
    }
#endif
    free(iface);
}

#ifdef _WIN32
static uint32_t net_get_default_ipv4_interface(void)
{
    PMIB_IPFORWARD_TABLE2 routes = NULL;

    DWORD result = GetIpForwardTable2(AF_INET, &routes);

    if (result != NO_ERROR)
        return 0;


    NET_IFINDEX default_interface = 0;

    for (ULONG i = 0; i < routes->NumEntries; ++i)
    {
        MIB_IPFORWARD_ROW2* route = &routes->Table[i];

        if (route->DestinationPrefix.PrefixLength != 0)
            continue;

        SOCKADDR_INET* destination = &route->DestinationPrefix.Prefix;

        if (destination->si_family != AF_INET)
            continue;

        if (destination->Ipv4.sin_addr.S_un.S_addr != 0)
            continue;

        default_interface = route->InterfaceIndex;

        break;
    }

    FreeMibTable(routes);

    return default_interface;
}
#else
static uint32_t net_get_default_ipv4_interface(void)
{
    int fd = socket(
        AF_NETLINK,
        SOCK_RAW,
        NETLINK_ROUTE
    );

    if (fd < 0)
        return 0;


    /*
     * Ask the kernel for the IPv4 routing table.
     */
    struct
    {
        struct nlmsghdr header;
        struct rtmsg route;
    } request = { 0 };

    request.header.nlmsg_len =
        NLMSG_LENGTH(sizeof(struct rtmsg));

    request.header.nlmsg_type =
        RTM_GETROUTE;

    request.header.nlmsg_flags =
        NLM_F_REQUEST | NLM_F_DUMP;

    request.header.nlmsg_seq = 1;

    request.route.rtm_family = AF_INET;


    struct sockaddr_nl kernel = { 0 };

    kernel.nl_family = AF_NETLINK;


    if (sendto(
        fd,
        &request,
        request.header.nlmsg_len,
        0,
        (struct sockaddr*)&kernel,
        sizeof(kernel)) < 0)
    {
        close(fd);
        return 0;
    }


    unsigned int result_interface = 0;

    char buffer[8192];

    for (;;)
    {
        ssize_t length = recv(
            fd,
            buffer,
            sizeof(buffer),
            0
        );

        if (length < 0)
        {
            if (errno == EINTR)
                continue;

            break;
        }

        if (length == 0)
            break;


        for (struct nlmsghdr* header =
            (struct nlmsghdr*)buffer;
            NLMSG_OK(header, (unsigned int)length);
            header = NLMSG_NEXT(
                header,
                length))
        {
            if (header->nlmsg_type == NLMSG_DONE)
                goto done;

            if (header->nlmsg_type == NLMSG_ERROR)
                goto done;


            if (header->nlmsg_type != RTM_NEWROUTE)
                continue;

            struct rtmsg* route =
                NLMSG_DATA(header);

            if (route->rtm_family != AF_INET)
                continue;

            if (route->rtm_dst_len != 0)
                continue;

            if (route->rtm_table != RT_TABLE_MAIN)
                continue;


            int attribute_length =
                RTM_PAYLOAD(header);


            for (struct rtattr* attribute =
                RTM_RTA(route);
                RTA_OK(
                    attribute,
                    attribute_length);
                attribute = RTA_NEXT(
                    attribute,
                    attribute_length))
            {
                if (attribute->rta_type != RTA_OIF)
                    continue;

                result_interface =
                    *(unsigned int*)RTA_DATA(attribute);

                goto done;
            }
        }
    }


done:

    close(fd);

    return result_interface;
}
#endif

int net_get_lan_ipv4(uint32_t* address, uint32_t* interface_index, uint8_t* prefix_length)
{
    if (!address)
        return 0;

    uint32_t default_interface = net_get_default_ipv4_interface();
    if (default_interface == 0)
        return 0;

    net_interface_t* interfaces = 0;
    size_t interface_count = 0;
    net_get_interfaces(&interfaces, &interface_count);
    bool found = false;
    for (int i = 0; i < interface_count; i++)
    {
        if (interfaces[i].index != (size_t)default_interface) continue;
        if (interfaces[i].loopback) continue;
#ifndef _WIN32
        if (!interfaces[i].up) continue;
#endif
        if (!interfaces[i].running) continue;

        if (strlen(interfaces[i].ipv4) == 0) continue;
        uint32_t ip = inet_addr(interfaces[i].ipv4);
        if ((ip >> 24) == 127) // 127.0.0.1/8
            continue;
        // APIPA/link-local:
        if ((ip >> 16) == 0xA9FE) // 169.254.0.0/16
            continue;

        *address = ip;
        *interface_index = interfaces[i].index;
        *prefix_length = interfaces[i].prefix_length;
        found = true;
        break;
    }
    if (!found)
    {
        for (int i = 0; i < interface_count; i++)
        {
            if (interfaces[i].index != (size_t)default_interface) continue;
            if (strlen(interfaces[i].ipv4) == 0) continue;
            uint32_t ip = inet_addr(interfaces[i].ipv4);
            *address = ip;
            *interface_index = interfaces[i].index;
            *prefix_length = interfaces[i].prefix_length;

            destroy_interfaces(interfaces, interface_count);

            return 1;
        }
    }
    destroy_interfaces(interfaces, interface_count);

    return 1;
}

void net_copy_interface(net_interface_t* dst_interface, net_interface_t* src_interface)
{
    if (dst_interface == 0) return;
    if (src_interface == 0) return;

    memcpy(dst_interface, src_interface, sizeof(net_interface_t));
#ifdef _WIN32
    dst_interface->friendly_name = malloc(wcslen(src_interface->friendly_name) + 1);
    dst_interface->description = malloc(wcslen(src_interface->description) + 1);
    wcscpy(dst_interface->friendly_name, src_interface->friendly_name);
    wcscpy(dst_interface->description, src_interface->description);
#endif
}

int net_get_interface_by_index(net_interface_t* interface, uint32_t index)
{
    if (interface == 0) return 0;
    net_interface_t* interfaces = 0;
    size_t count;
    if (net_get_interfaces(&interfaces, &count) == -1) return -1;
    bool found = false;
    memset(interface, 0, sizeof(net_interface_t));
    for (int i = 0; i < (int)count; i++)
    {
        if (interfaces[i].index == index)
        {
            if (strlen(interfaces[i].ipv4) == 0) continue;

            net_copy_interface(interface, interfaces + i);

            found = true;
            break;
        }
    }
    destroy_interfaces(interfaces, count);
    if (!found) return 0;
    return -1;
}