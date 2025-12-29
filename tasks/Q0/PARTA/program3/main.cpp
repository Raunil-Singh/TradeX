#include <iostream>

int main()
{
    long long sum{0};

    //task 3: cache-unfriendly access
    static int arr2[5000][2000];
    sum = 0;
    for (int c = 0; c < 2000; c++) {
        for (int r = 0; r < 5000; r++) {
            sum += arr2[r][c];
        }
    }

    return 0;
}