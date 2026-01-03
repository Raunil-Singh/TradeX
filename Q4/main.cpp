#include <iostream>
#include <mutex>
#include <thread>
#include <random>
#include <vector>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <atomic>
long long num = 1;
std::mutex cout_mtx;

class Order
{
public:
    int order_id;
    double price;
    int quantity;

    Order(int id) : order_id(id)
    {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> price_dist(100.0, 200.0);
        std::uniform_int_distribution<int> qty_dist(10, 1000);

        price = price_dist(rng);
        quantity = qty_dist(rng);
    }
};

const size_t MAX_QUEUE_SIZE = 20;
int current_size = 0;
std::queue<Order> order_queue;
std::mutex mtx;
std::condition_variable cv_not_full;
std::condition_variable cv_not_empty;

std::atomic<bool> shutdown_flag(false);
std::atomic<int> active_producers{3};
std::atomic<size_t> max_buffer_occupancy{0};

std::vector<int> produced_count = {0, 0, 0};
std::vector<int> consumed_count = {0, 0};

void producer(int producer_id, int delay_ms, int base_order_id)
{
    int order_id = base_order_id;
    while (!shutdown_flag.load())
    {
        Order order(order_id);
        order_id++;
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (current_size >= MAX_QUEUE_SIZE && !shutdown_flag.load())
            {
                cv_not_full.wait(lock);
            }
            if (shutdown_flag.load()){
                break;
            }

            order_queue.push(order);
            current_size++;
            produced_count[producer_id]++;

            size_t curr_occ = current_size;
            size_t max_occ = max_buffer_occupancy.load();
            while (curr_occ > max_occ && !max_buffer_occupancy.compare_exchange_weak(max_occ, curr_occ)) {
                max_occ = max_buffer_occupancy.load();
            }

            cv_not_empty.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
    active_producers.fetch_sub(1);
    cv_not_empty.notify_all();
}

void consumer(int consumer_id)
{
    while (true)
    {
        Order order(0);
        {
            std::unique_lock<std::mutex> lock(mtx);
            while (current_size == 0 && active_producers.load() > 0)
            {
                cv_not_empty.wait(lock);
            }
            if (current_size == 0 && active_producers.load() == 0)
            {
                break;
            }

            order = order_queue.front();
            order_queue.pop();
            current_size--;
            consumed_count[consumer_id]++;

            cv_not_full.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

int main()
{
    std::thread fast_producer(producer, 0, 10, 1000);
    std::thread medium_producer(producer, 1, 25, 2000);
    std::thread slow_producer(producer, 2, 50, 3000);

    std::thread consumer1(consumer, 0);
    std::thread consumer2(consumer, 1);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    shutdown_flag.store(true);
    cv_not_full.notify_all();

    fast_producer.join();
    medium_producer.join();
    slow_producer.join();

    consumer1.join();
    consumer2.join();

    std::cout<<"Fast Producer produced "<<produced_count[0]<<" orders.\n";
    std::cout<<"Medium Producer produced "<<produced_count[1]<<" orders.\n";
    std::cout<<"Slow Producer produced "<<produced_count[2]<<" orders.\n";

    std::cout<<"Consumer 1 consumed "<<consumed_count[0]<<" orders.\n";
    std::cout<<"Consumer 2 consumed "<<consumed_count[1]<<" orders.\n";

    std::cout<<"Max buffer occupancy: "<<max_buffer_occupancy.load()<<"\n";

}