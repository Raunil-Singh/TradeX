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
           nodes = new OrderNode[capacity];

           freeHead = &nodes[0];
           for(size_t i = 0; i<capacity-1; i++){
               nodes[i].next = &nodes[i+1];
           }
           nodes[capacity-1].next = nullptr;
       }

       ~MemPool(){
          delete[] nodes;
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

        Order* findSymbol(char* symbol){
            OrderNode* curr = head;
            while(curr){
                if(curr->order.symbol == symbol){
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
       PriceQueues* price_levels[PRICE_LEVELS];
       MemPool pool;

       inline int priceToIndex(double price) const {
            int ticks = static_cast<int>(price * PRICE_SCALE + 0.5);
            return ticks - LOWER_LIMIT_TICKS;
        }

    public:
       explicit OrderBook(size_t poolSize) : pool(poolSize){
        memset(price_levels, 0, sizeof(price_levels));
       }

       bool addOrder(Order &order){
           int index = priceToIndex(order.price);
           if(index<0 || index>=PRICE_LEVELS) return false;

           if(!price_levels[index]) {
               price_levels[index] = new PriceQueues(&pool);
           }

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

        bool removeOrder(uint64_t order_id) {
            for (int i = 0; i < PRICE_LEVELS; i++) {
                if (price_levels[i] && price_levels[i]->removeOrder(order_id)) {
                    return true;
                }
            }
            return false;
        }

        bool modifyQuantity(uint64_t order_id, uint32_t new_qty) {
            Order* order = findOrder(order_id);
            if (!order) return false;

            order->quantity = new_qty;
            return true;
        }

 /*       Trade* marketMatching(Order* order){
            double price = order->price;
            OrderType t = order->type;
            char* symbol = order->symbol;

            int index = priceToIndex(price);

        }
*/

        ~OrderBook() {
            for (int i = 0; i < PRICE_LEVELS; i++) {
                delete price_levels[i];
            }
        }

};

#endif