#ifndef  ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstring>
#include <iostream>
#include <cstdint>
#include "order.h"
#include "trade.h"

struct OrderNode{
    Order order;
    OrderNode* next;
};

//Memory Pool
class MemPool{
    private:
       OrderNode* nodes;
       OrderNode* freeHead;
       size_t used;
       size_t capacity;

    public:
       explicit MemPool(size_t cap) : capacity(cap), used(0) {
           nodes = static_cast<OrderNode*>(std::malloc(sizeof(OrderNode) * capacity));

           if(!nodes){
            throw std::bad_alloc();
           }
           freeHead = &nodes[0];
           for(size_t i = 0; i<capacity-1; i++){
               nodes[i].next = &nodes[i+1];
           }
           nodes[capacity-1].next = nullptr;
       }

       ~MemPool(){
          std::free(nodes);
       }

       OrderNode* allocate(){
          if(!freeHead) return nullptr;

          OrderNode* node = freeHead;
          freeHead = freeHead->next;
          node->next = nullptr;
          used++;
          return node;
       }

       void deallocate(OrderNode* node){
           if(!node) return;

           node->next = freeHead;
           freeHead = node;
           used--;
       }
};

//Queue for price levels
class PriceQueues {
    private:
        OrderNode* head;
        OrderNode* tail;
        size_t count;
        MemPool* pool;
    public: 
        explicit PriceQueues(MemPool* p) : head(nullptr), tail(nullptr), count(0), pool(p) {}

        OrderNode* gethead() {return head;}

        bool isEmpty() const {
            return count == 0;
        }

        bool insertOrder(Order& order){
            OrderNode* node = pool->allocate();
            if(!node) return false;

            node->order = order;
            if(!head){
                head = tail = node;
            }
            else{
                tail->next = node;
                tail = node;
            }
            count++;
            return true;
        }

        Order* findOrder(uint64_t id){
            OrderNode* curr = head;
            while(curr){
                if(curr->order.order_id == id){
                    return &curr->order;
                }
                curr = curr->next;
            }
            return nullptr;
        }

        bool removeOrder(uint64_t id) {
            OrderNode* prev = nullptr;
            OrderNode* curr = head;

            while (curr) {
                if (curr->order.order_id == id) {
                    if (prev){
                        prev->next = curr->next;
                    }
                    else{
                        head = curr->next;
                    }

                    if (curr == tail){
                        tail = prev;
                    }

                    pool->deallocate(curr);
                    count--;
                    return true;
                }
                prev = curr;
                curr = curr->next;
            }
            return false;
        }
};

//Order Book
class OrderBook{
    private:
       uint8_t symbol_id;
       uint64_t best_buy_price;
       uint64_t best_sell_price;
       PriceQueues* price_levels[PRICE_LEVELS];
       unsigned char orderSide[PRICE_LEVELS];
       MemPool pool;

    public:
       OrderBook::OrderBook(size_t poolSize) : pool(poolSize) {
           for(int i = 0; i < PRICE_LEVELS; i++) {
               price_levels[i] = new PriceQueues(&pool);
            }
            for(int i = 0; i<PRICE_LEVELS; i++) {
                orderSide[i] = 0;
            }
            best_buy_price= -1;
            best_sell_price = -1;
        }

        inline int priceToIndex(uint64_t price) const {
            int ticks = static_cast<int>(price * PRICE_SCALE);
            return ticks - LOWER_LIMIT_TICKS;
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

           if(order.type == BUY && order.price < best_buy_price) best_buy_price = order.price;
           if(order.type == SELL && order.price > best_sell_price) best_sell_price = order.price;

           if(order.type == BUY) orderSide[index] = 1;
           if(order.type == SELL) orderSide[index] = 2;
           return price_levels[index]->insertOrder(order);
       }

        Order* findOrder(uint64_t order_id) {
            for (int i = 0; i < PRICE_LEVELS; i++) {
                if (price_levels[i]) {
                    Order* o = price_levels[i]->findOrder(order_id);
                    if (o) return o;
                }
            }
            return nullptr;
        }

        bool removeOrder(uint64_t price) {
            int idx = priceToIndex(price);
            uint64_t id = price_levels[idx]->gethead()->order.order_id;
            for (int i = 0; i < PRICE_LEVELS; i++) {
                if (price_levels[i] && price_levels[i]->removeOrder(id)) {
                    if (price_levels[i]->isEmpty()) {
                        orderSide[i] = 0;
                    }
                    return true;
                }
            }
            return false;
        }

        bool modifyQuantity(uint64_t price, uint32_t new_qty) {
            int idx = priceToIndex(price);
            price_levels[idx]->gethead()->order.quantity = new_qty;
            return true;
        }
        

        ~OrderBook() {
            for (int i = 0; i < PRICE_LEVELS; i++) {
                delete price_levels[i];
            }
        }

};

class OrderBookManager {
private:
    OrderBook** books;
    size_t numSymbols;

public:
    OrderBookManager(size_t nSymbols, size_t poolSize) : numSymbols(nSymbols)
    {
        books = new OrderBook*[numSymbols];

        for (size_t i = 0; i < numSymbols; i++) {
            books[i] = new OrderBook(poolSize);
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

#endif