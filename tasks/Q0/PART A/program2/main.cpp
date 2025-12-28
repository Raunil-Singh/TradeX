#include <iostream>

int main()
{
    long long sum{0};

    //task 2: cache-friendly access
    static int arr1[10'000'000];
    sum = 0;
    for (int i = 0; i < 10'000'000; i++)
    {
        sum += arr1[i];  
    }

    return 0;
}