/*
Student ID: 2003900
Student Name: Dafydd-Rhys Maund
Date: 17/10/2024

I slightly changed some of the methods for better readability - includes slight code changes and removed comments.

I was unsure about number of walkers and sleep times so kept them as the code was given, i also implemented some
code for debugging which I left commented for help if needed.

I was also unsure about whether the locks sleep function should remain in the codebase, without the sleep it takes my computer
20 - 25 seconds to complete using the default variables, but with the sleep part of the lock function it takes significantly longer 60+.
*/

#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <vector>

#define N 20
#define S 10
#define MAX_WALKERS_PER_LOCATION 3
#define MAX_WALKERS_PER_EDGE 4
#define VERTEX_SLEEP_TIME 1
#define EDGE_SLEEP_TIME 1

typedef struct _Walker
{
    int currentX, currentY, finalX, finalY;
    bool hasArrived;

    void Init()
    {
        currentX = rand() % S;
        currentY = rand() % S;
        finalX = rand() % S;
        finalY = rand() % S;
        hasArrived = false;
    }

    bool RandomWalk(int& newX, int& newY, int& edgeIndex)
    {
        int direction = rand() % 2; // 0: horizontal, 1: vertical
        int sign = (rand() % 2) ? +1 : -1;

        if (direction == 1) {
            newX = currentX;
            newY = currentY + sign;
            edgeIndex = (S * (S - 1)) + newX * (S - 1) + newY + (sign > 0 ? -1 : 0);
        }
        else {
            newX = currentX + sign;
            newY = currentY;
            edgeIndex = newY * (S - 1) + newX + (sign > 0 ? -1 : 0);
        }

        //converted to inline - (true = inbounds)
        return newX >= 0 && newX < S && newY >= 0 && newY < S;
    }
} Walker;

int originalGridCount[S][S];
int finalGridCount[S][S];
int obtainedGridCount[S][S];
int locationCount[S][S];
Walker walkers[N];

std::vector<std::mutex> edgeMutexes(S * (S - 1) * 2);
std::vector<int> edgeWalkerCount(S* (S - 1) * 2, 0);
std::mutex locationMutexes[S][S];

void Lock(std::mutex* m, int t = VERTEX_SLEEP_TIME)
{
    //std::cout << "Thread " << std::this_thread::get_id() << " attempting to lock mutex\n";
    m->lock();
    //std::cout << "Thread " << std::this_thread::get_id()<< " acquired mutex\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

void Unlock(std::mutex* m)
{
    //std::cout << "Thread " << std::this_thread::get_id() << " releasing mutex\n";
    m->unlock();
}

void CrossTheStreet()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(EDGE_SLEEP_TIME));
}

void WaitAtLocation()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(VERTEX_SLEEP_TIME));
}

void PrintGrid(std::string message, int grid[S][S])
{
    std::cout << message;
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < S; j++)
            std::cout << (grid[i][j] < 10 ? "  " : " ") << grid[i][j];
        std::cout << "\n";
    }
}

void SetObtainedGrid()
{
    for (int i = 0; i < N; i++) {
        obtainedGridCount[walkers[i].currentY][walkers[i].currentX]++;
        if (!walkers[i].hasArrived) {
            std::cout << "\nAt least one walker has not arrived!\n";
            return;
        }
    }
}

void CompareGrids(int a[S][S], int b[S][S])
{
    for (int i = 0; i < S; i++)
        for (int j = 0; j < S; j++)
            if (a[i][j] != b[i][j]) {
                std::cout << "\nError: results are different!\n";
                return;
            }
    std::cout << "\nSeems to be OK!\n";
}

void adjustEdge(int edgeIndex, int num)
{
    Lock(&edgeMutexes[edgeIndex]);
    edgeWalkerCount[edgeIndex] += num;
    Unlock(&edgeMutexes[edgeIndex]);
}

void WalkerI(int id)
{
    int newX, newY, edgeIndex;

    while (!walkers[id].hasArrived) {
        if (!walkers[id].RandomWalk(newX, newY, edgeIndex)) {
            continue; //out of bounds, skip
        }

        //check if the edge has reached the max allowed walkers
        if (edgeWalkerCount[edgeIndex] >= MAX_WALKERS_PER_EDGE) {
            continue; //too many on street, skip
        }

        adjustEdge(edgeIndex, 1);

        CrossTheStreet(); //cross street
  
        //check if the location has reached the max allowed walkers
        if (locationCount[newY][newX] >= MAX_WALKERS_PER_LOCATION) {
            adjustEdge(edgeIndex, -1);
            continue; //too many at location, skip
        }

        int oldX = walkers[id].currentX;
        int oldY = walkers[id].currentY;

        //lock location and update counts - tie used for consistent locking order 
        if (std::tie(oldY, oldX) < std::tie(newY, newX)) {
            Lock(&locationMutexes[oldY][oldX]);
            Lock(&locationMutexes[newY][newX]);
        }
        else {
            Lock(&locationMutexes[newY][newX]);
            Lock(&locationMutexes[oldY][oldX]);
        }

        locationCount[oldY][oldX]--;
        locationCount[newY][newX]++;

        walkers[id].currentX = newX;
        walkers[id].currentY = newY;

        Unlock(&locationMutexes[newY][newX]);
        Unlock(&locationMutexes[oldY][oldX]);

        adjustEdge(edgeIndex, -1); //walker moved off street

        //check if the walker has arrived
        if (walkers[id].currentX == walkers[id].finalX && walkers[id].currentY == walkers[id].finalY) {
            walkers[id].hasArrived = true;
        }

        WaitAtLocation(); //wait at location if not arrived
    }
}

void InitGame()
{
    for (int i = 0; i < S; i++)
        for (int j = 0; j < S; j++)
            originalGridCount[i][j] = finalGridCount[i][j] = obtainedGridCount[i][j] = 0;

    for (int i = 0; i < N; i++) {
        do walkers[i].Init();
            while (originalGridCount[walkers[i].currentY][walkers[i].currentX] >= MAX_WALKERS_PER_LOCATION);
        originalGridCount[walkers[i].currentY][walkers[i].currentX]++;
        locationCount[walkers[i].currentY][walkers[i].currentX]++; //sets location counts to initial grid

        finalGridCount[walkers[i].finalY][walkers[i].finalX]++;

        //std::cout << "Walker " << i << " at (" << walkers[i].currentX << ", "  << walkers[i].currentY <<
            //") with destination (" << walkers[i].finalX << ", " << walkers[i].finalY << ")\n";
    }
}

int main()
{
    InitGame();
    //auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> walkerThreads;
    for (int i = 0; i < N; i++) {
        walkerThreads.push_back(std::thread(WalkerI, i));
    }

    for (auto& thread : walkerThreads) {
        thread.join();
    }

    PrintGrid("Original locations:\n\n", originalGridCount);
    PrintGrid("\n\nIntended Result:\n\n", finalGridCount);
    SetObtainedGrid();
    PrintGrid("\n\nObtained Result:\n\n", obtainedGridCount);
    CompareGrids(finalGridCount, obtainedGridCount);

    /*
    * code to test runtime - check for consistency of concurrency
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "took " << duration.count() << " seconds\n"; 
    */
}
