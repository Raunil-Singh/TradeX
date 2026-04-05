// Listens to udp feed as well and queries for lost packets
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
#include <atomic>
#include <iostream>

#include "message.h"

constexpr size_t msg_per_packet = 23;
constexpr size_t MAX_MSG_SIZE   = msg_per_packet * sizeof(MarketDataMessage);

typedef struct {
    struct mmsghdr *msgs;
    struct iovec *iov;
    char * buffer;
    int capacity
} batch_t;
class Listener
{

    public:
        Listener();
        void listener();
        void missingPackRequest();
        void init_batch(int capacity);
}