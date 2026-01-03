#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>

std:: mutex mtx;
void increment1(long long &counter){
    for(int i = 0; i<10000000; i++){
        std::lock_guard<std::mutex> lock(mtx);
        counter++;
    }
}

void increment2(std::atomic<int> &counter){
    for(int i = 0; i<10000000; i++){
        counter.fetch_add(1,std::memory_order_relaxed);
    }
}

void increment3(long long &counter){
    long long temp = 0;
    for(int i = 0; i<10000000; i++){
        temp++;
    }
    std::lock_guard<std::mutex> lock(mtx);
    counter += temp;
}
 
void mutex_inc(){
    std::cout<<"Mutex Version : \n";
    long long counter = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(increment1, std::ref(counter));
    std::thread t2(increment1, std::ref(counter));
    std::thread t3(increment1, std::ref(counter));
    std::thread t4(increment1, std::ref(counter));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout<<"Counter value : "<<counter<<"\n";
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

void atomic_inc(){
    std::cout<<"Atomic Version : \n";
    std::atomic<int> counter{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(increment2, std::ref(counter));
    std::thread t2(increment2, std::ref(counter));
    std::thread t3(increment2, std::ref(counter));
    std::thread t4(increment2, std::ref(counter));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    auto end = std::chrono::high_resolution_clock::now();;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout<<"Counter value : "<<counter.load()<<"\n";
    std::cout<<"Execution Time : "<<duration.count()<<"\n";
}

void thread_local_inc(){
    std::cout<<"Thread Local Version : \n";
    long long counter = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(increment3, std::ref(counter));
    std::thread t2(increment3, std::ref(counter));
    std::thread t3(increment3, std::ref(counter));
    std::thread t4(increment3, std::ref(counter));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout<<"Counter value : "<<counter<<"\n";
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

int main(){
    mutex_inc();
    atomic_inc();
    thread_local_inc();
}