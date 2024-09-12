/**
Author: Taisia Zhizhina
Date: May 15 2024
This code uses mutexes to synchronize pthreads
*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
// bounds for rand
#define PADDLE_LOWER 1
#define PADDLE_UPPER 2
#define CANOE_LOWER 5
#define CANOE_UPPER 7
#define PKG_LOWER 4
#define PKG_UPPER 6
// global vars
static int T;
static int totalPaddle = 0;
static int totalCanoe = 0;
static int paddleStock = 0;
static int canoeStock = 0;
static int totalPkgs = 0;
// mutexes
static pthread_mutex_t paddleLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t canoeLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pkgLock = PTHREAD_MUTEX_INITIALIZER;

void *createPaddle(void *arg)
{
    time_t initTime;
    time(&initTime);
    while (difftime(time(NULL), initTime) < T)
    {
        pthread_mutex_lock(&paddleLock);
        ++totalPaddle;
        ++paddleStock;
        pthread_mutex_unlock(&paddleLock);
        printf("We have a paddle.\n");
        int time = rand() % (PADDLE_UPPER - PADDLE_LOWER + 1) + PADDLE_LOWER;
        sleep(time);
    }
}

void *createCanoe(void *arg)
{
    time_t initTime;
    time(&initTime);
    while (difftime(time(NULL), initTime) < T)
    {
        pthread_mutex_lock(&canoeLock);
        ++totalCanoe;
        ++canoeStock;
        pthread_mutex_unlock(&canoeLock);
        printf("We have a canoe.\n");
        int time = rand() % (CANOE_UPPER - CANOE_LOWER + 1) + CANOE_LOWER;
        sleep(time);
    }
}

void *createShippment(void *arg)
{
    time_t initTime;
    time(&initTime);
    while (difftime(time(NULL), initTime) < T)
    {
        if (canoeStock < 1 || paddleStock < 2)
            continue;

        pthread_mutex_lock(&pkgLock);

        pthread_mutex_lock(&paddleLock);
        paddleStock -= 2;
        pthread_mutex_unlock(&paddleLock);

        pthread_mutex_lock(&canoeLock);
        --canoeStock;
        pthread_mutex_unlock(&canoeLock);

        ++totalPkgs;
        pthread_mutex_unlock(&pkgLock);

        printf("We now have a shipment!\n");

        int time = rand() % (PKG_UPPER - PKG_LOWER + 1) + PKG_LOWER;
        sleep(time);
    }
}

void *inventoryManager(void *arg)
{
    time_t initTime;
    time(&initTime);
    while (difftime(time(NULL), initTime) < T)
    {
        getchar();
        printf("%d Paddles and %d Canoes made - %d packages shipped!\n", totalPaddle, totalCanoe, totalPkgs);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("wrong number of args\n");
        return 1;
    }
    int P, C, S;
    P = atoi(argv[1]);
    C = atoi(argv[2]);
    S = atoi(argv[3]);
    T = atoi(argv[4]);
    pthread_t paddleThreads[P];
    pthread_t canoeThreads[C];
    pthread_t shipperThreads[S];
    pthread_t invManThread;
    // create all threads
    for (int i = 0; i < P; i++)
    {
        if (pthread_create(&paddleThreads[i], NULL, &createPaddle, NULL))
        {
            return 1; // error in creating thread
        }
    }
    for (int i = 0; i < C; i++)
    {
        if (pthread_create(&canoeThreads[i], NULL, &createCanoe, NULL))
        {
            return 1; // error in creating thread
        }
    }
    for (int i = 0; i < S; i++)
    {
        if (pthread_create(&shipperThreads[i], NULL, &createShippment, NULL))
        {
            return 1; // error in creating thread
        }
    }
    if (pthread_create(&invManThread, NULL, &inventoryManager, NULL))
    {
        return 1; // error in creating thread
    }
    // join all threads
    for (int i = 0; i < P; i++)
    {
        if (pthread_join(paddleThreads[i], NULL))
        {
            return 1; // error in joining thread
        }
    }
    for (int i = 0; i < C; i++)
    {
        if (pthread_join(canoeThreads[i], NULL))
        {
            return 1; // error in joining thread
        }
    }
    for (int i = 0; i < S; i++)
    {
        if (pthread_join(shipperThreads[i], NULL))
        {
            return 1; // error in joining thread
        }
    }
    if (pthread_join(invManThread, NULL))
    {
        return 1; // error in joining thread
    }
}