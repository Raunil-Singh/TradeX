#include "retransmitter.h"



Retransmitter::Retransmitter()
{
    init_batch(&batch, 64);
    int sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0); //UDP over IPV4
    int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0); // TCP over IPV4
    struct in_addr local_ip_udp, local_ip_tcp;

    //setting up listening on udp multicast
    inet_pton(AF_INET, "", &local_ip_udp); // will this process data over ethernet or wifi
    addr_udp.sin_port = htons(5000);
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_addr.s_addr = INADDR_ANY; // why am i not listening to just ethernet?
    bind(sockfd_udp, (sockaddr*) &addr_udp, sizeof(addr_udp));

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.1.1.1"); //subscribing to the multicast group
    mreq.imr_interface.s_addr = inet_addr("10.0.0.5"); // over ethernet
    setsockopt(sockfd_udp, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    
    //setting up tcp
    int opt = 1; //what role does opt play here;
    int PORT = 8080;
    setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_addr.s_addr = INADDR_ANY;
    addr_tcp.sin_port = htons(PORT);
    addrlen_tcp = sizeof(addr_tcp);
    if (bind(sockfd_tcp, (struct sockaddr *)&addr_tcp, sizeof(addr_tcp)) < 0) {
        perror("bind failed");
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
    while (true)
    {
        int ret = recvmmsg(sockfd_udp, batch.msgs, 64, MSG_DONTWAIT, NULL); // non-blocking batch receive
        for (int i = 0; i < ret; i++)
        {

            //parsing logic feels fragile
            MarketDataMessage* p_msg = (MarketDataMessage * ) batch.iov[i]->iov_base;
            for (int count = 0; count < (batch.iov[i].iov_len / sizeof(MarketDataMessage)); count++)
            {
                MarketDataMessage* msg = reinterpret_cast<MarketDataMessage*>(p_msg + 64); // trying to get the MarketDataMessage
                std::memcpy(buffer + (msg->seq_num & (SIZE - 1)), msg, sizeof(*msg));
                p_msg += sizeof(MarketDataMessage);
            }
        }
    }
}

void Retransmitter::listenerThread()
{
    while (true)
    {
        int new_socket = accept(sockfd_tcp, (struct sockaddr *)&addr_tcp, (socklen_t *)&addrlen_tcp);
        read(new_socket, tcp_buffer, sizeof(uint64_t));
        // fetch data how?? :- bitmask sequence_number, fetch from raw array, reconfigure queue
        //lets say you got the message
        uint64_t* seq_num = reinterpret_cast<uint64_t*>(tcp_buffer);
        if (buffer[*seq_num & (SIZE - 1)].seq_num == *seq_num) // sequence numbers match
        {

            send(sockfd_tcp, buffer + ((*seq_num) & (SIZE - 1)), sizeof(MarketDataMessage), MSG_DONTWAIT); // MSG_DONTWAIT :- non-blocking

        }
        else
        {
            //handle the lack of message
        }
    }
}


void Retransmitter::init_batch(batch_t *b, int cap)
{
    b->capacity = cap;
    b->msgs = (struct mmsghdr *)calloc(cap, sizeof(struct mmsghdr)); // creates pointer to structs of mmsghdr
    b->iov = (struct iovec *)calloc(cap, sizeof(struct iovec)); //creates pointers to the iovec structs
    b->buffer = (char *)aligned_alloc(64, cap * MAX_MSG_SIZE); //the actual memory buffer

    for (int i = 0; i < cap; i++)
    {
        b->iov[i].iov_base = b->buffer + i * MAX_MSG_SIZE; //setting the base ptr for the message in the buffer
        b->iov[i].iov_len = 0; //no bytes of messages currently present
        b->msgs[i].msg_hdr.msg_iov = &b->iov[i]; //each mmsghdr gets its a pointer to the iov struct
        b->msgs[i].msg_hdr.msg_iovlen = 1; //number of messages in each iovec struct, we have a single message, MarketDataMessage
        b->msgs[i].msg_hdr.msg_name = &addr_udp; //address of the multicast group
        b->msgs[i].msg_hdr.msg_namelen = sizeof(addr_udp);
    }
}