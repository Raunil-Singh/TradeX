#include "MarketFeedReader.h"
#include <sys/socket.h>
/*TODO:
1) Find a way to get ethernet IP address.
2) Come up with a way to deal with deal with lag out
3) Backoff class to deal with spinning on any new trade
*/
std::string get_interface_ip(const std::string& iface_name)
{
    struct ifaddrs* ifaddr;
    getifaddrs(&ifaddr);
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET && iface_name == ifa->ifa_name)
        {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, buf, sizeof(buf));
            freeifaddrs(ifaddr);
            return std::string(buf);
        }
    }
    freeifaddrs(ifaddr);
    throw std::runtime_error("interface not found: " + iface_name);
}
MarketFeedReader::MarketFeedReader() : trb(false)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct in_addr local_ip;
    std::string ip = get_interface_ip("en0");
    if (inet_pton(AF_INET, ip.data(), &local_ip) != 1)
    {
        perror("inet_pton local ip");
        exit(EXIT_FAILURE);
    } // using ethernet or wifi?, sets the ip address according

    int bufsize = 1 << 20;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)); // increases TX (send) buffer size
    addr.sin_family = AF_INET;                                            // operating on IPV4
    addr.sin_port = htons(5000);                                          // listening to port 5000, converts to big endian network order bytes?
    if (inet_pton(AF_INET, "239.1.1.1", &addr.sin_addr) != 1)
    {
        perror("inet_pton multicast");
        exit(EXIT_FAILURE);
    } // writes the binary address for the multicast group

    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &local_ip, sizeof(local_ip)); // setting interface based on ip
}

void MarketFeedReader::init_batch(batch_t *b, int cap)
{
    b->capacity = cap;
    b->msgs = (struct mmsghdr *)calloc(cap, sizeof(struct mmsghdr)); // creates pointer to structs of mmsghdr
    b->iov = (struct iovec *)calloc(cap, sizeof(struct iovec));      // creates pointers to the iovec structs
    b->buffer = (char *)aligned_alloc(64, cap * MAX_MSG_SIZE);       // the actual memory buffer

    for (int i = 0; i < cap; i++)
    {
        b->iov[i].iov_base = b->buffer + i * MAX_MSG_SIZE; // setting the base ptr for the message in the buffer
        b->iov[i].iov_len = 0;                             // no bytes of messages currently present
        b->msgs[i].msg_hdr.msg_iov = &b->iov[i];           // each mmsghdr gets its a pointer to the iov struct
        b->msgs[i].msg_hdr.msg_iovlen = 1;                 // number of messages in each iovec struct, we have a single message, MarketDataMessage
        b->msgs[i].msg_hdr.msg_name = &addr;               // address of the multicast group
        b->msgs[i].msg_hdr.msg_namelen = sizeof(addr);
    }
}

void MarketFeedReader::readThread()
{
    while (true)
    {
        if (trb.lagged_out())
        {
            // deal with lag
        }
        while (!trb.any_new_trade())
        {
            // spinning
        }
        queue.push(formatMarketData(trb.get_trade()));
    }
}

void MarketFeedReader::sendThread()
{
    batch_t *batch = (batch_t *)malloc(sizeof(batch_t));
    init_batch(batch, 64);

    while (true)
    {
        int msg_count = 0;

        while (msg_count < batch->capacity)
        {
            char *ptr = (char *)batch->iov[msg_count].iov_base; // get message buffer
            int msgs_packed = 0;

            while (msgs_packed < (int)msg_per_packet)
            {
                MarketDataMessage msg;
                while (!queue.pop(msg))
                {
                } // spin until we get one, maybe back off a acouple of times and exit and send incomplete packet
                memcpy(ptr + msgs_packed * sizeof(MarketDataMessage), &msg, sizeof(MarketDataMessage));
                msgs_packed++;
            }

            batch->iov[msg_count].iov_len = msgs_packed * sizeof(MarketDataMessage);
            msg_count++;
        }

        // partial send retry loop
        int to_send = msg_count;
        struct mmsghdr *cursor = batch->msgs;

        while (to_send > 0)
        {
            int sent = sendmmsg(sockfd, cursor, to_send, 0);
            if (sent < 0)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                case EAGAIN:
                    // back off and retry
                    break;
                case EBADF:
                case ENOTSOCK:
                case ENETDOWN:
                case ENETUNREACH:
                case EACCES:
                    perror("sendmmsg fatal"); // prints "sendmmsg fatal: <human readable error>" to stderr
                    // shutdown logic
                    break;
                default:
                    perror("sendmmsg");
                    break;
                }
            }
            cursor += sent;
            to_send -= sent;
        }
    }
}

MarketDataMessage MarketFeedReader::formatMarketData(matching_engine::Trade &&trade)
{
    MarketDataMessage out;
    out.price = trade.price;
    out.quantity = trade.quantity;
    out.symbol_id = trade.symbol_id;
    out.timestamp_ns = trade.timestamp_ns;
    out.trade_id = trade.trade_id;
    out.seq_num = seq_num++;
    return out;
}


