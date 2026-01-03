#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <random>

class ThreadPool{
    private:
       std::vector<std::thread> threads;
       std::queue<std::function<void()>> tasks;
       std::mutex queue_mutex;
       std::condition_variable cv;
       bool stop = false;

    public:
       ThreadPool(size_t numThreads){
           for(size_t i = 0; i<numThreads; i++){
               threads.emplace_back([this]{
                     while(true){
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);

                            cv.wait(lock, [this]{
                                return !tasks.empty() || stop;
                            });

                            if(stop && tasks.empty()){
                                return;
                            }

                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool(){
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }

            cv.notify_all();

            for(auto &thread : threads){
                thread.join();
            }
        }

        void enqueue(std::function<void()> task){
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace(std::move(task));
            }
            cv.notify_one();
        }
};

std::mutex cout_mtx;

int main(){

    std::cout<<"---------Execution using Thread Pool----------"<<std::endl;

    auto start1 = std::chrono::high_resolution_clock::now();
    {
        ThreadPool pool(4);

        for(int i = 0; i<50; i++){
            pool.enqueue([i]{
                
                long long sum = 0;
                thread_local std::mt19937 rng1(std::random_device{}());
                std::uniform_int_distribution<int> dist1(0, 100);
                for(long long j = 0; j<100000; j++){
                    sum+= dist1(rng1);
                }
                {
                    std::lock_guard<std::mutex> cout_lock(cout_mtx);
                    std::cout<<"Task : "<<i<<" Thread : "<<std::this_thread::get_id()<<" Sum : "<< sum<<std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            });
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration1 = end1 - start1;
    std::cout << "Time taken with ThreadPool: " << duration1.count() << " ms" << std::endl;

    std::cout<<std::endl;

    std::cout<<"---------Execution without Thread Pool----------"<<std::endl;
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        thread_local std::mt19937 rng2(std::random_device{}());
        std::uniform_int_distribution<int> dist2(0, 100);
        for(int i = 0; i<50; i++){
            long long sum = 0;
            for(int j = 0; j<100000; j++){
                sum+= dist2(rng2);
            }
            {
                std::lock_guard<std::mutex> cout_lock(cout_mtx);
                std::cout<<"Task : "<<i<<" Thread : "<<std::this_thread::get_id()<<" Sum : "<< sum<<std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration2 = end2 - start2;
    std::cout << "Time taken without ThreadPool: " << duration2.count() << " ms" << std::endl;
}
