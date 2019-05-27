#include "circular-buffer.h"
#include <stdio.h>

void cbuffer_init(cbuffer_t *cbuffer) {
    memset(cbuffer->buffer, '\0', BUFF_SIZE);
    cbuffer->head = cbuffer->buffer;
    cbuffer->tail = cbuffer->buffer;
    cbuffer->nbytes = 0;
    return;
}
bool cbuffer_insert(cbuffer_t *cbuffer, void *data, int length) {
    if(length > BUFF_SIZE - cbuffer->nbytes) {
        printf("not enough space\n");
        exit(-1);
        return false;
    }
    if(cbuffer->nbytes == 0){
        memmove(cbuffer->buffer, data, length);
        cbuffer->tail += length - 1;
    } else {
        int i = 0;
        while(i < length){
            if(cbuffer->tail != &cbuffer->buffer[BUFF_SIZE -1]){
                cbuffer->tail++;
            } else {
                cbuffer->tail = cbuffer->buffer;
            }
            memmove(cbuffer->tail, &data[i], 1);
            i++;
        }
    }   
    cbuffer->nbytes += length;
    return true;
}
int cbuffer_get(cbuffer_t *cbuffer, void *dest, int max_length, int length) {
    if(length > max_length) return 0;
    if(cbuffer->nbytes == 0) return 0;
    memset(dest, '\0', max_length);
    int i = 0;
    while (i < length && cbuffer->nbytes != 0){
        memmove(&dest[i], cbuffer->head, 1);
        memset(cbuffer->head, '\0', 1);
        if(cbuffer->head != &cbuffer->buffer[BUFF_SIZE - 1])
            cbuffer->head++;
        else cbuffer->head = cbuffer->buffer;
        cbuffer->nbytes--;
        i++;
    }
    if(cbuffer->nbytes == 0){
        cbuffer->head = cbuffer->buffer;
        cbuffer->tail = cbuffer->buffer;
    }
    return i;
}
int cbuffer_get_msg(cbuffer_t *cbuffer, void *dest, int max_length){
    int len = 0;
    memmove(&len, cbuffer->head, sizeof(int));
    if(cbuffer->nbytes < len){
        return 0;
    }
    else{
        return cbuffer_get(cbuffer, dest, max_length, len);
    }
}
