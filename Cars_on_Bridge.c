/**
Author: Taisia Zhizhina
Date: May 26 2024
This is the cars on an old bridge problem where traffic can only go one way thing
This solution is flawed I believe so take it with a grain of salt
Notes:
Because I was testing with a small amount of cars,
I made the # of cars on bridge at any time small,
and # of cars passed before direction change small as well,
but they can be changed.
Also I use linked list when taking the input of cars from user
because we dont know how many user will input and I
didnt wan't to use extra mem or have to resize and array
though implementing a list makes the code a little bit longer.
*/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LINE_LENGTH 256
#define MAX_NAME_LENGTH 20
#define MAX_CARS_PASSED 3    // number of cars that can do one way before allowing direction to change
#define MAX_CARS_ON_BRIDGE 3 // number of cars that can be on bridge at a time

//-------------------------------structs------------------------------
typedef struct Node
{
    void *data;
    struct Node *next;
} Node;

typedef struct List
{
    Node *head;
    Node *tail;
    int size;
} List;

typedef struct
{
    char driver[MAX_NAME_LENGTH]; // name of car, but I didnt want to use the same variable name a lot
    char direction;               // direction it's traveling in, either N or S
    int arrival;
    int duration;
} Car;
typedef struct
{
    int carsPassed;        // cars that have passed in current direction
    int carsWaitN;         // num of cars waiting to go North
    int carsWaitS;         // num of cars waiting to go South
    int carsOnBridge;      // current number of cars on the bridge
    char trafficDirection; // N or S
} Bridge;
//------------------------------global--------------------------------
Bridge bridge = {
    .carsPassed = 0,
    .carsWaitN = 0,
    .carsWaitS = 0,
    .carsOnBridge = 0,
    .trafficDirection = 'D', // default
};
static pthread_mutex_t bridgeLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t enterBridgeLock = PTHREAD_MUTEX_INITIALIZER;
//----------------------helper functions--------------------------------
Node *addNode(List *list);
void clearList(List *list);
//------------------------thread functions------------------------------
void changeDirection(char dir)
{
    if (bridge.trafficDirection == dir) // in case it was called unnecessairly
        return;
    bridge.trafficDirection = dir; // update direction
    bridge.carsPassed = 0;         // reset cars passed
    // print message
    if (dir == 'N')
    {
        printf("Direction: North\n");
        return;
    }
    printf("Direction: South\n");
}

void *crossBridge(Car *car)
{
    // car gets on bridge
    pthread_mutex_lock(&bridgeLock);
    if ((car->direction) == 'N')
    {
        --bridge.carsWaitN;
    }
    else
    {
        --bridge.carsWaitS;
    }
    ++bridge.carsOnBridge;
    printf("%s\n", car->driver);
    pthread_mutex_unlock(&bridgeLock);

    // car is on bridge
    sleep(car->duration);

    // car gets off bridge
    pthread_mutex_lock(&bridgeLock);
    --bridge.carsOnBridge;
    ++bridge.carsPassed;
    pthread_mutex_unlock(&bridgeLock);
}

void *arrive(void *arg)
{
    Car *car = (Car *)arg;
    sleep(car->arrival);
    // put ar down as waiting in "queue"
    pthread_mutex_lock(&bridgeLock);
    if ((car->direction) == 'N')
        ++bridge.carsWaitN;
    else
        ++bridge.carsWaitS;
    pthread_mutex_unlock(&bridgeLock);

    while (1)
    {                              // basically a busy wait loop
        if (car->direction == 'N') // Exit conditions for cars going north
        {
            if (bridge.trafficDirection != 'N') // traffic is going other way
            {
                // if no more cars need to go other way, or too many have passed that way already
                if (bridge.carsWaitS == 0 || bridge.carsPassed >= MAX_CARS_PASSED)
                {
                    break;
                }
            }
            else
            { // car going same way as traffic
                // if there are no cars waiting for opposite way, or less than 3 cars have passed this way you can go
                if ((bridge.carsWaitS == 0 || bridge.carsPassed < MAX_CARS_PASSED))
                    break;
            }
        }
        else
        {                                       // Exit conditions for cars going south
            if (bridge.trafficDirection != 'S') // traffic is opposite of car direction
            {
                // if no more cars need to go other way, or too many have passed that way already
                if (bridge.carsWaitN == 0 || bridge.carsPassed >= MAX_CARS_PASSED)
                {
                    break;
                }
            }
            else
            {
                // if there are no cars waiting for opposite way, or less than 3 cars have passed this way you can go
                if ((bridge.carsWaitN == 0 || bridge.carsPassed < MAX_CARS_PASSED))
                    break;
            }
        }
    }

    // car has been chosen to enter bridge next
    // change direction if needed
    if (car->direction != bridge.trafficDirection)
    {
        while (bridge.carsOnBridge)
            ;                            // wait for cars going in other direction to get off the bridge
        pthread_mutex_lock(&bridgeLock); // prevent multiple cars changing the diretion at once
        changeDirection(car->direction); // change the direction of bridge
        pthread_mutex_unlock(&bridgeLock);
    }
    else
    {
        while (bridge.carsOnBridge >= MAX_CARS_ON_BRIDGE)
            ; // wait for space on bridge
    }

    // cross
    crossBridge(car);
}
//--------------------------main----------------------------------------
int main(int argc, char *argv[])
{

    char line[MAX_LINE_LENGTH];
    short i = 0;
    List cars = {0};

    while (fgets(line, sizeof line, stdin))
    { // reading line by line from input

        size_t eol = strcspn(line, "\n");
        line[eol] = '\0'; // trim \n at the end of the line

        if (i)
        { // skip header
            // create new car
            Node *curNode = addNode(&cars);
            curNode->data = malloc(sizeof(Car));
            Car *newCar = (Car *)curNode->data;
            char delim[] = "\t ";
            char *ptr = strtok(line, delim); // driver
            strcpy(newCar->driver, ptr);

            ptr = strtok(NULL, delim); // direction
            newCar->direction = ptr[0];

            ptr = strtok(NULL, delim); // arrival
            newCar->arrival = atoi(ptr);

            ptr = strtok(NULL, delim); // duration
            newCar->duration = atoi(ptr);
        }
        else
            ++i;
    }

    // create threads
    int S = cars.size;
    pthread_t carThreads[S];
    Node *cur = cars.head;
    for (int i = 0; i < S; i++)
    {

        if (pthread_create(&carThreads[i], NULL, &arrive, cur->data))
        {
            return 1; // error in creating thread
        }
        cur = cur->next;
    }

    // join threads
    for (int i = 0; i < S; i++)
    {
        if (pthread_join(carThreads[i], NULL))
        {
            return 1; // error in joining thread
        }
    }

    clearList(&cars); // dealocate memory
    return 0;
}
//--------------------------helper methods-----------------------------
// add node to linked list
Node *addNode(List *list)
{
    ++list->size;
    Node *tmp = malloc(sizeof(Node));
    if (tmp != NULL)
    {
        tmp->data = NULL;
        tmp->next = NULL;
    }
    if (list->tail)
    {
        list->tail->next = tmp;
        list->tail = tmp;
    }
    else
    {
        list->head = list->tail = tmp;
    }
    return tmp;
}

// free up memory we used during the program
void clearList(List *list)
{
    if (!list->head)
        return;
    Node *current = list->head;
    Node *next = NULL;
    while (current != NULL)
    {
        next = current->next;
        if (!current->data)
            free(current->data);
        free(current);
        current = next;
    }
}