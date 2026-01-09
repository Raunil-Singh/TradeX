#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <random>
#include <queue>
#include <chrono>
#include <random>
#include <atomic>


const int BUFFER_CAPACITY = 20;
const int RUN_TIME_SECONDS = 5;

struct Order
{
    int order_id;
    double price;
    int quantity;
};

std::queue<Order> buffer;
int buffer_size = 0;

std::mutex buffer_mutex;
std::condition_variable not_full;
std::condition_variable not_empty;

std::atomic<bool> stop_production{false};

std::atomic<int> fast_count{0};
std::atomic<int> medium_count{0};
std::atomic<int> slow_count{0};

std::atomic<int> first_consumed_count{0};
std::atomic<int> second_consumed_count{0};

std::atomic<int> max_buffer_occupancy{0};


void producer(int producer_id, int delay_ms, std::atomic<int>& produced_count)
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_real_distribution<double> price_distribution(99.0, 101.0);
    std::uniform_int_distribution<int> quantity_distribution(1, 100);

    int order_id = 0;

    while (!stop_production)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        Order order{
            producer_id * 1'000'000 + order_id++,
            price_distribution(mt),
            quantity_distribution(mt)
        };

        std::unique_lock<std::mutex> lock(buffer_mutex);

        not_full.wait(lock, [] {
            return buffer_size < BUFFER_CAPACITY || stop_production;
        });

        if (stop_production)
            break;

        buffer.push(order);
        buffer_size++;

        produced_count++;

        max_buffer_occupancy = std::max(max_buffer_occupancy.load(), buffer_size);

        not_empty.notify_one();
    }
}

void consumer(int consumer_id, std::atomic<int>& consumed_count)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(buffer_mutex);

        not_empty.wait(lock, [] {
            return buffer_size > 0 || stop_production;
        });

        if (buffer_size == 0 && stop_production)
            break;

        Order order = buffer.front();
        buffer.pop();
        buffer_size--;

        not_full.notify_one();

        lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(15));

        consumed_count++;
    }
}

int main()
{
    std::thread fast_producer(producer, 1, 10, std::ref(fast_count));
    std::thread medium_producer(producer, 2, 25, std::ref(medium_count));
    std::thread slow_producer(producer, 3, 50, std::ref(slow_count));

    std::thread consumer_a(consumer, 1, std::ref(first_consumed_count));
    std::thread consumer_b(consumer, 2, std::ref(second_consumed_count));

    std::this_thread::sleep_for(std::chrono::seconds(RUN_TIME_SECONDS));
 
    stop_production = true;
    not_full.notify_all();
    not_empty.notify_all();

    fast_producer.join();
    medium_producer.join();
    slow_producer.join();
    consumer_a.join();
    consumer_b.join();

    std::cout << "\n\nAnalysis:\n\n";
    std::cout << "Produced (fast):   " << fast_count << "\n";
    std::cout << "Produced (medium): " << medium_count << "\n";
    std::cout << "Produced (slow):   " << slow_count << "\n";

    std::cout << "Consumed (C1):     " << first_consumed_count << "\n";
    std::cout << "Consumed (C2):     " << second_consumed_count << "\n";

    std::cout << "Max buffer size:   " << max_buffer_occupancy << "\n\n\n";
}
