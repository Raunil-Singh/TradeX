#include <iostream>
#include <chrono>

volatile long long sink = 0;

void fast_func(){
    long long sum = 0;
    for(int i = 0; i<1000000; i++){
        sum+=i;
    }
    sink = sum;
}

void slow_func(){
    double value = 1.0;
    for(int i = 0; i<1000000; i++){
        value *= 1.000001;
        value /= 1.0000001;
    }
    sink = static_cast<long long>(value);
}

void ver_slow_func(){
    long long ans = 0;
    for(int i = 0; i<100; i++){
        for(int j = 0; j<1000000; j++){
            ans += (i+j);
        }
    }
    sink = ans;
}

int main(){

    auto start1 = std::chrono::high_resolution_clock::now();
    fast_func();
    auto start2 = std::chrono::high_resolution_clock::now();
    slow_func();
    auto start3 = std::chrono::high_resolution_clock::now();
    ver_slow_func();
    auto end = std::chrono::high_resolution_clock::now();

    std::cout<<"Execution time of fast function : "<<std::chrono::duration_cast<std::chrono::milliseconds>(start2 - start1).count()<<"ms\n";
    std::cout<<"Execution time of slow function : "<<std::chrono::duration_cast<std::chrono::milliseconds>(start3 - start2).count()<<"ms\n";
    std::cout<<"Execution time of very slow function : "<<std::chrono::duration_cast<std::chrono::milliseconds>(end - start3).count()<<"ms\n";
}