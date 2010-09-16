#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_SSL
#include "xssl.h"
#endif
#ifndef _MEMB_H
#define _MEMB_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _MEMB
{
    char *data;
    int  ndata;
    int block_size;
    int size;
    int left;
    char *end;
    char *p;
    int  n;
}MEMB;
#define MB_BLOCK_SIZE  65536
#define MB(ptr)        ((MEMB *)ptr)
#define MB_SIZE(ptr)   (MB(ptr)->size)
#define MB_BSIZE(ptr)  (MB(ptr)->block_size)
#define MB_LEFT(ptr)   (MB(ptr)->left)
#define MB_DATA(ptr)   (MB(ptr)->data)
#define MB_NDATA(ptr)  (MB(ptr)->ndata)
#define MB_END(ptr)    (MB(ptr)->end)
#define MBP(ptr)       (MB(ptr)->p)
#define MBN(ptr)       (MB(ptr)->n)
#define MB_RESIZE(ptr, nsize) (((MBN(ptr) = MB_BSIZE(ptr) * (((nsize / MB_BSIZE(ptr)))  \
                + (((nsize % MB_BSIZE(ptr)) == 0) ? 0 : 1))) > 0 ) ?                    \
            ((MB_DATA(ptr) = (char *)realloc(MB_DATA(ptr), (MB_SIZE(ptr) + MBN(ptr))))? \
              (((MB_END(ptr) = MB_DATA(ptr) + MB_NDATA(ptr))                            \
                && (MB_SIZE(ptr) += MBN(ptr)) && (MB_LEFT(ptr) += MBN(ptr))             \
                && memset(MB_END(ptr), 0, MB_LEFT(ptr)))? MB_DATA(ptr) : NULL)  : NULL) : NULL)
#define MB_CHECK(ptr) ((ptr)?((MB_LEFT(ptr) <= 0)?((MB_RESIZE(ptr, MB_BSIZE(ptr)))?0:-1):0):-1)
#define MB_INIT(ptr, b_size)                                                        \
do                                                                                  \
{                                                                                   \
    if((ptr = calloc(1, sizeof(MEMB))))                                             \
    {                                                                               \
        MB_BSIZE(ptr) = MB_BLOCK_SIZE;                                              \
        if(b_size > MB_BLOCK_SIZE) MB_BSIZE(ptr) = b_size;                          \
    }                                                                               \
}while(0)
#define MB_SET_BLOCK_SIZE(ptr, b_size) {MB_BSIZE(ptr) = b_size;}
#ifdef HAVE_SSL
#define MB_READ_SSL(ptr, ssl) ((MB_CHECK(ptr) == 0) ?                               \
    (((MBN(ptr) = SSL_read(XSSL(ssl), MB_END(ptr), MB_LEFT(ptr))) > 0 )?            \
         (((MB_END(ptr) += MBN(ptr)) && (MB_NDATA(ptr) += MBN(ptr)) >= 0            \
           && (MB_LEFT(ptr) -= MBN(ptr)) >= 0) ? MBN(ptr): -1) : MBN(ptr)) : -1)
#else 
#define MB_READ_SSL(ptr, ssl) -1
#endif
#define MB_READ(ptr, fd) ((MB_CHECK(ptr) == 0) ?                                    \
    (((MBN(ptr) = read(fd, MB_END(ptr), MB_LEFT(ptr))) > 0 )?                       \
         (((MB_END(ptr) += MBN(ptr)) && (MB_NDATA(ptr) += MBN(ptr)) >= 0            \
           && (MB_LEFT(ptr) -= MBN(ptr)) >= 0) ? MBN(ptr): -1) : MBN(ptr)) : -1)
#define MB_RECV(ptr, fd, flag) ((MB_CHECK(ptr) == 0) ?                              \
    (((MBN(ptr) = recv(fd, MB_END(ptr), MB_LEFT(ptr), flag)) > 0 )?                 \
         (((MB_END(ptr) += MBN(ptr)) && (MB_NDATA(ptr) += MBN(ptr)) >= 0            \
           && (MB_LEFT(ptr) -= MBN(ptr)) >= 0) ? MBN(ptr): -1) : MBN(ptr)) : -1)
#define MB_PUSH(ptr, pdata, npdata)                                                 \
do{                                                                                 \
    if(ptr && pdata && npdata > 0)                                                  \
    {                                                                               \
        if(MB_LEFT(ptr) < npdata) {MB_RESIZE(ptr, npdata);}                         \
        if(MB_DATA(ptr))                                                            \
        {                                                                           \
            MBN(ptr) = npdata;                                                      \
            MBP(ptr) = pdata;                                                       \
            while(MBN(ptr)-- > 0)                                                   \
            {                                                                       \
                MB_LEFT(ptr)--;                                                     \
                MB_NDATA(ptr)++;                                                    \
                *(MB_END(ptr))++ = *(MBP(ptr))++;                                   \
            }                                                                       \
        }                                                                           \
        else {MB_LEFT(ptr) = 0; MB_SIZE(ptr) = 0; MB_END(ptr) = NULL;}              \
    }                                                                               \
}while(0)
#define MB_DEL(ptr, npdata)                                                         \
do{                                                                                 \
    if(ptr && npdata > 0 && MB_DATA(ptr))                                           \
    {                                                                               \
        MBN(ptr)        = MB_NDATA(ptr) - npdata;                                   \
        MBP(ptr)        = MB_DATA(ptr) + npdata;                                    \
        MB_END(ptr)     = MB_DATA(ptr);                                             \
        MB_LEFT(ptr)    = MB_SIZE(ptr);                                             \
        MB_NDATA(ptr)   = 0;                                                        \
        while(MBN(ptr)-- > 0)                                                       \
        {                                                                           \
            *(MB_END(ptr))++ = *(MBP(ptr))++;                                       \
            MB_NDATA(ptr)++;                                                        \
            MB_LEFT(ptr)--;                                                         \
        }                                                                           \
    }                                                                               \
}while(0)
#define MB_STREND(ptr) {MB_PUSH(ptr, "\0", 1);}
#ifdef _SBASE_MIN_MM_
#define MB_RESET(ptr)                                                               \
do                                                                                  \
{                                                                                   \
    if(ptr)                                                                         \
    {                                                                               \
        MB_LEFT(ptr) = MB_SIZE(ptr) = 0;                                            \
        if(MB_DATA(ptr)){free(MB_DATA(ptr));}                                       \
        memset(ptr, 0, sizeof(MEMB));                                               \
        MB_BSIZE(ptr) = MB_BLOCK_SIZE;                                              \
    }                                                                               \
}while(0)
#else
#define MB_RESET(ptr)                                                               \
do{                                                                                 \
    if(ptr)                                                                         \
    {                                                                               \
        if(MB_SIZE(ptr) > MB_BSIZE(ptr) && MB_BSIZE(ptr) > 0)                       \
        {                                                                           \
            if(MB_DATA(ptr)) free(MB_DATA(ptr));                                    \
            MB_DATA(ptr) = NULL;                                                    \
            MB_LEFT(ptr) = MB_SIZE(ptr) = 0;                                        \
        }                                                                           \
        MB_END(ptr)  = MB_DATA(ptr);                                                \
        MB_NDATA(ptr)  = 0;                                                         \
    }                                                                               \
}while(0)
#endif
#define MB_CLEAN(ptr)                                                               \
do                                                                                  \
{                                                                                   \
    if(ptr)                                                                         \
    {                                                                               \
        if(MB_DATA(ptr)) free(MB_DATA(ptr));                                        \
        free(ptr);                                                                  \
        ptr = NULL;                                                                 \
    }                                                                               \
}while(0)

#ifdef __cplusplus
 }
#endif
#endif
