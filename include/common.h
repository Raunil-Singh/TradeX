
#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/socket.h>
#include "message.h"

// Shared constants
constexpr size_t msg_per_packet = 23;
constexpr size_t MAX_MSG_SIZE   = msg_per_packet * sizeof(MarketDataMessage);

// Shared struct
typedef struct {
    struct mmsghdr *msgs;
    struct iovec *iov;
    char * buffer;
    char * tcp_buffer;
    int capacity;
    int tcp_capacity;
} batch_t;

#endif