#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <cmath>

inline double heavyCompute(double a, double b) {
    double x = a;
    for (int i = 0; i < 10; ++i) {
        x = std::log(x + b)
          + std::sqrt(x + 1.5)
          + std::exp(-x * 0.01);
    }
    return x;
}


constexpr int OPTIONS{10'000};
constexpr int SIMPLE_OPTIONS{static_cast<int>(.8 * OPTIONS)};
constexpr int BLACK_SCHOLES_OPTIONS{static_cast<int>(0.2 * OPTIONS)};

int THREADS{};
int remS{};
int remBS{};
int SEGMENT_SIZE_SIMPLE{};
int SEGMENT_SIZE_BS{};

std::uniform_int_distribution<> quantityDistribution(1, 100);
std::uniform_real_distribution<> priceDistribution(1, 100);
std::uniform_real_distribution<double> baseDist(1.0, 100.0);
std::uniform_real_distribution<double> factorDist(0.1, 2.0);

std::mt19937 rGen(std::random_device{}());
std::vector<long long> sumRecorder;

std::vector<int> simpleQuantityVector(SIMPLE_OPTIONS);
std::vector<int> simplePriceVector(SIMPLE_OPTIONS);
std::vector<double> baseVector(BLACK_SCHOLES_OPTIONS);
std::vector<double> factorVector(BLACK_SCHOLES_OPTIONS);

void setup(){
    for(int i = 0; i < SIMPLE_OPTIONS; i++){
        simpleQuantityVector[i] = quantityDistribution(rGen);
        simplePriceVector[i] = priceDistribution(rGen);
    }

    for(int i = 0; i < BLACK_SCHOLES_OPTIONS; i++){
        baseVector[i] = baseDist(rGen);
        factorVector[i] = factorDist(rGen);
    }
}

void worker(int id){
    long long sum{0};

    //t_i

    for(int i = id * SEGMENT_SIZE_SIMPLE; i < (id + 1) * SEGMENT_SIZE_SIMPLE; i++){
        sum += simplePriceVector[i] * simpleQuantityVector[i];//havent put formula
    }
    if(id < remS){
        int extra = SIMPLE_OPTIONS - 1 - id;
        sum += simplePriceVector[extra] * simpleQuantityVector[extra];
    }
    for(int i = id * SEGMENT_SIZE_BS; i < (id + 1) * SEGMENT_SIZE_BS; i++){
        sum += heavyCompute(baseVector[i], factorVector[i]);//havent put formula
    }
    if(id < remBS){
        sum += heavyCompute(baseVector[BLACK_SCHOLES_OPTIONS - 1 - id], factorVector[BLACK_SCHOLES_OPTIONS - 1 - id]);
    }
    sumRecorder[id] = sum;
}

int main(int argc, char* argv[]){
    //will include a command here for options initialization
    if(argc != 2){
        std::cout<<"Usage ./(exec_file_name) THREAD_COUNT\n";
        return EXIT_FAILURE;
    }else{
        THREADS = atoi(argv[1]);
    }
    setup();
    remS = SIMPLE_OPTIONS % THREADS;
    remBS = BLACK_SCHOLES_OPTIONS % THREADS;
    SEGMENT_SIZE_SIMPLE = SIMPLE_OPTIONS/THREADS;
    SEGMENT_SIZE_BS = BLACK_SCHOLES_OPTIONS/THREADS;
    for(int i = 0; i < THREADS; i++){
        sumRecorder.push_back(0);
    }
    //Total
    auto trueStart = std::chrono::steady_clock::now();
    std::vector<std::thread> threadPool;
    threadPool.reserve(THREADS);

    //parallel
    for(int i = 0; i < THREADS; i++){
        threadPool.emplace_back(worker, i);
    }
    for(auto& thread: threadPool){
        thread.join();
    }

    //sequential
    long long value{};
    for(long long sum: sumRecorder){
        value += sum;
    }


    auto trueEnd = std::chrono::steady_clock::now();
    auto trueDuration = trueEnd - trueStart;
    std::cout<<"Duration: "<<std::chrono::duration_cast<std::chrono::microseconds>(trueDuration).count()<<"microseconds. \n";
    std::cout<<"Value: "<<value<<std::endl;
}
/* 
includes
constants and useful data structures

worker function definition{
    local time variable "start"
    local "sum"
    
    calculation of simple options, with parallelization

    time variable "checkpoint 1"

    calculation of black scholes model

    time variable "check point2"

    storing sum in a vector with a specific location for that specific thread
    calculate separate times

}

main thread{
    threadpool creation
    threadpool join all threads
    record time in a variable
}
*/