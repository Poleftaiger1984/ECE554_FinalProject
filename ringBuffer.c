#include "ringBuffer.h"

static volatile char buffer[RING_BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;

void rbInit()
{
    head = 0;
    tail = 0;
}

int rbPush(char c)
{
    int nextHead = (head + 1) % RING_BUFFER_SIZE;

    if (nextHead == tail)
    {
        return 0; //No space drop the character
    }

    buffer[head] = c;
    head = nextHead;
    return 1;
}

int rbPop(char *c)
{
    if (head == tail)
    {
        return 0; //No unread data if head and tail are in the same spot
    }

    *c = buffer[tail];

    tail = (tail + 1) % RING_BUFFER_SIZE;
    return 1;
}

int rbIsEmpty()
{
    return (head == tail);
}