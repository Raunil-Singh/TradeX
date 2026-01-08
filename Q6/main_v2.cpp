#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <future>

constexpr int VECTOR_SIZE{50'000'000};

std::mutex vectorMutex;
std::mt19937 rGen(std::random_device{}());
std::uniform_int_distribution<> uInt(1, 100);
void merge(std::vector<int>&, int, int, int);
void mergeSort(int level, std::vector<int>& vector, int startIndex, int endIndex, int depth){
    int elementCount(endIndex - startIndex + 1);
    if(depth >= level){
        std::sort(vector.begin() + startIndex, vector.begin() + endIndex + 1);
    }
    else{
        depth++;
        auto futL = std::async(std::launch::async, mergeSort, level, std::ref(vector), startIndex, startIndex + elementCount/2 - 1, depth);
        auto futR = std::async(std::launch::async, mergeSort, level, std::ref(vector), startIndex + elementCount/2, endIndex, depth);
        futL.get();
        futR.get();
        merge(vector, startIndex, startIndex + elementCount/2 - 1, endIndex);
    }
}

int main(int argc, char* argv[]){
    int level{0};
    if(argc != 2){
        std::cout<<"Usage ./main.cpp [level]"<<std::endl;
        return EXIT_FAILURE;
    }
    else{
        level = std::atoi(argv[1]);
    }
    std::vector<int> vector(VECTOR_SIZE);
    for(int i = 0; i < VECTOR_SIZE; i++){
        vector[i] = uInt(rGen);
    }
    auto timeStart = std::chrono::steady_clock::now();
    auto fut = std::async(std::launch::async, mergeSort, level, std::ref(vector), 0, VECTOR_SIZE - 1, 0);
    fut.get();
    auto timeEnd = std::chrono::steady_clock::now();
    std::cout<<"Time elapsed in ms: "<<std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()<<". \n";
    return 0;
}

void merge(std::vector<int>& vector, int startIndex, int middle, int endIndex){
    int eleCount = endIndex - startIndex + 1;
    std::vector<int> vectorL;
    vectorL.reserve(eleCount);
    int pointerLeft = startIndex;
    int pointerRight = middle + 1;
    while(pointerLeft <= middle && pointerRight <= endIndex){
        if(vector[pointerLeft] < vector[pointerRight]){
            vectorL.push_back(vector[pointerLeft]);
            pointerLeft++;
        }
        else{
            vectorL.push_back(vector[pointerRight]);
            pointerRight++;
        }
    }
    while(pointerLeft <= middle){
        vectorL.push_back(vector[pointerLeft]);
        pointerLeft++;
    }
    while(pointerRight <= endIndex){
        vectorL.push_back(vector[pointerRight]);
        pointerRight++;
    }
    for(int i = 0; i < eleCount; i++){
        vector[startIndex + i] = vectorL[i];
    }
}