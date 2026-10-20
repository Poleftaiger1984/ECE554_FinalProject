#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#define RING_BUFFER_SIZE 1024

void rbInit();

int rbPush(char c);

int rbPop(char *c);

int rbIsEmpty();

#endif