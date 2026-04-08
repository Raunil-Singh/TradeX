#ifndef MARKETREADER
#define MARKETREADER

#include "common.h"
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
#include <array>
/*TODO:
1) Figure out a better way for the tcp transmission to be non blocking
2) Verify the network models work
3) Come up with a way to get all the ip addresses
*/
namespace MFR{
class MarketFeedReader
{
    private:
        uint64_t global_seq{1};
        spsc_queue queue;
        int sockfd;
        int sockfd_tcp;
        struct sockaddr_in addr;
        struct sockaddr_in servaddr, client;
        int client_len;
        int tcp_port;
        int connect_;
        std::atomic_bool& flag;
        const size_t trades_to_be_read{23};
        uint64_t seq_num;
        std::array<TradeRingBuffer::trade_ring_buffer*, TradeRingBuffer::total_ring_buffer_count> trb_vec;
        std::atomic_bool done{false};
    public:
        MarketFeedReader(std::atomic_bool& flag);
        MarketDataMessage formatMarketData(matching_engine::Trade &&trade); // fix: return by value, not &&
        void readThread();
        void sendThread();
        void init_batch(batch_t*, int cap);
};
}
#endif