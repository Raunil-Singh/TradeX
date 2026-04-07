#ifndef MARKETREADER
#define MARKETREADER


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <ifaddrs.h>
#include <string>
#include <atomic>
#include "trade_ring_buffer.h"
#include "message.h"
#include "spsc_queue.h"
/*TODO:
1) Figure out a better way for the tcp transmission to be non blocking
2) Verify the network models work
3) Come up with a way to get all the ip addresses
*/
constexpr size_t msg_per_packet = 23;
constexpr size_t MAX_MSG_SIZE   = msg_per_packet * sizeof(MarketDataMessage); // 23 * 64 = 1472
static uint64_t seq_num{};
typedef struct {
    struct mmsghdr *msgs;
    struct iovec   *iov;
    char           *buffer;
    int             capacity;

    char* tcp_buffer;
    size_t tcp_capacity;
} batch_t;

class MarketFeedReader
{
    private:
        TradeRingBuffer::trade_ring_buffer trb;
        uint64_t global_seq{1};
        spsc_queue queue;
        int sockfd;
        int sockfd_tcp;
        struct sockaddr_in addr;
        struct sockaddr_in servaddr, client;
        int client_len;
        int tcp_port;
        int connect_;
        // std::atomic_bool done{false};
    public:
        MarketFeedReader(int id);
        MarketDataMessage formatMarketData(matching_engine::Trade &&trade); // fix: return by value, not &&
        void readThread();
        void sendThread();
        void init_batch(batch_t*, int cap);
};

#endif