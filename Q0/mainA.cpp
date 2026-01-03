#include <iostream>

long long CPU_bound()
{
    long long sum = 0;
    for (long long i = 0; i < 1000000000; i++)
    {
        sum += i;
    }
    return sum;
}

long long cache_friendly_arr[10000000];

long long Cache_friendly()
{
    for (long long i = 0; i < 10'000'000; i++)
    {
        cache_friendly_arr[i] = i;
    }

    long long sum = 0;
    for (long long i = 0; i < 10000000; i++)
    {
        sum += cache_friendly_arr[i];
    }
    return sum;
}

long long cache_unfriendly_arr[5000][2000];

long long Cache_unfriendly()
{
    
    for (long long i = 0; i < 5000; i++)
    {
        for (long long j = 0; j < 2000; j++)
        {
            cache_unfriendly_arr[i][j] = i + j;
        }
    }

    long long sum = 0;
    for (long long i = 0; i < 2000; i++)
    {
        for (long long j = 0; j < 5000; j++)
        {
            sum += cache_unfriendly_arr[j][i];
        }
    }
    return sum;
}

int main()
{
    long long sum1 = CPU_bound();
    long long sum2 = Cache_friendly();
    long long sum3 = Cache_unfriendly();

    std::cout << "CPU bound sum: " << sum1 << std::endl;
    std::cout << "Cache friendly sum: " << sum2 << std::endl;
    std::cout << "Cache unfriendly sum: " << sum3 << std::endl;
    return 0;
}