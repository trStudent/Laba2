#include <iostream>
#include <tchar.h>
#include <core/General/Thread.h>

using namespace core;

volatile int min_el, max_el, av;
int n;
int* arr;

DWORD WINAPI min_max(LPVOID lpParameters)
{
    int i = 0;
    while(i < n - 2)
    {
        Sleep(7);
        int a = arr[i];
        int b = arr[i+1];
        if(a < b) {
            if(arr[min_el] > a)
                min_el = i;
            if(arr[max_el] < b)
                max_el = i+1;
                Sleep(21);
        } else {
            if(arr[min_el] > b)
                min_el = i+1;
            if(arr[max_el] < a)
                max_el = i;
            Sleep(21);
        }
        i+=2;
    }
    if(i == n-1)
    {
        if(arr[min_el] > arr[i])
                min_el = i;
        else if(arr[max_el] < arr[i])
            max_el = i;
    } else 
    {
        int a = arr[i];
        int b = arr[i+1];
        if(arr[min_el] > a)
            min_el = i;
        if(arr[max_el] < b)
            max_el = i+1;
    }
    Sleep(14);

    
    return 0;
}

DWORD WINAPI average(LPVOID lpParameters)
{
    int sum = 0;
    for(int i = 0; i < n; i++) {
        Sleep(12);
        sum+=arr[i];
    }
    av = sum/n;

    return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_ALL, "Russian");

    std::wcout << L"Введите размер массива: " << std::endl;
    std::wcin >> n;
    while(n < 1)
    {
        std::wcout << L"Для размера массива используйте число больше 0. Введите ещё раз: " << std::endl;
        std::wcin >> n;
    }

    std::wcout << L"Введите элементы массива: " << std::endl;
    arr = new int[n];
    for(int i = 0; i < n; i++)
    {
        std::wcout << i << ": ";
        std::wcin >> arr[i];
    }
    std::wcout << std::endl;

    General::Thread th_minmax = General::Thread::create(NULL, 0, min_max, NULL, 0, NULL);
    General::Thread th_average = General::Thread::create(NULL, 0, average, NULL, 0, NULL);

    th_minmax.join();
    th_average.join();
    std::wcout << L"Минимальный элемент массива: " << arr[min_el] << std::endl;
    std::wcout << L"Максимальный элемент массива: " << arr[max_el] << std::endl;
    std::wcout << L"Среднее значение элементов массива: " << av << std::endl;
    
    std::wcout << L"Массив до замены минимального и максимального элемента на среднее значение: " << std::endl;
    for(int i = 0; i < n; i++)
        std::wcout << arr[i] << ' ';
    std::wcout << std::endl;

    std::wcout << L"Массив после замены минимального и максимального элемента на среднее значение: " << std::endl;
    arr[min_el] = arr[max_el] = av;
    for(int i = 0; i < n; i++)
        std::wcout << arr[i] << ' ';
    std::wcout << std::endl;

    delete[] arr;
    return 0;
}   