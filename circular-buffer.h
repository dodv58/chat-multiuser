#ifndef __CIRCULAR_BUFFER__
#define __CIRCULAR_BUFFER__

#include <string.h>
#include <stdbool.h>

#define BUFF_SIZE 10000

typedef struct circular_buffer_s {
    unsigned char buffer[BUFF_SIZE];
    unsigned char *head;
    unsigned char *tail;
    int nbytes;
} cbuffer_t;

void cbuffer_init(cbuffer_t *cbuffer);
bool cbuffer_insert(cbuffer_t *cbuffer, void *data, int length);
int cbuffer_get(cbuffer_t *cbuffer, void *dest, int max_length, int length);
int cbuffer_get_msg(cbuffer_t *cbuffer, void *dest, int max_length);

#endif 
