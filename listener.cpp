#include "listener.h"

void Listener::init_batch(int capacity)
{
    batch->msgs = (struct mmsghdr*) calloc(capacity, sizeof(struct mmsghdr));
    batch->msgs = (struct iovec*) calloc(capacity, sizeof(struct iovec));
    batch->buffer = (char*) aligned_alloc(64, MAX_MSG_SIZE * capacity);

    for (int i = 0; i < capacity; i++)
    {
        batch->msgs[i]msg_iov = buffer + i * MAX_MSG_SIZE;
        batch->iov[i]->iov_base = buffer + i * MAX_MSG_SIZE;
        batch->iov[i]->iov_len = MAX_MSG_SIZE;
    }
}