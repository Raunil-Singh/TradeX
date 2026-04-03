#include "MarketFeedReader.h"
#include <sys/socket.h>
#include <chrono>

/*TODO:
1) Find a way to get ethernet IP address.
2) Come up with a way to deal with deal with lag out
3) Backoff class to deal with spinning on any new trade
*/

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
MarketFeedReader::MarketFeedReader() : trb(false)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct in_addr local_ip;
    // std::string ip = get_interface_ip("eno1");
    if (inet_pton(AF_INET, "10.50.59.247", &local_ip) != 1)
    {
        perror("inet_pton local ip");
        exit(EXIT_FAILURE);
    } // using ethernet or wifi?, sets the ip address according

    int bufsize = 1 << 20;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)); // increases TX (send) buffer size
    addr.sin_family = AF_INET;                                            // operating on IPV4
    addr.sin_port = htons(5000);                                          // listening to port 5000, converts to big endian network order bytes?
    if (inet_pton(AF_INET, "239.1.1.1", &addr.sin_addr) != 1) // was 239.1.1.1
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
    int trade_counter{};
    while (trade_counter < 1'000'960)
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
        trade_counter++;
    }
    done.store(true, std::memory_order_release);
    std:: cout << "reader done \n";
}

void MarketFeedReader::sendThread()
{
    batch_t *batch = (batch_t *)malloc(sizeof(batch_t));
    init_batch(batch, 64);
    int msgs_sent{};
    int msgs_packed{};
    while (!done.load(std::memory_order_acquire) || !queue.empty())
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
                    // std::cout << "stuck here \n";
                } // spin until we get one, maybe back off a acouple of times and exit and send incomplete packet
                memcpy(ptr + msgs_packed * sizeof(MarketDataMessage), &msg, sizeof(MarketDataMessage));
                msgs_packed++;
            }

            batch->iov[msg_count].iov_len = msgs_packed * sizeof(MarketDataMessage);
            msg_count++;
        }
        msgs_packed++;
        // std::cout << "all messages packed : " << msgs_packed<<"\n";
        // partial send retry loop
        int to_send = msg_count;
        struct mmsghdr *cursor = batch->msgs;

        while (to_send > 0)
        {
            int sent = sendmmsg(sockfd, cursor, to_send, 0);
            if (sent < 0)
            {
                // switch (errno)
                // {
                // case EINTR:
                //     continue;
                // case EAGAIN:
                //     // back off and retry
                //     break;
                // case EBADF:
                // case ENOTSOCK:
                // case ENETDOWN:
                // case ENETUNREACH:
                // case EACCES:
                //     perror("sendmmsg fatal"); // prints "sendmmsg fatal: <human readable error>" to stderr
                //     // shutdown logic
                //     break;
                // default:
                //     perror("sendmmsg");
                //     break;
                // } 
                perror("send");
                goto done_sending;  
            }
            cursor += sent;
            to_send -= sent;
        }
        msgs_sent++;
        // std::cout << "msgs sent : " << msgs_sent << "\n";
    }
    done_sending:
        std::cout << "exited sender \n";
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
matching_engine::Trade make_fake_trade(uint64_t i)
{
    matching_engine::Trade t;
    t.price        = 100.0 + (i % 100);
    t.quantity     = 10 + (i % 50);
    t.symbol_id    = i % 8;
    t.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    t.trade_id     = i;
    return t;
}
void pusher()
{
    TradeRingBuffer::trade_ring_buffer trb(true);
    matching_engine::Trade trade;
    for (int i = 0; i < 1'000'960; i++)
    {
        trade = make_fake_trade(i);
        trb.add_trade(trade);
    }
    std::cout<< "pusher done \n";
}
#include <thread>
int main(){
    MarketFeedReader mfr;
    std::thread push(pusher);
    push.join();
    auto tp = std::chrono::steady_clock::now();
    std::thread sender([&]{ mfr.sendThread(); });
    std::thread reader([&]{ mfr.readThread(); });
    sender.join();
    reader.join();

    auto tp2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp);
    std::cout << "Throughput: " << (1'000'960) / static_cast<double>(duration.count()) << "\n";
    std::cout << "Duration: " << duration.count() << "ms\n";
}
