#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <future>
#include <climits>

void merge(int arr[], int l, int m, int r)
{

    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    std::vector<int> L(n1);
    std::vector<int> R(n2);

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int arr[], int l, int r)
{

    if (l < r)
    {
        int m = l + (r - l) / 2;

        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}

void mergeSort_parallel(int arr[], int l, int r, int depth, int max_depth)
{

    if (l < r)
    {
        int m = l + (r - l) / 2;

        if (depth > max_depth)
        {
            mergeSort(arr, l, m);
            mergeSort(arr, m + 1, r);
        }
        else
        {

            std::thread left_thread(mergeSort_parallel, arr, l, m, depth + 1, max_depth);
            std::thread right_thread(mergeSort_parallel, arr, m + 1, r, depth + 1, max_depth);

            left_thread.join();
            right_thread.join();
        }
        merge(arr, l, m, r);
    }
}

void mergeSort_workstealing(int arr[], int l, int r, int depth, int max_depth)
{
    if (l >= r)
        return;

    int m = l + (r - l) / 2;

    if (depth > max_depth)
    {
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
    }
    else
    {
        auto left_thread = std::async(std::launch::async, mergeSort_workstealing, arr, l, m, depth + 1, max_depth);
        auto right_thread = std::async(std::launch::async, mergeSort_workstealing, arr, m + 1, r, depth + 1, max_depth);

        left_thread.get();
        right_thread.get();
    }

    merge(arr, l, m, r);
}

int main()
{
    std::vector<int> arr(1000000);
    std::uniform_int_distribution<size_t> dist(0, 200000);
    thread_local std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < 1000000; i++)
    {
        arr[i] = dist(rng);
    }

    std::vector<int> arr1 = arr;
    std::vector<int> arr2 = arr;
    std::vector<int> arr3 = arr;

    long long avg_time1 = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start1 = std::chrono::high_resolution_clock::now();
        mergeSort(arr1.data(), 0, arr1.size() - 1);
        auto end1 = std::chrono::high_resolution_clock::now();
        auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
        avg_time1 += duration1.count();
    }
    long long duration1 = avg_time1 / 3;
    std::cout << "Sorting completed in " << duration1 << " ms\n";

    long long avg_time2 = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start2 = std::chrono::high_resolution_clock::now();
        std::thread merge_sort_parallel(mergeSort_parallel, arr2.data(), 0, arr2.size() - 1, 0, 3);
        merge_sort_parallel.join();
        auto end2 = std::chrono::high_resolution_clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
        avg_time2 += duration2.count();
    }
    long long duration2 = avg_time2 / 3;
    std::cout << "Parallel Sorting completed in " << duration2 << " ms " << "| Speedup : " << duration1 / duration2 << "x\n";

    long long avg_time3 = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start3 = std::chrono::high_resolution_clock::now();
        std::thread merge_sort_workstealing(mergeSort_workstealing, arr3.data(), 0, arr3.size() - 1, 0, 3);
        merge_sort_workstealing.join();
        auto end3 = std::chrono::high_resolution_clock::now();
        auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3);
        avg_time3 += duration3.count();
    }
    long long duration3 = avg_time3 / 3;
    std::cout << "Work-stealing Sorting completed in " << duration3 << " ms | Speedup : " << duration1 / duration3 << "x\n";

    long long mini = LLONG_MAX;
    int optimal_threshold = 0;
    for (int i = 1; i < 10; i++)
    {
        std::vector<int> arr_opt = arr;
        auto start_opt = std::chrono::high_resolution_clock::now();
        std::thread merge_sort_opt(mergeSort_parallel, arr_opt.data(), 0, arr_opt.size() - 1, 0, i);
        merge_sort_opt.join();
        auto end_opt = std::chrono::high_resolution_clock::now();
        auto duration_opt = std::chrono::duration_cast<std::chrono::milliseconds>(end_opt - start_opt);
        if (mini > duration_opt.count())
        {
            mini = duration_opt.count();
            optimal_threshold = i;
        }
    }
    std::cout << "Optimal threshold for parallel merge sort is: " << optimal_threshold << "\n";
}