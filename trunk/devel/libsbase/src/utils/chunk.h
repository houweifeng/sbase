#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#ifdef HAVE_SSL
#include "xssl.h"
#endif
#ifndef _CHUNK_H
#define _CHUNK_H
#define CHUNK_MEM   0x02
#define CHUNK_FILE  0x04
#define CHUNK_ALL  (CHUNK_MEM | CHUNK_FILE)
#define CHUNK_FILE_NAME_MAX 1024 
#ifndef MMAP_CHUNK_SIZE
#define MMAP_CHUNK_SIZE         1048576
//#define MMAP_CHUNK_SIZE       2097152
//#define MMAP_CHUNK_SIZE       3145728
//#define MMAP_CHUNK_SIZE       4194304
//#define MMAP_CHUNK_SIZE       8388608 
#endif
#ifndef CHUNK_BLOCK_SIZE
#define CHUNK_BLOCK_SIZE        8192 
#endif
#define CHUNK_STATUS_ON     0x01
#define CHUNK_STATUS_OVER   0x02
typedef struct _CHUNK
{
    char *data;
    int ndata;
    off_t left;
    int bsize;
    char *end;
    char *mdata;
    int  offmdata;
    int nmdata;
    int type;
    int fd;
    char filename[CHUNK_FILE_NAME_MAX];
    off_t size;
    off_t offset;
    int n ;
    int status;
}CHUNK;
typedef struct _CHUNK * PCHUNK;
#define CK(ptr) ((CHUNK *)ptr)
#define CK_DATA(ptr) (CK(ptr)->data)
#define CK_NDATA(ptr) (CK(ptr)->ndata)
#define CK_MDATA(ptr) (CK(ptr)->mdata)
#define CK_OFFMDATA(ptr) (CK(ptr)->offmdata)
#define CK_NMDATA(ptr) (CK(ptr)->nmdata)
#define CK_LEFT(ptr) (CK(ptr)->left)
#define CK_END(ptr) (CK(ptr)->end)
#define CK_TYPE(ptr) (CK(ptr)->type)
#define CK_FD(ptr) (CK(ptr)->fd)
#define CK_FILENAME(ptr) (CK(ptr)->filename)
#define CK_SIZE(ptr) (CK(ptr)->size)
#define CK_BSIZE(ptr) (CK(ptr)->bsize)
#define CK_OFFSET(ptr) (CK(ptr)->offset)
#define CK_STATUS(ptr) (CK(ptr)->status)
#define CKN(ptr) (CK(ptr)->n)
#define CK_SET_BSIZE(ptr, len)                                                              \
{                                                                                           \
    if(ptr && len > 0)                                                                      \
    {                                                                                       \
        if(len > CK_BSIZE(ptr))                                                             \
        {                                                                                   \
            CKN(ptr) = (len/CHUNK_BLOCK_SIZE);                                              \
            if(len % CHUNK_BLOCK_SIZE) ++CKN(ptr);                                          \
            CK_BSIZE(ptr)  = CKN(ptr) * CHUNK_BLOCK_SIZE;                                   \
            if((CK_DATA(ptr)   = (char *)realloc(CK_DATA(ptr), CK_BSIZE(ptr))))             \
            {                                                                               \
                memset(CK_DATA(ptr), 0, CK_BSIZE(ptr));                                     \
            }                                                                               \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            if(CK_DATA(ptr) && CK_BSIZE(ptr) > 0)                                           \
                memset(CK_DATA(ptr), 0, CK_BSIZE(ptr));                                     \
        }                                                                                   \
        CK_END(ptr)    = CK_DATA(ptr);                                                      \
    }                                                                                       \
}

#define CK_MEM(ptr, len)                                                                    \
{                                                                                           \
    if(ptr && len > 0)                                                                      \
    {                                                                                       \
        CK_TYPE(ptr)   = CHUNK_MEM;                                                         \
        CK_STATUS(ptr) = CHUNK_STATUS_ON;                                                   \
        CK_SIZE(ptr)   = len;                                                               \
        CK_LEFT(ptr)   = len;                                                               \
        if(len > CK_BSIZE(ptr))                                                             \
        {                                                                                   \
            CKN(ptr) = (len/CHUNK_BLOCK_SIZE);                                              \
            if((len % CHUNK_BLOCK_SIZE)) ++CKN(ptr);                                        \
            CK_BSIZE(ptr)  = CKN(ptr) * CHUNK_BLOCK_SIZE;                                   \
            if((CK_DATA(ptr)   = (char *)realloc(CK_DATA(ptr), CK_BSIZE(ptr))))             \
            {                                                                               \
                memset(CK_DATA(ptr), 0, CK_BSIZE(ptr));                                     \
            }                                                                               \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            if(CK_DATA(ptr) && CK_BSIZE(ptr) > 0)                                           \
                memset(CK_DATA(ptr), 0, CK_BSIZE(ptr));                                     \
        }                                                                                   \
        CK_END(ptr)    = CK_DATA(ptr);                                                      \
        CK_NDATA(ptr)  = 0;                                                                 \
    }                                                                                       \
}

#define CK_FILE(ptr, file, off, len)                                                        \
{                                                                                           \
    if(ptr && file && off >= 0 && len > 0)                                                  \
    {                                                                                       \
        CK_TYPE(ptr)    = CHUNK_FILE;                                                       \
        CK_STATUS(ptr)  = CHUNK_STATUS_ON;                                                  \
        CK_SIZE(ptr)    = len;                                                              \
        CK_LEFT(ptr)    = len;                                                              \
        CK_OFFSET(ptr)  = off;                                                              \
        CK_NDATA(ptr)   = 0;                                                                \
        if(file)strcpy(CK_FILENAME(ptr), file);                                             \
    }                                                                                       \
}
#define CHUNK_STATUS(ptr) ((CK_LEFT(ptr) == 0) ? CHUNK_STATUS_OVER : CHUNK_STATUS_ON)
/* read to mem chunk from fd */
#ifdef HAVE_SSL
#define CK_READ_SSL(ptr, ssl) ((ptr && ssl && CK_DATA(ptr)                                  \
            && (CKN(ptr) = SSL_read(XSSL(ssl), CK_END(ptr), CK_LEFT(ptr))) > 0) ?           \
            (((CK_END(ptr) += CKN(ptr)) && (CK_LEFT(ptr) -= CKN(ptr)) >= 0                  \
            && (CK_NDATA(ptr) += CKN(ptr)) > 0                                              \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0 ) ? CKN(ptr) : -1): -1)
#else
#define CK_READ_SSL(ptr, ssl)
#endif
#define CK_READ(ptr, fd) ((ptr && fd > 0 && CK_DATA(ptr)                                    \
            && (CKN(ptr) = read(fd, CK_END(ptr), CK_LEFT(ptr))) > 0) ?                      \
            (((CK_END(ptr) += CKN(ptr)) && (CK_LEFT(ptr) -= CKN(ptr)) >= 0                  \
            && (CK_NDATA(ptr) += CKN(ptr)) > 0                                              \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0 ) ? CKN(ptr) : -1): -1)

/* write to fd from mem chunk */
#ifdef HAVE_SSL
#define CK_WRITE_SSL(ptr, ssl) ((ptr && ssl && CK_DATA(ptr)                                 \
        && (CKN(ptr) = SSL_write(XSSL(ssl), CK_END(ptr), CK_LEFT(ptr))) > 0) ?              \
            (((CK_END(ptr) += CKN(ptr)) && (CK_LEFT(ptr) -= CKN(ptr)) >= 0                  \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0) ? CKN(ptr) : -1): -1)
#else
#define CK_WRITE_SSL(ptr, ssl)
#endif
#define CK_WRITE(ptr, fd) ((ptr && fd > 0 && CK_DATA(ptr)                                   \
            && (CKN(ptr) = write(fd, CK_END(ptr), CK_LEFT(ptr))) > 0) ?                     \
            (((CK_END(ptr) += CKN(ptr)) && (CK_LEFT(ptr) -= CKN(ptr)) >= 0                  \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0) ? CKN(ptr) : -1): -1)
/* fill to memory from buffer */
#define CK_MEM_FILL(ptr, pdata, npdata) ((ptr && pdata && npdata > 0 && CK_DATA(ptr)        \
            && (CKN(ptr) = ((npdata > CK_LEFT(ptr)) ? CK_LEFT(ptr) : npdata)) > 0           \
            && memcpy(CK_END(ptr), pdata, CKN(ptr)))?                                       \
            (((CK_LEFT(ptr) -= CKN(ptr)) >= 0 && (CK_END(ptr) += CKN(ptr))                  \
            && (CK_NDATA(ptr) += CKN(ptr)) > 0                                              \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr):-1):-1)

/* push to memory chunk from bufer */
#define CK_MEM_COPY(ptr, pdata, npdata) ((ptr && pdata && npdata > 0 && CK_DATA(ptr)        \
            && (CKN(ptr) = ((npdata > CK_SIZE(ptr)) ? CK_SIZE(ptr) : npdata)) > 0           \
            && memcpy(CK_END(ptr), pdata, CKN(ptr)))? CKN(ptr):-1)                          

#define CK_CHECKFD(ptr) ((ptr)? ((CK_FD(ptr) <= 0) ?                                        \
     (CK_FD(ptr) = open(CK_FILENAME(ptr), O_CREAT|O_RDWR, 0644)): CK_FD(ptr)): -1)
#define CK_FLEFT(ptr) ((CK_LEFT(ptr) > CK_BSIZE(ptr))?CK_BSIZE(ptr) : CK_LEFT(ptr))
/* read to file from fd */
#ifdef HAVE_SSL
#define CK_READ_TO_FILE_SSL(ptr, ssl) ((ptr && ssl && CK_LEFT(ptr) > 0 && CK_DATA(ptr)      \
            && (CK_NDATA(ptr) = CK_FLEFT(ptr)) > 0 && CK_CHECKFD(ptr) > 0                   \
            && (CKN(ptr) = SSL_read(XSSL(ssl), CK_DATA(ptr), CK_NDATA(ptr))) > 0            \
            && lseek(CK_FD(ptr), CK_OFFSET(ptr), SEEK_SET) >= 0                             \
            && write(CK_FD(ptr), CK_DATA(ptr), CKN(ptr)) == CKN(ptr)) ?                     \
            (((CK_LEFT(ptr) -= CKN(ptr)) >= 0 && (CK_OFFSET(ptr) += CKN(ptr)) > 0           \
            && (CK_FD(ptr) = close(CK_FD(ptr))) >= 0                                        \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr): -1): -1)
#else
#define CK_READ_TO_FILE_SSL(ptr, ssl)
#endif
#define CK_READ_TO_FILE(ptr, fd) ((ptr && fd > 0 && CK_LEFT(ptr) > 0 && CK_DATA(ptr)        \
            && (CK_NDATA(ptr) = CK_FLEFT(ptr)) > 0 && CK_CHECKFD(ptr) > 0                   \
            && (CKN(ptr) = read(fd, CK_DATA(ptr), CK_NDATA(ptr))) > 0                       \
            && lseek(CK_FD(ptr), CK_OFFSET(ptr), SEEK_SET) >= 0                             \
            && write(CK_FD(ptr), CK_DATA(ptr), CKN(ptr)) == CKN(ptr)) ?                     \
            (((CK_LEFT(ptr) -= CKN(ptr)) >= 0 && (CK_OFFSET(ptr) += CKN(ptr)) > 0           \
            && (CK_FD(ptr) = close(CK_FD(ptr))) >= 0                                        \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr): -1): -1)
/* mmap buffer */
int ck_mmap(void *);
#define CK_MMAP(ptr) ((CK_NMDATA(ptr) <= 0)?(((CK_MDATA(ptr))?(munmap(CK_MDATA(ptr), MMAP_CHUNK_SIZE) == 0):1) && (CK_MDATA(ptr) = (char *)mmap(NULL, MMAP_CHUNK_SIZE, PROT_READ, MAP_PRIVATE, CK_FD(ptr), (CK_OFFSET(ptr)/(off_t)MMAP_CHUNK_SIZE) * (off_t)MMAP_CHUNK_SIZE)) != (char *)-1 && (CK_OFFMDATA(ptr) = (int)(CK_OFFSET(ptr)%((off_t)MMAP_CHUNK_SIZE))) >= 0 && (CK_NMDATA(ptr) = (CK_LEFT(ptr) > MMAP_CHUNK_SIZE) ? (MMAP_CHUNK_SIZE - CK_OFFMDATA(ptr)) : CK_LEFT(ptr)) > 0):1) 

/* write to fd from file */
#ifdef HAVE_SSL
#define CK_WRITE_FROM_FILE_SSL(ptr, ssl) ((ptr && ssl && CK_LEFT(ptr) > 0 && CK_DATA(ptr)   \
            && (CK_NDATA(ptr) = CK_FLEFT(ptr)) > 0 && CK_CHECKFD(ptr) > 0 && CK_MMAP(ptr)   \
            && (CKN(ptr)=SSL_write(XSSL(ssl),CK_MDATA(ptr)+CK_OFFMDATA(ptr),CK_NMDATA(ptr)))>0)? \
            (((CK_OFFSET(ptr) += CKN(ptr)) > 0 && (CK_LEFT(ptr) -= CKN(ptr)) >= 0           \
            && (CK_OFFMDATA(ptr) += CKN(ptr)) && (CK_NMDATA(ptr) -= CKN(ptr)) >= 0          \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr) : -1) : -1)
#else
#define CK_WRITE_FROM_FILE_SSL(ptr, ssl)
#endif
//            && lseek(CK_FD(ptr), CK_OFFSET(ptr), SEEK_SET) >= 0                             \
            && (CKN(ptr) = read(CK_FD(ptr), CK_DATA(ptr), CK_NDATA(ptr))) > 0               \

#define CK_WRITE_FROM_FILE(ptr, fd) ((ptr && fd > 0 && CK_LEFT(ptr) > 0 && CK_DATA(ptr)     \
            && (CK_NDATA(ptr) = CK_FLEFT(ptr)) > 0 && CK_CHECKFD(ptr) > 0 && CK_MMAP(ptr)   \
            && (CKN(ptr)=write(fd, CK_MDATA(ptr)+CK_OFFMDATA(ptr),CK_NMDATA(ptr))) > 0)?    \
            (((CK_OFFSET(ptr) += CKN(ptr)) > 0 && (CK_LEFT(ptr) -= CKN(ptr)) >= 0           \
            && (CK_OFFMDATA(ptr) += CKN(ptr)) && (CK_NMDATA(ptr) -= CKN(ptr)) >= 0          \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr) : -1) : -1)
/* fill to file from buffer */
#define CK_FILE_FILL(ptr, pdata, npdata) ((ptr && pdata && npdata > 0                       \
            && (CKN(ptr) = ((npdata > CK_LEFT(ptr)) ? CK_LEFT(ptr) : npdata)) > 0           \
            && CK_CHECKFD(ptr) > 0 && lseek(CK_FD(ptr), CK_OFFSET(ptr), SEEK_SET) >= 0      \
            && write(CK_FD(ptr), pdata, CKN(ptr)) > 0 )?                                    \
            (((CK_LEFT(ptr) -= CKN(ptr)) >= 0 && (CK_OFFSET(ptr) += CKN(ptr)) > 0           \
            && (CK_FD(ptr)      = close(CK_FD(ptr))) >= 0                                   \
            && (CK_STATUS(ptr) = CHUNK_STATUS(ptr)) > 0)? CKN(ptr) :-1):-1)
/* CHUNK WRITE */
#ifdef HAVE_SSL
#define CHUNK_WRITE_SSL(ptr, ssl) ((CK_TYPE(ptr) == CHUNK_MEM)?  \
        CK_WRITE_SSL(ptr, ssl) : CK_WRITE_FROM_FILE_SSL(ptr, ssl))
#else
#define  CHUNK_WRITE_SSL(ptr, ssl)
#endif
#define CHUNK_WRITE(ptr, fd) ((CK_TYPE(ptr) == CHUNK_MEM)?  \
        CK_WRITE(ptr, fd) : CK_WRITE_FROM_FILE(ptr, fd))
/* CHUNK READ */
#ifdef HAVE_SSL
#define CHUNK_READ_SSL(ptr, ssl) ((CK_TYPE(ptr) == CHUNK_MEM)? \
        CK_READ_SSL(ptr, ssl) : CK_READ_TO_FILE_SSL(ptr, ssl))
#else
#define CHUNK_READ_SSL(ptr, ssl)
#endif
#define CHUNK_READ(ptr, fd) ((CK_TYPE(ptr) == CHUNK_MEM)? \
        CK_READ(ptr, fd) : CK_READ_TO_FILE(ptr, fd))
/* CHUNK FILL from buffer */
#define CHUNK_FILL(ptr, pdata, npdata) ((CK_TYPE(ptr) == CHUNK_MEM)? \
        CK_MEM_FILL(ptr, pdata, npdata) : CK_FILE_FILL(ptr, pdata, npdata))

#ifdef _SBASE_MIN_MM_
/* reset chunk */
#define CK_RESET(ptr)                                                                       \
{                                                                                           \
    if(ptr)                                                                                 \
    {                                                                                       \
        if(CK_MDATA(ptr)){munmap(CK_MDATA(ptr), CK_BSIZE(ptr)); CK_MDATA(ptr) = NULL;}      \
        if(CK_FD(ptr) > 0 ) close(CK_FD(ptr));                                              \
        if(CK_DATA(ptr)) free(CK_DATA(ptr));                                                \
        memset(ptr, 0, sizeof(CHUNK));                                                      \
    }                                                                                       \
}
#else 
/* reset chunk */
#define CK_RESET(ptr)                                                                       \
{                                                                                           \
    if(ptr)                                                                                 \
    {                                                                                       \
        if(CK_MDATA(ptr)){munmap(CK_MDATA(ptr), CK_BSIZE(ptr)); CK_MDATA(ptr) = NULL;}      \
        if(CK_FD(ptr) > 0 ) close(CK_FD(ptr));                                              \
		CK_NDATA(ptr) = 0;                                                                  \
		CK_NMDATA(ptr) = 0;                                                                 \
		CK_OFFMDATA(ptr) = 0;                                                               \
		CK_END(ptr) = NULL;                                                                 \
		CK_TYPE(ptr) = 0;                                                                   \
		CK_FD(ptr) = -1;                                                                    \
		memset(CK_FILENAME(ptr), 0, CHUNK_FILE_NAME_MAX);                                   \
		CK_SIZE(ptr) = 0;                                                                   \
		CK_OFFSET(ptr) = 0;                                                                 \
		CK_STATUS(ptr) = 0;                                                                 \
		CKN(ptr) = 0;                                                                       \
        CK_LEFT(ptr) = 0;                                                                   \
        CK_END(ptr) = CK_DATA(ptr);                                                         \
    }                                                                                       \
}
#endif
//        if(CK_DATA(ptr) && (CK_END(ptr) = CK_DATA(ptr)  = (char *)realloc(CK_DATA(ptr),     
//                        CHUNK_BLOCK_SIZE)))                                                 
//        {                                                                                   
//            memset(CK_DATA(ptr), 0, CHUNK_BLOCK_SIZE);                                      
//            CK_BSIZE(ptr)  = CHUNK_BLOCK_SIZE;                                              
//        }                                                                                   

/* clean chunk */
#define CK_CLEAN(ptr)                                                                       \
{                                                                                           \
    if(ptr)                                                                                 \
    {                                                                                       \
        if(CK_MDATA(ptr)){munmap(CK_MDATA(ptr), CK_BSIZE(ptr)); CK_MDATA(ptr) = NULL;}      \
        if(CK_DATA(ptr)) free(CK_DATA(ptr));                                                \
        free(ptr);ptr = NULL;                                                               \
    }                                                                                       \
}
#define CK_INIT(ptr)                                                                        \
{                                                                                           \
    if((ptr = calloc(1, sizeof(CHUNK))))                                                    \
    {                                                                                       \
        CK_FD(ptr) = -1;                                                                    \
        CK_SET_BSIZE(ptr, CHUNK_BLOCK_SIZE);                                                \
    }                                                                                       \
}
#endif
