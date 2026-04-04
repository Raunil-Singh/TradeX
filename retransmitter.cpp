#include "retransmitter.h"

/*TODO:
1) Get ip for wifi and ethernet
2) Test
3) Find a better way to handle all setup errors*/

std::atomic_bool done{false};
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
Retransmitter::Retransmitter() : buffer(new MarketDataMessage[SIZE]), tcp_buffer(new char[1 << 8])
{
    init_batch(&batch, 64);
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0); // UDP over IPV4
    if (sockfd_udp < 0)
    {
        perror("Error in UDP socket creation in Retransmitter");
        exit(EXIT_FAILURE);
    }

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0); // TCP over IPV4
    if (sockfd_tcp < 0)
    {
        perror("Error in UDP socket creation in Retransmitter");
        exit(EXIT_FAILURE);
    }
    struct in_addr local_ip_udp, local_ip_tcp;

    // setting up listening on udp multicast
    addr_udp.sin_port = htons(5000);
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_addr.s_addr = INADDR_ANY; // why am i not listening to just ethernet?
    if (bind(sockfd_udp, (sockaddr *)&addr_udp, sizeof(addr_udp)))
    {
        if (errno == EADDRINUSE)
        {
            // suggested: sleep/retry/fail, need guidance on which
        }
        perror("Issue in binding udp socket");
        exit(EXIT_FAILURE);
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.1.1.1"); // subscribing to the multicast group
    // std::string ip = get_interface_ip("en1"); //idk??
    mreq.imr_interface.s_addr = inet_addr("10.50.59.247");  // over ethernet
    if (inet_pton(AF_INET, "239.1.1.1", &mreq.imr_multiaddr) != 1)
    {
        perror("inet_pton multicast group");
        exit(EXIT_FAILURE);
    }

    if (inet_pton(AF_INET, "10.50.59.247", &mreq.imr_interface) != 1)
    {
        perror("inet_pton interface");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd_udp, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt failed while setting up membership to udp multicast");
        exit(EXIT_FAILURE);
    }

    // setting up tcp
    int opt = 1;
    int PORT = 8080;
    setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(PORT);
    addrlen_tcp = sizeof(addr_tcp);
    if (bind(sockfd_tcp, (struct sockaddr *)&addr_tcp, sizeof(addr_tcp)) < 0)
    {
        perror("bind failed for tcp socket");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd_tcp, connection_backlog) < 0) // need to handle this error
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

void Retransmitter::storeThread()
{
    int trades{};
    while (trades < 1'000'960) // global synchronization flag
    {
        int ret = recvmmsg(sockfd_udp, batch.msgs, 64, MSG_DONTWAIT, NULL); // non-blocking batch receive
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                continue;

            perror("recvmmsg");
            continue;
        }
        
        for (int i = 0; i < ret; i++)
        {

            // parsing logic feels fragile
            char *p_msg = (char *)batch.iov[i].iov_base;
            for (int count = 0; count < (batch.iov[i].iov_len / sizeof(MarketDataMessage)); count++)
            {
                MarketDataMessage *msg = reinterpret_cast<MarketDataMessage *>(p_msg); // trying to get the MarketDataMessage, why am i doing + 64???
                std::memcpy(buffer + (msg->seq_num & (SIZE - 1)), msg, sizeof(*msg));
                p_msg += sizeof(MarketDataMessage);
                trades++;
            }
        }
        std::cout << trades << "\n";
    }
    done.store(true, std::memory_order_release);
}

void Retransmitter::listenerThread()
{
    while (!done.load(std::memory_order_acquire))
    {
        int new_socket = accept(sockfd_tcp, (struct sockaddr *)&addr_tcp, (socklen_t *)&addrlen_tcp);
        if (new_socket < 0)
        {
            if (errno == EINTR)
            {
                perror("acceptance error");
                continue;
            }
        }
        size_t t{};
        while (t < 8)
        {
            size_t n = read(new_socket, tcp_buffer + t, sizeof(uint64_t) - t);
            if (n == 0)
            {
                // client closed connection error
                break;
            }
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    perror("error in reading from tcp connection");
                    continue;
                }
            }
            t += n;
        }
        // fetch data how?? :- bitmask sequence_number, fetch from raw array, reconfigure queue
        // lets say you got the message
        uint64_t *seq_num = reinterpret_cast<uint64_t *>(tcp_buffer);
        if (buffer[*seq_num & (SIZE - 1)].seq_num == *seq_num) // sequence numbers match
        {
            size_t n = send(new_socket, buffer + ((*seq_num) & (SIZE - 1)), sizeof(MarketDataMessage), MSG_DONTWAIT | MSG_NOSIGNAL); // MSG_DONTWAIT :- non-blocking, MSG_NOSIGNAL :- dont crash everything
            if (n < 0){
                if (errno == EINTR || errno == EAGAIN){
                    size_t n = send(new_socket, buffer + ((*seq_num) & (SIZE - 1)), sizeof(MarketDataMessage), MSG_DONTWAIT | MSG_NOSIGNAL); // try sending again once
                }
                else if (errno == EPIPE)
                {
                    //do nothing
                }
                else
                {
                    perror("issue in send");
                }
            }
            if (n < sizeof(MarketDataMessage))
            {
                //do we retry send
            }
        }
        else
        {
            // handle the lack of message by outputting zeroes
        }
        close(new_socket);
    }
}

void Retransmitter::init_batch(batch_t *b, int cap)
{
    b->capacity = cap;
    b->msgs = (struct mmsghdr *)calloc(cap, sizeof(struct mmsghdr)); // creates pointer to structs of mmsghdr
    b->iov = (struct iovec *)calloc(cap, sizeof(struct iovec));      // creates pointers to the iovec structs
    b->buffer = (char *)aligned_alloc(64, cap * MAX_MSG_SIZE);       // the actual memory buffer

    for (int i = 0; i < cap; i++)
    {
        b->iov[i].iov_base = b->buffer + i * MAX_MSG_SIZE; // setting the base ptr for the message in the buffer
        b->iov[i].iov_len = MAX_MSG_SIZE;                  // no bytes of messages currently present
        b->msgs[i].msg_hdr.msg_iov = &b->iov[i];           // each mmsghdr gets its a pointer to the iov struct
        b->msgs[i].msg_hdr.msg_iovlen = 1;                 // number of messages in each iovec struct, we have a single message, MarketDataMessage
        b->msgs[i].msg_hdr.msg_name = nullptr;
        b->msgs[i].msg_hdr.msg_namelen = 0;
    }
}
#include <thread>
#include <chrono>
int main()
{
    Retransmitter rt;
    auto tp = std::chrono::steady_clock::now();
    std::thread store ([&]{ rt.storeThread(); });
    std::thread listen ([&]{ rt.listenerThread(); });
    store.join();
    listen.join();
    auto tp2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp);
    std::cout << duration.count() << "ms";
}