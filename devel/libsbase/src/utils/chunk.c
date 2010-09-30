#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "chunk.h"
/* initialize chunk */
CHUNK *chunk_init()
{
    return (CHUNK *)calloc(1, sizeof(CHUNK));
}

/* set/initialize chunk mem */
int chunk_set_bsize(void *chunk, int len)
{
    int n = 0, size = 0;

    if(chunk)
    {
        if(len > CHK(chunk)->bsize)
        {
            n = len / CHUNK_BLOCK_SIZE;
            if(len % CHUNK_BLOCK_SIZE) ++n;
            size = n * CHUNK_BLOCK_SIZE;
            if(CHK(chunk)->data) free(CHK(chunk)->data);
            CHK(chunk)->data = NULL;
            if((CHK(chunk)->data = (char *)calloc(1, size)))
            {
                CHK(chunk)->bsize = size;
            }
            else
            {
                CHK(chunk)->bsize = 0;
            }
        }
        CHK(chunk)->end = CHK(chunk)->data;
        CHK(chunk)->ndata = 0;
        return 0;
    }
    return -1;
}

/* set/initialize chunk mem */
int chunk_mem(void *chunk, int len)
{
    int n = 0, size = 0;

    if(chunk)
    {
        if(len > CHK(chunk)->bsize)
        {
            n = len/CHUNK_BLOCK_SIZE;
            if(len % CHUNK_BLOCK_SIZE) ++n;
            size = n * CHUNK_BLOCK_SIZE;
            if(CHK(chunk)->data) free(CHK(chunk)->data);
            CHK(chunk)->data = NULL;
            if((CHK(chunk)->data = (char *)calloc(1, size)))
            {
                CHK(chunk)->bsize = size;
            }
            else
            {
                CHK(chunk)->bsize = 0;
            }
        }
        CHK(chunk)->type = CHUNK_MEM;
        CHK(chunk)->status = CHUNK_STATUS_ON;
        CHK(chunk)->size = CHK(chunk)->left = len;
        CHK(chunk)->end = CHK(chunk)->data;
        CHK(chunk)->ndata = 0;
        return 0;
    }
    return -1;
}

/* initialize chunk file */
int chunk_file(void *chunk, char *file, off_t offset, off_t len )
{
    if(chunk && file && offset >= 0 && len > 0)
    {
        CHK(chunk)->type = CHUNK_FILE;
        CHK(chunk)->status = CHUNK_STATUS_ON;
        CHK(chunk)->size = CHK(chunk)->left = len;
        CHK(chunk)->offset = offset;
        CHK(chunk)->ndata = 0;
        strcpy(CHK(chunk)->filename, file);
        return 0;
    }
    return -1;
}

/* reading to chunk */
int chunk_read(void *chunk, int fd)
{
    int n = -1;

    if(chunk && fd > 0 && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end
            && (n = read(fd, CHK(chunk)->end, CHK(chunk)->left)) > 0)
    {
        CHK(chunk)->left -= n;
        CHK(chunk)->end += n;
        CHK(chunk)->ndata += n;
        if(CHK(chunk)->left <= 0) 
            CHK(chunk)->status = CHUNK_STATUS_OVER;
    }
    return n;
}

/* reading to chunk with SSL */
int chunk_read_SSL(void *chunk, void *ssl)
{
    int n = 1;
#ifdef HAVE_SSL
    if(chunk && ssl && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end
        && (n = SSL_read(XSSL(ssl), CHK(chunk)->end, CHK(chunk)->left)) > 0)
    {
        CHK(chunk)->left -= n;
        CHK(chunk)->end += n;
        CHK(chunk)->ndata += n;
        if(CHK(chunk)->left <= 0) 
            CHK(chunk)->status = CHUNK_STATUS_OVER;
    }
#endif
    return n;
}

/* writting from chunk */
int chunk_write(void *chunk, int fd)
{
    int n = -1;

    if(chunk && fd > 0 && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end
            && (n = write(fd, CHK(chunk)->end, CHK(chunk)->left)) > 0)
    {
        CHK(chunk)->left -= n;
        CHK(chunk)->end += n;
    }
    return n;
}

/* writting from chunk with SSL */
int chunk_write_SSL(void *chunk, void *ssl)
{
    int n = -1;
#ifdef HAVE_SSL
    if(chunk && ssl && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end
            && (n = SSL_write(XSSL(ssl), CHK(chunk)->end, CHK(chunk)->left)) > 0)
    {
        CHK(chunk)->left -= n;
        CHK(chunk)->end += n;
    }
#endif
    return n;
}

/* fill chunk memory */
int chunk_mem_fill(void *chunk, void *data, int ndata)
{
    int n = 0;

    if(chunk && data && ndata > 0 && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end)
    {
        n = ndata;
        if(CHK(chunk)->left < n) n = CHK(chunk)->left;
        memcpy(CHK(chunk)->end, data, n);
        CHK(chunk)->end += n;
        CHK(chunk)->left -= n;
        CHK(chunk)->ndata += n;
        if(CHK(chunk)->left <= 0) CHK(chunk)->status = CHUNK_STATUS_OVER;
    }
    return n;
}

/* copy to chunk */
int chunk_mem_copy(void *chunk, void *data, int ndata)
{
    int n = 0;

    if(chunk && data && ndata > 0 && CHK(chunk)->left > 0 && CHK(chunk)->data && CHK(chunk)->end)
    {
        n = ndata;
        if(CHK(chunk)->size > n) n = CHK(chunk)->size;
        memcpy(CHK(chunk)->end, data, n);
    }
    return n;
}

/* check chunk file */
int chunk_file_check(void *chunk)
{
    int fd = -1;

    if(chunk && CHK(chunk)->fd <= 0)
    {
        fd = CHK(chunk)->fd = open(CHK(chunk)->filename, O_RDONLY);
    }
    return fd;
}

/* check file left */
int chunk_file_left(void *chunk)
{
    int n = 0;

    if(chunk)
    {
        if(CHK(chunk)->left > CHK(chunk)->bsize) 
            n = CHK(chunk)->bsize;
        else 
            n = CHK(chunk)->left;
    }
    return n;
}

/* read data to file from fd */
int chunk_read_to_file(void *chunk, int fd)
{
    int ret = -1, n = 0, left = 0;

    if(chunk && fd > 0 && CHK(chunk)->data && CHK(chunk)->left > 0)
    {
        left = CHK(chunk)->bsize;
        if(CHK(chunk)->left < left) left = CHK(chunk)->left;
        if(chunk_file_check(chunk) > 0 && (n = read(fd, CHK(chunk)->data, left)) > 0
                && lseek(CHK(chunk)->fd, CHK(chunk)->offset, SEEK_SET) >= 0 
                && write(CHK(chunk)->fd, CHK(chunk)->data,  n) > 0)
        {
            CHK(chunk)->offset += n;
            CHK(chunk)->left -= n; 
            CHK(chunk)->ndata += n;
            if(CHK(chunk)->left == 0)
            {
                CHK(chunk)->status = CHUNK_STATUS_OVER;
                close(CHK(chunk)->fd);
            }
            ret = n;
        }
    }
    return ret;
}


/* push data to file */
int chunk_read_to_file_SSL(void *chunk, void *ssl)
{
    int ret = -1, n = 0, left = 0;
#ifdef HAVE_SSL
    if(chunk && ssl && CHK(chunk)->data && CHK(chunk)->left > 0)
    {
        left = CHK(chunk)->bsize;
        if(CHK(chunk)->left < left) left = CHK(chunk)->left;
        if(chunk_file_check(chunk) > 0 && (n = SSL_read(XSSL(ssl), CHK(chunk)->data, left)) > 0
            && lseek(CHK(chunk)->fd, CHK(chunk)->offset, SEEK_SET) >= 0 
            && write(CHK(chunk)->fd, CHK(chunk)->data,  n) > 0)
        {
            CHK(chunk)->offset += n;
            CHK(chunk)->left -= n; 
            CHK(chunk)->ndata += n;
            if(CHK(chunk)->left == 0)
            {
                CHK(chunk)->status = CHUNK_STATUS_OVER;
                close(CHK(chunk)->fd);
            }
            ret = n;
        }
    }
#endif
    return ret;
}

/* munmap */
void chunk_munmap(void *chunk)
{
    if(chunk)
    {
        if(CHK(chunk)->mmap)munmap(CHK(chunk)->mmap, MMAP_CHUNK_SIZE);
        CHK(chunk)->mmap = NULL;
    }
    return ;
}

/* mmap */
char *chunk_mmap(void *chunk)
{
    off_t offset = 0, n = 0;
    char *data = NULL;

    if(chunk && CHK(chunk)->left > 0 && CHK(chunk)->offset >= 0)
    {
        if(CHK(chunk)->mmap) munmap(CHK(chunk)->mmap, MMAP_CHUNK_SIZE);
        offset = (CHK(chunk)->offset / MMAP_PAGE_SIZE) * MMAP_PAGE_SIZE;
        if((CHK(chunk)->mmap = (char *)mmap(NULL, MMAP_CHUNK_SIZE, PROT_READ, MAP_SHARED, 
                        CHK(chunk)->fd, offset)) && CHK(chunk)->mmap != (void *)-1)
        {
            n = CHK(chunk)->offset - offset;
            data = CHK(chunk)->mmap + n;
            CHK(chunk)->mmleft = MMAP_CHUNK_SIZE - n;
            if(CHK(chunk)->left < CHK(chunk)->mmleft) 
                CHK(chunk)->mmleft = CHK(chunk)->left;
        }
    }
    return data;
}

/* write from file */
int chunk_write_from_file(void *chunk, int fd)
{
    int ret = -1, n = 0;
    char *data = NULL;

    if(chunk && fd > 0 && CHK(chunk)->left > 0 && CHK(chunk)->data)
    {
        if(chunk_file_check(chunk) > 0 && (data = chunk_mmap(chunk))
                && (n = write(fd, data,  CHK(chunk)->mmleft)) > 0)
        {
            CHK(chunk)->offset += n;
            CHK(chunk)->left -= n; 
            chunk_munmap(chunk);
            if(CHK(chunk)->left == 0)
            {
                CHK(chunk)->status = CHUNK_STATUS_OVER;
                close(CHK(chunk)->fd);
            }
            ret = n;
        }
    }
    return ret;
}

/* write from file with SSL */
int chunk_write_from_file_SSL(void *chunk, void *ssl)
{
    int ret = -1, n = 0;
    char *data = NULL;
#ifdef HAVE_SSL
    if(chunk && ssl && CHK(chunk)->left > 0 && CHK(chunk)->data)
    {
        if(chunk_file_check(chunk) > 0 && (data = chunk_mmap(chunk))
                && (n = SSL_write(XSSL(ssl), data,  CHK(chunk)->mmleft)) > 0)
        {
            CHK(chunk)->offset += n;
            CHK(chunk)->left -= n; 
            chunk_munmap(chunk);
            if(CHK(chunk)->left == 0)
            {
                CHK(chunk)->status = CHUNK_STATUS_OVER;
                close(CHK(chunk)->fd);
            }
            ret = n;
        }
    }
#endif
    return ret;
}

/* chunk file fill */
int chunk_file_fill(void *chunk, char *data, int ndata)
{
    int ret = -1, n = 0;

    if(chunk && data && ndata > 0 && CHK(chunk)->left > 0)
    {
        n = ndata;
        if(CHK(chunk)->left < n) n = CHK(chunk)->left;
        if(chunk_file_check(chunk) > 0 && lseek(CHK(chunk)->fd, CHK(chunk)->offset, SEEK_SET) >= 0 
                && write(CHK(chunk)->fd, data,  n) > 0)
        {
            CHK(chunk)->offset += n;
            CHK(chunk)->left -= n; 
            if(CHK(chunk)->left == 0)
            {
                CHK(chunk)->status = CHUNK_STATUS_OVER;
                close(CHK(chunk)->fd);
            }
            ret = n;
        }
    }
    return ret;
}

/* chunk reset */
void chunk_reset(void *chunk)
{
    if(chunk)
    {
        if(CHK(chunk)->mmap) munmap(CHK(chunk)->mmap, MMAP_CHUNK_SIZE);
        CHK(chunk)->mmap = NULL;
        CHK(chunk)->mmleft = 0;
        if(CHK(chunk)->fd > 0) close(CHK(chunk)->fd);
        CHK(chunk)->fd = 0;
        CHK(chunk)->status = 0;
        CHK(chunk)->type = 0;
        CHK(chunk)->ndata = 0;
        CHK(chunk)->left = 0;
        CHK(chunk)->offset = 0;
        if(CHK(chunk)->bsize > CHUNK_BLOCK_MAX)
        {
            if(CHK(chunk)->data) free(CHK(chunk)->data);
            CHK(chunk)->data = NULL;
            CHK(chunk)->bsize = 0;
        }
        CHK(chunk)->end = CHK(chunk)->data;
    }
    return ;
}

/* clean chunk */
void chunk_clean(void *chunk)
{
    if(chunk)
    {
        if(CHK(chunk)->mmap) munmap(CHK(chunk)->mmap, MMAP_CHUNK_SIZE);
        if(CHK(chunk)->data) free(CHK(chunk)->data);
        if(CHK(chunk)->fd > 0) close(CHK(chunk)->fd);
        free(chunk);
    }
    return ;
}
