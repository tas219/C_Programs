/**
Author: Taisia Zhizhina
Date: June 5 2024
This code simulates a Least Recently Used algorithm for paging
format of input is long list of logical addresses and memory oops from stdin
    r 12341234
    w 56785678
where r is read and w is write, if memory address is written to the data
has to be sent to swap space once the page leaves the frame

in this simplified scenario the minor and major page faults aren't exactly true to their deffinition
minor page faults are counted if the data at address doesnt have to be sent to swap space
major page fault counted if it does.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
//----------------------structs---------------------------------------
typedef struct Page
{
    int frame;      // index of frame page is presumably stored in
    short validBit; // 0 for Invalid, 1 for Valid
    short readBit;  // 0 for read only, 1 for write
    short inSwap;   // 0 for not in swap space, 1 for in swap space
    int lastCalled;
} Page;

typedef struct Frame
{
    Page *pg;     // page stored in this frame
    char freeBit; // 0 for frame is free, 1 for frame not free
} Frame;
//----------------------globals---------------------------------------
int n;             // num of frames
int p;             // bits of offset
int hits = 0;      // hit counter
int mjpf = 0;      // major page faults counter
int mnpf = 0;      // minor page faults counter
int swapSpace = 0; // pages in swap space counter
int count = 0;     // counter to implement LRU
Page *pageTable;   // Page Table
Frame *frames;     // array of frames
//----------------------funcs-----------------------------------------
/**
 * Loops over all frames to see if any are marked as free
 * @returns index of first free frame, or -1 if free frame not found
 */
int findFreeFrame()
{
    // I get this is a little redundant, but keeping a list of free frames sounded complicated
    for (int i = 0; i < n; i++)
    {
        if (frames[i].freeBit == 0)
        {
            return i;
        }
    }
    return -1;
}
/**
 * Loops over frames to find the one least recently used (by checking when it was called last)
 * @returns index of LRU frame
 */
int findVictimFrame()
{

    int fIndex = -1;
    int min = INT_MAX;
    for (int i = 0; i < n; i++)
    {
        if ((frames[i].pg)->lastCalled < min)
        {
            min = (frames[i].pg)->lastCalled;
            fIndex = i;
        }
    }
    return fIndex;
}
/**
 * Updates frame to be free, updates page in that frame to be invalid
 * Technically name is misleading as it is called even in minor PF
 * But since we aren't dealing with the actual data, it's hard to make a distinction
 * between uploading data to disk vs trashing it
 * @param frame: index of frame to update
 */
void sawpOut(int frame)
{

    Frame *f = &frames[frame];
    Page *pg = f->pg;
    pg->validBit = 0;
    f->freeBit = 0;
    return;
}
/**
 * Updates info on frame that is getting a new page, and info on page that is new
 * @param adr page being added
 * @param frame frame page is to be put in
 */
void swapIn(unsigned int adr, int frame)
{
    // update frame info
    Frame *f = &frames[frame];
    Page *p = &pageTable[adr];
    f->pg = &pageTable[adr];
    f->freeBit = 1;
    // update page table
    p->frame = frame;
    p->validBit = 1;
    return;
}
/**
 * Find page in PT, see if it is in memory, if not find frame to put it in
 * then print physical address. If yes, just print physical address
 * @param pgNum index of page in PT
 * @param offset offset of data we are looking for
 */
void refrencePage(unsigned int pgNum, unsigned int offset)
{
    Page page = pageTable[pgNum];
    int physAdr;

    if (page.validBit)
    { // page table entry is Valid therefore it's in mem
        ++hits;
        physAdr = (page.frame << p) | offset;
        printf("Physical address: %u, Frame: %d, Offset: %u\n", physAdr, page.frame, offset);
        return;
    }
    // page is not in memory, now we decide if its a major or minor page fault
    int newFrame; // will either be free frame or victim frame
    newFrame = findFreeFrame();

    if (newFrame == -1)
    {                                 // No free frames
        newFrame = findVictimFrame(); // find victim frame
        // if pages first time getting kicked out, add to swap space
        if (frames[newFrame].pg->inSwap == 0)
        {
            ++swapSpace;
            ++frames[newFrame].pg->inSwap;
        }
        // if page in victim frame was written to -> major PF
        if ((frames[newFrame].pg)->readBit)
        {
            ++mjpf;
        }
        else
        { // victim frame page was read only and we dont swapit out (minor PF)
            ++mnpf;
        }
        sawpOut(newFrame); // technically this method just kicks the page out of the frame and updates its PT entry
    }

    else
    { // if theres a free frame we have minor page fault
        ++mnpf;
    }

    swapIn(pgNum, newFrame); // swap in our page

    physAdr = (newFrame << p) | offset; // find physical address
    printf("Physical address: %u, Frame: %d, Offset: %u\n", physAdr, newFrame, offset);
    return;
}
//----------------------main------------------------------------------
int main(int argc, char *argv[])
{
    n = atoi(argv[1]);
    p = atoi(argv[2]);
    const int ptSize = (int)pow(2, (32 - p));

    pageTable = (Page *)malloc(ptSize * sizeof(*pageTable));
    if (pageTable == NULL)
    {
        return 1;
    }
    frames = (Frame *)malloc(n * sizeof(Frame));
    if (frames == NULL)
    {
        return 1;
    }
    // for loop to init all elements in frames

    char line[16];
    while (fgets(line, sizeof line, stdin))
    {
        ++count; // update count
        // variables used in loop
        // will be read from line
        char readBit;
        unsigned int fullAddress;
        int mask; // used to geth the following two
        unsigned int offset;
        unsigned int pgNum;

        // strip line
        size_t eol = strcspn(line, "\n");
        line[eol] = '\0';
        // split string into the two important parts
        char delim[] = "\t ";
        char *ptr = strtok(line, delim);
        readBit = ptr[0];
        ptr = strtok(NULL, delim);
        fullAddress = atoi(ptr);

        // use bitmask to get page table # and offset
        mask = (1 << p) - 1;
        offset = fullAddress & mask;
        pgNum = (fullAddress & (~mask)) >> p;

        Page *cur = &pageTable[pgNum]; // grab current page
        cur->lastCalled = count;       // update last called
        if (readBit == 'w')
        { // set readBit to "Write" if write is selected
            cur->readBit = 1;
        }
        // print virtual address to compare to physical address, optional(I commented it out bc it looked busy)
        // printf("Virtual Address: %u, PageNum: %u, Offset: %u\n", fullAddress, pgNum, offset);
        // find physical address of the page
        refrencePage(pgNum, offset);
    }
    // print stats
    printf("--------------------stats--------------------\n");
    printf("Minor Page Faults: %d\n", mnpf);
    printf("Major Page Faults: %d\n", mjpf);
    printf("Page Hits: %d\n", hits);
    printf("Pages in swap space: %d\n", swapSpace);
    // deallocate mem
    free(pageTable);
    free(frames);
    return 0;
}