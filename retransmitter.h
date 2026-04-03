#define _GNU_SOURCE
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <string>

#include "message.h"
#include "spsc_queue.h"


typedef struct {
    struct mmsghdr *msgs;
    struct iovec   *iov;
    char           *buffer;
    int             capacity;
} batch_t;

constexpr size_t msg_per_packet = 23;
constexpr size_t MAX_MSG_SIZE   = msg_per_packet * sizeof(MarketDataMessage);
constexpr size_t connection_backlog{10}; //what should it be?
static const uint64_t SIZE{1 << 20}; // configure slot sizes to service requests for sequence numbers transmitted 1s ago
class Retransmitter{
    private:
        spsc_queue queue;
        MarketDataMessage buffer[SIZE];
        int sockfd_udp;
        int sockfd_tcp;
        struct sockaddr_in addr_udp, addr_tcp;
        int addrlen_tcp;
        batch_t batch;
        char tcp_buffer[1 << 8]; //should be size of sequence number
    public:
        Retransmitter();
        void storeThread(); // listenes on MarketFeed's UDP connection 
        void listenerThread(); //separate thread that processes requests by clients for missing messages
        void init_batch(batch_t*, int cap);
};