#include <iostream>

int main()
{
    long long sum{0};

    //task 1: cpu-bound loop
    for (int i{0}; i <= 1'000'000; i++) 
    {
        sum += i;
    }
    
    return 0;
}