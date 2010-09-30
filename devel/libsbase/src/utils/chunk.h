#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#ifdef HAVE_SSL
#include "xssl.h"
#endif
#ifndef _CHUNK_H
#define _CHUNK_H
#ifdef __cplusplus
extern "C" {
#endif
#define CHUNK_MEM   0x02
#define CHUNK_FILE  0x04
#define CHUNK_ALL  (CHUNK_MEM | CHUNK_FILE)
#define CHUNK_FILE_NAME_MAX     256
//#define CHUNK_BLOCK_MAX         262144
#define CHUNK_BLOCK_MAX         1024
//#define CHUNK_BLOCK_MAX       1048576
#ifndef MMAP_CHUNK_SIZE
#define MMAP_CHUNK_SIZE         4096
//#define MMAP_CHUNK_SIZE       1048576
//#define MMAP_CHUNK_SIZE       2097152
//#define MMAP_CHUNK_SIZE       2097152
//#define MMAP_CHUNK_SIZE       3145728
//#define MMAP_CHUNK_SIZE       4194304
//#define MMAP_CHUNK_SIZE       8388608 
#endif
#ifndef CHUNK_BLOCK_SIZE
#define CHUNK_BLOCK_SIZE        1024 
#endif
#define CHUNK_STATUS_ON         0x01
#define CHUNK_STATUS_OVER       0x02
typedef struct _CHUNK
{
    char *data;
    int ndata;
    int bsize;
    int type;
    int fd;
    int  status;
    off_t size;
    off_t offset;
    off_t left;
    char *mmap;
    char *end;
    char filename[CHUNK_FILE_NAME_MAX];
}CHUNK;
typedef struct _CHUNK * PCHUNK;
#define CHK(ptr) ((CHUNK *)ptr)
#define CHK_DATA(ptr) (CK(ptr)->data)
#define CHK_NDATA(ptr) (CK(ptr)->ndata)
#define CHK_LEFT(ptr) (CK(ptr)->left)
#define CHK_END(ptr) (CK(ptr)->end)
#define CHK_TYPE(ptr) (CK(ptr)->type)
#define CHK_FD(ptr) (CK(ptr)->fd)
#define CHK_FILENAME(ptr) (CK(ptr)->filename)
#define CHK_SIZE(ptr) (CK(ptr)->size)
#define CHK_BSIZE(ptr) (CK(ptr)->bsize)
#define CHK_OFFSET(ptr) (CK(ptr)->offset)
#define CHK_STATUS(ptr) (CK(ptr)->status)
/* set/initialize chunk mem */
int chunk_set_bsize(void *chunk, int len);
/* set/initialize chunk mem */
int chunk_mem(void *chunk, int len);
/* reading to chunk */
int chunk_read(void *chunk, int fd);
/* reading to chunk with SSL */
int chunk_read_SSL(void *chunk, void *ssl);
/* writting from chunk */
int chunk_write(void *chunk, int fd);
/* writting from chunk with SSL */
int chunk_write_SSL(void *chunk, void *ssl);
/* fill chunk memory */
int chunk_mem_fill(void *chunk, void *data, int ndata);
/* copy to chunk */
int chunk_mem_copy(void *chunk, void *data, int ndata);
/* read data to file from fd */
int chunk_read_to_file(void *chunk, int fd);
/* push data to file */
int chunk_read_to_file_SSL(void *chunk, void *ssl);
/* write from file */
int chunk_write_from_file(void *chunk, int fd);
/* write from file with SSL */
int chunk_write_from_file_SSL(void *chunk, void *ssl);
/* chunk file fill */
int chunk_file_fill(void *chunk, char *data, int ndata);
/* chunk reset */
void chunk_reset(void *chunk);
/* clean chunk */
void chunk_clean(void *chunk);
/* initialize chunk file */
int chunk_file(void *chunk, char *file, off_t offset, off_t len);
#ifdef __cplusplus
 }
#endif
#endif
