#include <iostream>

void fast_function();
void slow_function();
void very_slow_function();

int main()
{
    fast_function();
    slow_function();
    very_slow_function();
}

void fast_function()
{
    long long sum = 0;
    for (int i = 1; i < 100'000'000; i++)
    {
        sum += i;
    }
    std::cout << sum << '\n';
}

void slow_function()
{
    long long sum = 10;
    for (int i = 1; i < 1'000'000; i++)
    {
        sum += i * (sum / i + 1);
    }
    std::cout << sum << '\n';
}

void very_slow_function()
{
    long long sum = 0;
    for (int i = 1; i < 1000000000; i++)
    {
        for (int j = 1; j < 100000000; j++)
        {
            sum += i * j;
        }
    }
    std::cout << sum << '\n';
}
