/**
Author: Taisia Zhizhina
Student number: 219504513
Date: June 15 2024

This code implements the following disk scheduling algorithms
FCFS (F), SSTF (T), C-SCAN (C), LOOK (L) on a disk with 10,000 sectors
sector requests are in the following format: <sector to read> <time request arrives>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#define MAX_LINE_LENGTH 256
//------------------------------------structs-------------------------
typedef struct Request
{                        // I/O request
    unsigned int arrive; // arrival time
    unsigned int sector; // basically index of reuqst location
} Request;
// linked list structs
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
// Linked list funcs
Node *addNode(List *list);
void clearList(List *list);
void removeNode(Node *prev, Node *curr, List *list);
//------------------------------------globals-------------------------
int headPos;           // current potition of the head
int headMovement;      // total distance head has moved up to current point in time
unsigned int time = 0; // total time
char direction;        // current direction of head movement
int distance;          // distance between head and next sector
// bool reverseDirection = false;//boolean to help track whether we have reversed direction or not
int reverseCost = 0; // time cost to reverse direction

//----------------------------------------fcfs---------------------------------------------------
/**psuedo code:
 * Since list is passed in time sequence order, just go through the list in order updating
 * time, head position, direction, and total distance passed. One thing to check though is
 * whether the following request has arrived yet, if not we must wait for it first (update time
 * to when it arrives)
 */
void fcfs(List *list)
{
    Node *nextRequest = list->head;
    while (nextRequest != NULL)
    {
        // create request ptr to data for ease
        Request *r = (Request *)nextRequest->data;
        // check if we need to wait for the next request
        if (r->arrive > time)
        {
            time += r->arrive - time;
        }
        // check if direction is reversed
        if ((r->sector > headPos && direction == 'd') || (r->sector < headPos && direction == 'a'))
        {
            reverseCost = 3;
            direction = (direction == 'a') ? 'd' : 'a';
        }
        // find distance
        distance = abs(r->sector - headPos);
        // update time, head movement and position
        time += (distance / 10) + reverseCost;
        headMovement += distance;
        headPos = r->sector;
        // print the selected sector for easy testing
        printf("-> %d\n", r->sector);
        // reset/set things up for next itteration
        reverseCost = 0;
        nextRequest = nextRequest->next;
    }
    // print final results
    printf("FCFS: total time was: %d, and total head movement was: %d\n", time, headMovement);
}
//----------------------------------------sstf---------------------------------------------------
/**psuedo code:
 * At first make a pool of all available requests (that is requests that have arrived up to this time),
 * then go through that pool and find the request that is closest to head position, service that request
 * by updating all appropriate fields accordingly, and deleting it from the list. If no requests are in
 * the pool, but the list size isn't yet 0, that must mean more requests are coming at a later time
 * and due to the sorted nature of the list we may update time to be the arrival time of first request in list
 * as this must be earliest time the next request shall arrive
 */
void sstf(List *list)
{
    while (list->size != 0)
    {
        int s = 0; // this will be the number of available requests
        // array for available requests, max length is size of list
        Node **availableRequests = (Node **)malloc(list->size * sizeof(list->head));
        if (availableRequests == NULL)
        { // make sure malloc worked
            return;
        }
        // find all available requests that have arrived and put them in the array
        Node *nextRequest = list->head;
        while (nextRequest != NULL && ((Request *)nextRequest->data)->arrive <= time)
        {
            availableRequests[s] = nextRequest;
            ++s;
            nextRequest = nextRequest->next;
        }

        // case where no requests are available at this time
        if (s == 0)
        {
            time = ((Request *)(list->head)->data)->arrive; // set time to next available request
            continue;
        }

        // Find request with minimum distance from the head
        Node *selectReq = availableRequests[0];                         // this will eventually be the selected request
        distance = abs(((Request *)selectReq->data)->sector - headPos); // keep track of min distance
        int index = 0;                                                  // this will be used later when removing request from list
        for (int i = 0; i < s; i++)
        {
            if (abs(((Request *)availableRequests[i]->data)->sector - headPos) < distance)
            {
                selectReq = availableRequests[i];
                distance = abs(((Request *)availableRequests[i]->data)->sector - headPos);
                index = i;
            }
        }
        // service selected request
        // check if direction is reversed
        if ((((Request *)selectReq->data)->sector > headPos && direction == 'd') || (((Request *)selectReq->data)->sector < headPos && direction == 'a'))
        {
            reverseCost = 3;
            direction = (direction == 'a') ? 'd' : 'a';
        }

        // increase counters and reset everything for next round
        headMovement += distance;
        time += (distance / 10) + reverseCost;
        headPos = ((Request *)selectReq->data)->sector;
        reverseCost = 0;
        // print selected request for easy testing
        printf("-> %d\n", ((Request *)selectReq->data)->sector);
        // remove request from list, since array always starts at head of list and is in order we can use index
        if (index == 0)
        {
            removeNode(NULL, selectReq, list); // node is head of list
        }
        else
        {
            removeNode(availableRequests[index - 1], selectReq, list); // node is not head of list
        }

        free(availableRequests); // free array allocated with malloc
    }
    // print final results
    printf("SSTF: total time was: %d, and total head movement was: %d\n", time, headMovement);
}
//----------------------------------------cscan--------------------------------------------------
/**psuedo code:
 * Go through list and look for the next available request in the direction head is moving,
 * (break ties by arrival time) check if the request would arrive by the time head gets there
 * if so, update fields and remove node from list, otherwise, update fields but dont remove node.
 * If there are no more requests in that direction, add the distance between head pos and 9999 + 1
 * and set head pos to 0. CSCAN is continuously moving in one direction so there is never a reverse
 * direction time penalty. Even when it comes back around I see it as a circular array so it going
 * from 9999 to 0 is one step in assending direction.
 */
void cscan(List *list)
{
    while (list->size != 0)
    {
        Node *selectReq = NULL;  // will be selected node
        Node *selectPrev = NULL; // needed for removing node
        // to go through list keeping track of current and previous node
        Node *curr = list->head;
        Node *prev = NULL;
        Request *sr = NULL; // this is the data of the selected node
        distance = INT_MAX; // init distance to max so we can find min
        while (curr != NULL)
        {
            Request *r = (Request *)curr->data; // this is data of current node
            // check if node is in the correct direction
            if ((r->sector <= headPos && direction == 'd') || (r->sector >= headPos && direction == 'a'))
            {
                // check if node is closer to head than previously selected node
                if (abs(r->sector - headPos) < distance)
                {
                    // if so we have a new selected node
                    selectReq = curr;
                    selectPrev = prev;
                    sr = (Request *)selectReq->data;
                    distance = abs(r->sector - headPos);
                }
            }
            // update so that loop can keep going
            prev = curr;
            curr = curr->next;
        }
        // suppose we havent found any requests in correct direction from head
        if (selectReq == NULL)
        {
            // in that case we can scan all the way to the end and around
            if (direction == 'a')
            { // CSCAN is moving -> direction
                distance = abs(9999 - headPos) + 1;
                headPos = 0; // set position to beginning end
            }
            else
            { // CSCAN is moving <- direction
                distance = abs(0 - headPos) + 1;
                headPos = 9999; // set position to beginning end
            }
            // we need to update the time and movement we just spent on looping around
            time += (distance / 10);
            headMovement += distance;
            printf("-> scan back to first pos: %d\n", headPos); // let the people know
            continue;
        }
        // otherwise, we check whether by the time we get to that position the request would have come in
        if ((time + (distance / 10)) >= sr->arrive)
        {
            // if it has we can update everything and print that we serviced that request and remove it from the list
            time += (distance / 10);
            headPos = sr->sector;
            headMovement += distance;
            printf("-> %d\n", ((Request *)selectReq->data)->sector);
            removeNode(selectPrev, selectReq, list);
        }
        else
        {
            // otherwise, we update everything, moving over one extra sector but dont print or remove request
            distance++;
            time += (distance / 10);
            headPos = (sr->sector) + 1;
            headMovement += distance;
        }
    }
    // print final results
    printf("CSCAN: total time was: %d, and total head movement was: %d\n", time, headMovement);
}
//----------------------------------------look---------------------------------------------------
/**psuedo code:
 * create a pool of all requests that have arrived so far and that are to the correct side of
 * the head position. then find the closest one, and service it by updating all fields and
 * removing it from the list. Once there are no requests in pool switch directions and also
 * check if we need to wait some time for the next request to come in, adn start scanning the other way.
 */
void look(List *list)
{
    while (list->size != 0)
    {
        // find all available requests (based on time and direction)
        int s = 0;                                                                   // num of available requests (s for size of that array)
        Node **availableRequests = (Node **)malloc(list->size * sizeof(list->head)); // array of available requests
        if (availableRequests == NULL)
        { // safety check
            return;
        }
        // list through all requests available at this time
        Node *nextRequest = list->head;
        while (nextRequest != NULL && ((Request *)nextRequest->data)->arrive <= time)
        {
            // only add them to array if they are on the correct side
            if ((((Request *)nextRequest->data)->sector <= headPos && direction == 'd') || (((Request *)nextRequest->data)->sector >= headPos && direction == 'a'))
            {
                availableRequests[s] = nextRequest;
                ++s;
            }
            nextRequest = nextRequest->next;
        }
        // If no requests in this direction at this time
        if (s == 0)
        {
            direction = (direction == 'a') ? 'd' : 'a'; // reverse the direction
            reverseCost = 3;                            // penalty will be added in next itteration
            // check if we need to wait for the next request to come in
            if (((Request *)list->head->data)->arrive > time)
            {
                time = ((Request *)list->head->data)->arrive;
            }
            continue;
        }
        // Now find min distance from head among available requests
        Node *selectReq = availableRequests[0];                         // this will be the selected request
        distance = abs(((Request *)selectReq->data)->sector - headPos); // will be min distance == distance from head to selected request
        for (int i = 0; i < s; i++)
        {
            if (abs(((Request *)availableRequests[i]->data)->sector - headPos) < distance)
            {
                selectReq = availableRequests[i];
                distance = abs(((Request *)availableRequests[i]->data)->sector - headPos);
            }
        }
        // service selected request
        // increase counters and reset everything for next round
        time += (distance / 10) + reverseCost;
        headMovement += distance;
        headPos = ((Request *)selectReq->data)->sector;
        reverseCost = 0;
        // print selected request
        printf("-> %d\n", ((Request *)selectReq->data)->sector);
        // remove selected request
        if (selectReq == list->head)
        { // request is the head
            removeNode(NULL, selectReq, list);
        }
        else
        { // request is not the head, in which case we do need to find the prev
            Node *prev = list->head;
            while (prev->next != NULL)
            {
                if (prev->next == selectReq)
                {
                    break;
                }
                prev = prev->next;
            }
            removeNode(prev, selectReq, list);
        }

        free(availableRequests); // deallocate array
    }
    // print final results
    printf("LOOK: total time was: %d, and total head movement was: %d\n", time, headMovement);
}
//------------------------------------main----------------------------
int main(int argc, char *argv[])
{
    char line[MAX_LINE_LENGTH];
    List requests = {0};         // linked list of requests
    char algorithm = argv[1][0]; // which algorithm we will use
    headPos = atoi(argv[2]);     // initial head position
    direction = argv[3][0];      // initial direction

    while (fgets(line, sizeof line, stdin))
    { // reading line by line from input

        size_t eol = strcspn(line, "\n");
        line[eol] = '\0'; // trim \n at the end of the line

        // create new Request
        Node *curNode = addNode(&requests);
        curNode->data = malloc(sizeof(Request));
        Request *newRequest = (Request *)curNode->data;
        char delim[] = "\t ";
        char *ptr = strtok(line, delim); // sector
        newRequest->sector = atoi(ptr);

        ptr = strtok(NULL, delim); // arrival
        newRequest->arrive = atoi(ptr);
    }
    // call correct algorithm
    switch (algorithm)
    {
    case 'F':
        fcfs(&requests);
        break;
    case 'T':
        sstf(&requests);
        break;
    case 'C':
        cscan(&requests);
        break;
    case 'L':
        look(&requests);
        break;
    }
    // deallocate list
    clearList(&requests);
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
// remove node, if node is head pass NULL as prev
void removeNode(Node *prev, Node *curr, List *list)
{
    if (prev == NULL)
    { // remove head
        list->head = curr->next;
        --list->size;
        free(curr->data);
        free(curr);
        return;
    }
    // node is in the middle or end
    prev->next = curr->next;
    --list->size;
    free(curr->data);
    free(curr);
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