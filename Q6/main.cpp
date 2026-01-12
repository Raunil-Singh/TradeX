#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <condition_variable>
#include <functional>
#include <queue>
#include <future>
#include <cstdlib>
#include <shared_mutex>
#include <algorithm>
using namespace std;


void merge_sequential(vector<int> &arr, int left, int right) {
    if(left>=right) return;
    int mid = left+(right-left)/2;
    merge_sequential(arr, left, mid);
    merge_sequential(arr, mid+1, right);
    inplace_merge(arr.begin()+left, arr.begin()+mid+1, arr.begin()+right+1);
}

void merge_fixed(vector<int> &arr, int left, int right, int depth, int max_depth) {
    if(depth>=max_depth) {
        merge_sequential(arr, left, right);
        return;
    }
    int mid = left + (right-left)/2;
    thread t([&]() {
        merge_fixed(arr, left, mid, depth+1, max_depth);
    });
    merge_fixed(arr, mid+1, right, depth+1, max_depth);
    t.join();
    inplace_merge(arr.begin()+left, arr.begin()+mid+1, arr.begin()+right+1);
}

void merge_async(vector<int> &arr, int left, int right) {
    if(left>=right) return;
    if(right-left<5000) {
        merge_sequential(arr, left, right);
        return;
    }
    int mid = left+(right-left)/2;
    future<void> bg_thread = async(launch::async, [&]() {
        merge_async(arr, left, mid);
    });
    merge_async(arr, mid+1, right);
    bg_thread.get();
    inplace_merge(arr.begin()+left, arr.begin()+mid+1, arr.begin()+right+1);
}

vector<int> gen_arr(int n) {
    vector<int> arr(n);
    for(int i=0; i<n; i++) {
        arr[i] = rand()%n;
    }
    return arr;
}

int main() {
    int arr_size = 10000000;
    vector<int> data = gen_arr(arr_size);
    cout << "Sorting " << arr_size << " elements..." << endl;

    // Test 1 : Sequential 
    vector<int> t1 = data;
    auto start = chrono::high_resolution_clock::now();
    merge_sequential(t1, 0, arr_size-1);
    auto end = chrono::high_resolution_clock::now();
    cout << "Sequential Sort time : " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    // Test 2 : Fixed Parallelism
    vector<int> depths = {1,2,3};
    vector<int> threads = {2,4,8};
    for(int i=0; i<3; i++) {
        int d = depths[i];
        int t = threads[i];
        vector<int> t2 = data;
        start = chrono::high_resolution_clock::now();
        merge_fixed(t2, 0, arr_size-1, 0, d);
        end = chrono::high_resolution_clock::now();
        cout << "Fixed Parallelism : " << " Threads = " << t << ", Max depth = " << d << ", Time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    }

    // Test 3 : Work Stealing
    vector<int> t3 = data;
    start = chrono::high_resolution_clock::now();
    merge_async(t3, 0, arr_size-1);
    end = chrono::high_resolution_clock::now();
    cout << "Work Stealing(Async) : " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    return 0;
}