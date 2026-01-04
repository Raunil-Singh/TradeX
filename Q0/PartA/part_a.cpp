#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
using namespace std;


void cpu_bound() {
    cout << "Starting CPU-Bound task..." << endl;
    long long sum = 0;
    for(long long i=1; i<=1000000000; i++) {
        sum += i;
    }
    cout << "Calculated sum = " << sum << endl;
}

void cache_friendly() {
    const int N = 10000000;
    vector<int> arr(N,1);
    cout << "Starting Cache-friendly task..." << endl;
    long long sum = 0;
    for(int i=0; i<N; i++) {
        sum += arr[i];
    }
    cout << "Calculated sum = " << sum << endl;
}

void cache_unfriendly() {
    const int ROWS = 5000;
    const int COLS = 2000;
    vector<vector<int>> arr(ROWS, vector<int>(COLS,1));
    cout << "Starting Cache-unfriendly task..." << endl;
    long long sum = 0;
    for(int j=0; j<COLS; j++) {
        for(int i=0; i<ROWS; i++) {
            sum += arr[i][j];
        }
    }
    cout << "Calculated sum = " << sum << endl;
}

int main(int argc, char* argv[]) {
    if(argc<2) {
        cout << "Error : Enter a valid mode(1, 2, 3 or 4)" << endl;
        return 1;
    }
    int mode = stoi(argv[1]);

    if(mode==1) cpu_bound();
    else if(mode==2) cache_friendly();
    else if(mode==3) cache_unfriendly();
    else if(mode==4) {
        cpu_bound();
        cache_friendly();
        cache_unfriendly();
    }

    return 0;
}