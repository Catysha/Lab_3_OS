#include <iostream>
#include <vector>
#include <windows.h>
#include <process.h>

using namespace std;

struct markerArguments {
    int* array;
    int& markerToStop;
    int size;
    int markerNumber;

    CRITICAL_SECTION& criticalSection;
    HANDLE startEvent;

    markerArguments(int* array, int size, int number, const CRITICAL_SECTION& criticalSection, const void* startEvent,
        int& markerToStop) : array(array), size(size), markerNumber(number),
        criticalSection(const_cast<CRITICAL_SECTION&>(criticalSection)), startEvent(const_cast<HANDLE>(startEvent)),
        markerToStop(markerToStop) {}
};

unsigned int WINAPI marker(void* arguments) {
    markerArguments* params = (markerArguments*)arguments;
    srand(params->markerNumber);
    WaitForSingleObject(params->startEvent, INFINITE);

    printf("Marker %i started\n", params->markerNumber);

    bool stop = false;

    int rNumber;
    int* changedNumbers = new int[params->size];
    int changedCount = 0;

    while (!stop) {
        EnterCriticalSection(&params->criticalSection);

        rNumber = rand() % params->size;
        if (params->array[rNumber] == 0) {
            Sleep(5);
            params->array[rNumber] = params->markerNumber;
            changedNumbers[changedCount] = rNumber;
            ++changedCount;
            LeaveCriticalSection(&params->criticalSection);
            Sleep(5);
        }
        else {
            printf("Marker %i stopped\n", params->markerNumber);
            LeaveCriticalSection(&params->criticalSection);

            WaitForSingleObject(params->startEvent, INFINITE);
            if (params->markerNumber == params->markerToStop) {
                while (changedCount != 0) {
                    --changedCount;
                    params->array[changedNumbers[changedCount]] = 0;
                }
                stop = true;
                printf("Marker %i ended its work\n", params->markerNumber);
            }
            else {
                WaitForSingleObject(params->startEvent, INFINITE);
            }
        }
    }

    delete[] changedNumbers;
    return 0;
}

void print(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        cout << array[i] << " ";
    }
    cout << endl;
}

int main() {
    int size;
    int* array;
    int markerCount;

    CRITICAL_SECTION criticalSection;
    InitializeCriticalSection(&criticalSection);

    cout << "Input array size: " << endl;
    cin >> size;

    array = new int[size];
    fill(array, array + size, 0);

    cout << "Input markers count: " << endl;
    cin >> markerCount;

    HANDLE startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    vector<HANDLE> threads;
    vector<markerArguments*> args;

    int markerToStop;

    for (int i = 0; i < markerCount; ++i) {
        auto* temp = new markerArguments(array, size, i + 1, criticalSection, startEvent, markerToStop);
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, marker, temp, 0, NULL);
        threads.push_back(thread);
        args.push_back(temp);
    }

    SetEvent(startEvent);

    while (true) {
        print(array, size);
        cout << "Enter the marker thread number to stop: ";
        cin >> markerToStop;

        if (markerToStop <= 0 || markerToStop > markerCount) {
            cout << "Invalid marker number. Try again." << endl;
            continue;
        }

        WaitForSingleObject(threads[markerToStop - 1], INFINITE);

        CloseHandle(threads[markerToStop - 1]);
        delete args[markerToStop - 1];

        threads[markerToStop - 1] = NULL;
        args[markerToStop - 1] = NULL;

        bool allStopped = true;
        for (auto& thread : threads) {
            if (thread != NULL) {
                allStopped = false;
                break;
            }
        }

        if (allStopped) break;
    }

    for (auto& thread : threads) {
        if (thread != NULL) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }

    for (auto* arg : args) {
        delete arg;
    }

    delete[] array;
    DeleteCriticalSection(&criticalSection);
    CloseHandle(startEvent);

    cout << "All marker threads have finished. Exiting." << endl;
    return 0;
}
