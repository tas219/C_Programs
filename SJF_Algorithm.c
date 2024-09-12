/**
Author: Taisia Zhizhina
Date: May 10 2024
This is a shortest job firt algorithm that takes the input in the following format:
    User Process Arrival Duration
    Jim D 2 5
    Mary B 2 3
    Sue A 5 5
    Mary C 6 4
and outputs two tables:
First table is the running time and the job currently executing.
The second table is a summary with the user name (in the order in
which jobs arrive) and the time when their last job is completed
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#define LINE_MAX_LENGTH 256
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

typedef struct Process
{
    char user[20];
    char id;
    int arrival;
    int duration;
    char done;
} Process;

typedef struct UserData
{
    char name[20];
    int time;
} UD;
//-------------------------------funcs--------------------------------
// List related functions:
Node *addNode(List *list);
void clearList(List *list);
// UserData related funtions:
UD *findUser(List *users, const char *userName);
// SJF algorithm related functions:
int fillPool(Process **pool, int time, List *list);
int findShortestJob(Process **pool, int poolSize);
void schedule(List *jobs, List *users);

int main(int argc, char *argv[])
{

    char line[LINE_MAX_LENGTH]; // line we will read into from stdin
    int i = 0;                  // basically boolean to see if we passed the header

    List jobs = {0};  // list of processes
    List users = {0}; // list of users

    while (fgets(line, sizeof line, stdin))
    { // reading line by line from input

        size_t eol = strcspn(line, "\n");
        line[eol] = '\0'; // trim \n at the end of the line

        if (i != 0)
        { // skip first line (header)
            // create new process node
            Node *curNode = addNode(&jobs);
            curNode->data = malloc(sizeof(Process));
            Process *newP = (Process *)curNode->data;

            // fill Process in based on info provided in the line
            char delim[] = "\t ";
            char *ptr = strtok(line, delim); // ptr points to user name
            strcpy(newP->user, ptr);

            // sidetrack to fill in user array while we have the user name ptr
            if (!findUser(&users, ptr))
            {
                // basically if this users name isnt in the list add it to the list
                Node *newUser = addNode(&users);
                newUser->data = malloc(sizeof(UD));
                strcpy(((UD *)(newUser->data))->name, ptr);
            }

            ptr = strtok(NULL, delim); // ptr points to process (what I called id)
            newP->id = ptr[0];

            ptr = strtok(NULL, delim); // ptr points to arrival
            newP->arrival = atoi(ptr);

            ptr = strtok(NULL, delim); // ptr points to duration
            newP->duration = atoi(ptr);

            newP->done = 0; // this process has not been done yet
        }
        else
            ++i;
    }

    // now that we actually have the lists from the input we go onto sjf scheduling algorithm
    schedule(&jobs, &users); // it also prints the output for the question

    // free memory we used (at least in the lists)
    clearList(&users);
    clearList(&jobs);

    // its finally done :D
    return 0;
}

//--------------------------List related functions-------------------------

Node *addNode(List *list)
{
    ++list->size;                     // update size
    Node *tmp = malloc(sizeof(Node)); // create node in mem

    if (tmp != NULL)
    {
        tmp->data = NULL;
        tmp->next = NULL;
    }

    if (list->tail)
    {
        // list wasn't empty, add to the end
        list->tail->next = tmp;
        list->tail = tmp;
    }
    else
    {
        // if list was empty start with head
        list->head = list->tail = tmp;
    }

    return tmp;
}

void clearList(List *list)
{
    // deallocates any mem that isn't already empty
    if (!list->head)
        return;
    Node *curr = list->head;
    Node *next = NULL;
    while (curr != NULL)
    {
        next = curr->next;
        if (!curr->data)
            free(curr->data);
        free(curr);
        curr = next;
    }
}
//--------------------------UserData related funtions-------------------------
UD *findUser(List *users, const char *userName)
{
    if (!userName) // make sure name isn't null
        return NULL;
    // loop through the list and compare name
    Node *curr = users->head;
    while (curr != NULL)
    {
        UD *userData = (UD *)curr->data;
        assert(userData);
        if (!strcmp(userData->name, userName))
            return userData; // return pointer to element if name matches
        curr = curr->next;
    }
    return NULL; // name not found return null
}
//--------------------SJF algorithm related functions-----------------------
/**Basically, this function creates a pool (or set) of processes
that havent executed yet (checked through "done"), and that
have arrival time less than or equal to time (which is some
time when the cpu is empty and looking for next process)
*/
int fillPool(Process **pool, int time, List *list)
{

    Node *curr = list->head;
    int sizeOfPool = 0; // keep track of size
    while (curr != NULL)
    { // loop over jobs list
        Process *currProcess = (Process *)curr->data;
        if (!currProcess->done && currProcess->arrival <= time)
        {
            *pool = currProcess; // add all jobs that meet conditions
            ++pool;              // ptr arithmetic
            ++sizeOfPool;
        }
        curr = curr->next;
    }
    return sizeOfPool; // return num of jobs in pool
}

// finds smallest duration amond jobs in the pool
int findShortestJob(Process **pool, int poolSize)
{
    // basic find min algorithm
    int minTime = INT_MAX;
    int index = -1; // returns -1 if pool was empty (and therefore no min)
    for (int i = 0; i < poolSize; ++i)
    {
        if (pool[i]->duration < minTime)
        {
            minTime = pool[i]->duration;
            index = i;
        }
    }
    return index; // return index of job with shortest duration
}

void schedule(List *jobs, List *users)
{
    if (!jobs->head) // if we have no jobs then don't bother
        return;
    // create space where the pool will be, size is the num of proesses since thats the biggest it can be
    void *poolMemory = malloc(jobs->size * sizeof(void *));
    // get start time by finding the arrival time of the first process
    Process *job1 = (Process *)jobs->head->data;
    int time = job1->arrival;

    printf("Time\tJob\n"); // start printing out first table

    while (1)
    {
        Process **pool = poolMemory;                 // pool is a pointer to the first element in a Process* "array"
        int poolSize = fillPool(pool, time, jobs);   // fill pool
        int index = findShortestJob(pool, poolSize); // find min
        if (index == -1)                             // if pool was empty ie. no min ie. no next job: break
            break;
        // check that the index is in the pool
        assert(index < poolSize);
        pool[index]->done = 1;                     // mark process as done
        printf("%d\t%c\n", time, pool[index]->id); // print job to first table

        time += pool[index]->duration; // update time to after the process is done

        UD *userData = findUser(users, pool[index]->user); // find which users process just executed
        if (userData)
            userData->time = time; // update the "map" with the end time of the process
    }
    printf("%d\tIDLE\n\n", time); // finish first table

    // print second table based on (user, time their last job is completed) map
    printf("Summary\n");
    Node *curr = users->head;
    while (curr != NULL)
    {
        UD *userData = (UD *)curr->data;
        printf("%s\t%d\n", userData->name, userData->time);
        curr = curr->next;
    }
    // wow this question sure was awesome :D
}