#include "listener.h"
namespace Client{
    bool spsc_queue::push(uint64_t&& msg)
    {
        uint64_t _head = head.load(std::memory_order_acquire);
        uint64_t _tail = tail.load(std::memory_order_relaxed);

        if (((_tail + 1) & (queue_buffer_size - 1)) != (_head & (queue_buffer_size - 1))) // queue not full
        {
            array[_tail & (queue_buffer_size - 1)] = std::move(msg);
            tail.store(_tail + 1, std::memory_order_release);
            return true;
        }

        return false;
    }

    bool spsc_queue::pop(uint64_t& out_msg)
    {
        uint64_t _head = head.load(std::memory_order_relaxed);
        uint64_t _tail = tail.load(std::memory_order_acquire);

        if (_tail != _head) // queue not empty
        {
            out_msg = array[_head & (queue_buffer_size - 1)];
            head.store(_head + 1, std::memory_order_release);
            return true;
        }

        return false;
    }
    void Listener::init_batch(int capacity)
    {
        batch.msgs = (struct mmsghdr*) calloc(capacity, sizeof(struct mmsghdr));
        batch.iov = (struct iovec*) calloc(capacity, sizeof(struct iovec));
        batch.buffer = (char*) aligned_alloc(64, MAX_MSG_SIZE * capacity);

        for (int i = 0; i < capacity; i++)
        {
            batch.iov[i].iov_base = batch.buffer + i * MAX_MSG_SIZE;
            batch.iov[i].iov_len = MAX_MSG_SIZE;
            batch.msgs[i].msg_hdr.msg_iov = &batch.iov[i];
            batch.msgs[i].msg_hdr.msg_iovlen = 1; //one message per datagram -> consisting of 23 packets
            batch.msgs[i].msg_hdr.msg_name = nullptr; //stores no address (receiver side)
            batch.msgs[i].msg_hdr.msg_namelen = 0;
        }
    }

    Listener::Listener() : last_seq_num{}
    {
        sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0); // UDP over IPV4
        if (sockfd_udp < 0)
        {
            perror("Error in UDP socket creation in Retransmitter");
            exit(EXIT_FAILURE);
        }

        

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


    }

    void Listener::listener()
    {
        while (true)
        {
            int ret = recvmmsg(sockfd_udp, batch.msgs, 64, MSG_DONTWAIT, NULL); // non-blocking batch receive
            if (ret < 0)
            {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                perror("recvmmsg");
                continue;
            }
            uint64_t identity_seq_num =  ((MarketDataMessage*)batch.iov[0].iov_base)->seq_num; //getting first sequence number;
            if (identity_seq_num != last_seq_num + msg_per_packet)
            {
                //create a request for missing_packet_request probably launch a thread for that or an async task
                int missing_packets = (identity_seq_num - last_seq_num) / msg_per_packet - 1;
                for (int i = 0; i < missing_packets; i++)
                {
                    last_seq_num += 23;
                    queue.push(std::move(last_seq_num)); // sequence number. Change retransmitter to send back a whole packet
                }
            }
            last_seq_num =  identity_seq_num; //I think this should be handled both ways, but just becasue
            for (int i = 0; i < ret; i++)
            {

                // parsing logic feels fragile
                char *p_msg = (char *)batch.iov[i].iov_base;
                for (int count = 0; count < (batch.iov[i].iov_len / sizeof(MarketDataMessage)); count++)
                {
                    MarketDataMessage *msg = reinterpret_cast<MarketDataMessage *>(p_msg); // trying to get the MarketDataMessage, why am i doing + 64???
                    // std::memcpy(buffer + (msg->seq_num & (SIZE - 1)), msg, sizeof(*msg)); //confused as to what listener will do
                    p_msg += sizeof(MarketDataMessage);
                }
            }
        }
    }

    void Listener::missing_packet_request()
    {
        uint64_t seq_num{};
        while (true)
        {
                // setting up tcp
                //move to create request for missing seq num
            sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0); // TCP over IPV4
            if (sockfd_tcp < 0)
            {
                perror("Error in UDP socket creation in Retransmitter");
                exit(EXIT_FAILURE);
            }
            struct in_addr local_ip_udp, local_ip_tcp;
            int opt = 1;
            int PORT = 8080;
            setsockopt(sockfd_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            addr_tcp.sin_family = AF_INET;
            if (inet_pton(AF_INET, "", &addr_tcp.sin_addr) <= 0)
            {
                std::cerr << "Invalid address for server\n";
            }
            addr_tcp.sin_port = htons(PORT);
            addrlen_tcp = sizeof(addr_tcp);

            while (!queue.pop(seq_num))
            {
                continue;
            }

            //open tcp connection
            int status = connect(sockfd_tcp, (struct sockaddr*)&addr_tcp, sizeof(addr_tcp));
            if (status < 0)
            {
                switch(errno){
                    //handle errors
                }
                    
            }
            int ret = send(sockfd_tcp, &seq_num, sizeof(seq_num), 0); // get just that trade or all 23 trades in a packet???
            if (ret < 0)
            {
                //handle errors
            }
            // put it in smth
            close(sockfd_tcp);
            
        }
    }
}