#ifndef MARKETREADER
#define MARKETREADER

#include "trade_ring_buffer.h"
#include "retransmitter.h"
#include "message.h"

class MarketFeedReader {
    private:
        TradeRingBuffer::trade_ring_buffer trb;
        uint64_t global_seq{1};
        Retransmitter rt;

    public:
        MarketFeedReader(): trb(false)
        {}//create Retransmitter class, create thread for retransmitter
        MarketDataMessage formatMarketData(matching_engine::Trade& trade);
        void send_multicast (MarketDataMessage& msg);
        
        void transmitThread ()
        {
            while(true) // add conditions later
            {
                if (trb.lagged_out())
                {
                    //deal with lag
                }
                while (!trb.any_new_trade())
                {
                    //spinning while waiting for trades
                }

                matching_engine::Trade trade = trb.get_trade();
                MarketDataMessage msg = formatMarketData(trade);
                send_multicast(msg);
                rt.store(msg); //call store within Retransmitter
            }
        }

};

#endif