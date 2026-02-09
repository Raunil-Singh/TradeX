#ifndef  ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstring>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "order.h"
#include "trade.h"
#include "absl/container/flat_hash_map.h"

namespace matching_engine {
struct OrderNode{
    Order order;
    uint32_t next;
    uint32_t back;
};

//Memory Pool
class MemPool{
    private:
       OrderNode* nodes;
       uint32_t freeHead;
       size_t used;
       size_t capacity;

    public:
       explicit MemPool(size_t cap) : capacity(cap), used(0) {
           nodes = static_cast<OrderNode*>(std::malloc(sizeof(OrderNode) * capacity));

           if(!nodes){
            throw std::bad_alloc();
           }
           freeHead = 0;
           for(size_t i = 0; i<capacity-1; i++){
               nodes[i].next = i+1;
           }
           nodes[capacity-1].next = NULL_IDX;
       }

       ~MemPool(){
          std::free(nodes);
       }

       uint32_t allocate(){
          if(freeHead == NULL_IDX) return NULL_IDX;

          uint32_t idx = freeHead;
          freeHead = nodes[idx].next;
          nodes[idx].next = NULL_IDX;
          nodes[idx].back = NULL_IDX;
          used++;
          return idx;
       }

       void deallocate(int idx){
           if(idx == NULL_IDX) return;

           nodes[idx].next = freeHead;
           freeHead = idx;
           used--;
       }

        OrderNode* getNode(int idx) {
            return &nodes[idx];
        }
};



//Queue for price levels
class PriceQueues {
    private:
        uint32_t head;
        uint32_t tail;
        size_t count;
        MemPool* pool;
    public: 
        explicit PriceQueues(MemPool* p) : head(NULL_IDX), tail(NULL_IDX), count(0), pool(p) {}

        OrderNode* gethead() {return pool->getNode(head);}

        bool isEmpty() const {
            return count == 0;
        }

        int insertOrder(Order& order){
            uint32_t idx = pool->allocate();
            if(idx == NULL_IDX) return NULL_IDX;

            OrderNode* node = pool->getNode(idx); 
            node->order = order;
            if(head == NULL_IDX){
                head = tail = idx;
            }
            else{
                pool->getNode(tail)->next = idx;
                pool->getNode(idx)->back = tail;
                tail = idx;
            }
            count++;
            return idx;
        }


        bool removeOrder(int idx){
            if(idx == NULL_IDX) return false;

            OrderNode* node = pool->getNode(idx);
            uint32_t prev = node->back;
            uint32_t next = node->next;

            if(idx == head){
                head = next;
                if(head != NULL_IDX)
                    pool->getNode(head)->back = NULL_IDX;
            }
            else{
                pool->getNode(prev)->next = next;
            }

            if(idx == tail){
                tail = prev;
            }
            else if(next != NULL_IDX){
                pool->getNode(next)->back = prev;
            }

            pool->deallocate(idx);
            count--;
            return true;
        }
};

//Order Book
class OrderBook{
    private:
       uint64_t lower_limit;
       uint64_t upper_limit;
       size_t PRICE_LEVELS;
       uint64_t best_buy_price;
       uint64_t best_sell_price;
       PriceQueues** price_levels;
       unsigned char* orderSide;
       MemPool pool;
       absl::flat_hash_map<uint64_t, uint32_t> orderMap;

    public:
       OrderBook(size_t poolSize, uint64_t lower_price, uint64_t upper_price) : pool(poolSize), lower_limit(lower_price), upper_limit(upper_price) {
           PRICE_LEVELS = upper_price - lower_price + 1;
           price_levels = new PriceQueues*[PRICE_LEVELS];
           orderSide = new unsigned char[PRICE_LEVELS];
           for(int i = 0; i < PRICE_LEVELS; i++) {
               price_levels[i] = new PriceQueues(&pool);
               orderSide[i] = '0';
            }
            orderMap.rehash(poolSize);
            best_buy_price= 0;
            best_sell_price = UINT64_MAX;
        }

        inline int priceToIndex(uint64_t price) const {
            return static_cast<int>(price) - lower_limit;  
        }


        uint64_t bestbuyprice() const { return best_buy_price; }

        uint64_t bestsellprice() const { return best_sell_price; }

        PriceQueues* getpricelevels(int idx) {
            return price_levels[idx];
        }

        unsigned char getorderside(uint64_t idx) const { return orderSide[idx]; }

        bool addOrder(Order &order){
           int index = priceToIndex(order.price);
           if(index<0 || index>=PRICE_LEVELS) return false;

           int idx = price_levels[index]->insertOrder(order);
           if(idx == NULL_IDX) return false;
           orderMap[order.order_id] = idx;
           if(order.type == OrderType::BUY && order.price > best_buy_price) best_buy_price = order.price;
           if(order.type == OrderType::SELL && order.price < best_sell_price) best_sell_price = order.price;

           if(order.type == OrderType::BUY) orderSide[index] = '1';
           if(order.type == OrderType::SELL) orderSide[index] = '2';
           return true;
       }

        Order* findOrder(uint64_t order_id) {
            auto it = orderMap.find(order_id);
            if(it == orderMap.end()) return nullptr;

            return &(pool.getNode(it->second)->order);
        }

        bool removeOrder(uint64_t price) {
            int idx = priceToIndex(price);
            if(orderSide[idx] == '0') return false;
            uint64_t id = price_levels[idx]->gethead()->order.order_id;
            
            auto it = orderMap.find(id);
            if(it == orderMap.end()) return false;

            int index = it->second;
            bool removed = price_levels[idx]->removeOrder(index); 
            if(removed){
                orderMap.erase(it);
                if(price_levels[idx]->isEmpty()) orderSide[idx] = '0';
            }
            return removed;
        }

        bool cancelOrder(uint64_t id){
            auto it = orderMap.find(id);
            if(it == orderMap.end()) return false;

            int idx = it->second;
            OrderNode* node = pool.getNode(idx);
            int i = priceToIndex(node->order.price);
            bool removed = price_levels[i]->removeOrder(idx);
            if(removed){
                orderMap.erase(it);
                if(price_levels[i]->isEmpty()) orderSide[i] = '0';
            }
            return removed;
        }

        bool modifyQuantity(uint64_t price, uint32_t new_qty) {
            int idx = priceToIndex(price);
            if(orderSide[idx] == '0') return false;
            price_levels[idx]->gethead()->order.quantity = new_qty;
            return true;
        }
        

        ~OrderBook() {
            for (int i = 0; i < PRICE_LEVELS; i++) {
                delete price_levels[i];
            }
            delete[] price_levels;
            delete[] orderSide;
        }

};

class OrderBookManager {
private:
    OrderBook** books;
    size_t numSymbols;

public:
    OrderBookManager(size_t nSymbols, size_t poolSize, uint64_t* lower_price, uint64_t* upper_price) : numSymbols(nSymbols)
    {
        books = new OrderBook*[numSymbols];

        for (size_t i = 0; i < numSymbols; i++) {
            books[i] = new OrderBook(poolSize,lower_price[i],upper_price[i]);
        }
    }

    OrderBook* get(uint32_t symbol_id) {
        return books[symbol_id];
    }

    ~OrderBookManager() {
        for (size_t i = 0; i < numSymbols; i++) {
            delete books[i];
        }
        delete[] books;
    }
};
}
#endif