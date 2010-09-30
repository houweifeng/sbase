#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "chunk.h"
/* set/initialize chunk mem */
int chunk_init_mem(void *chunk, int len)
{
    int n = 0, size = 0;

    if(chunk)
    {
        if(len > CHK(chunk)->base_size)
        {
            n = len/CHUNK_BLOCK_SIZE;
            if(len % CHUNK_BLOCK_SIZE) ++n;
            size = n * CHUNK_BLOCK_SIZE;
            if(CHK(chunk)->data) free(CHK(chunk)->data);
            CHK(chunk)->data = NULL;
            if((CHK(chunk)->data = (char *)calloc(1, size)))
            {
                CHK(chunk)->left = CHK(chunk)->base_size = size;
            }
            else
            {
                CHK(chunk)->left = CHK(chunk)->base_size = 0;
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
int chunk_init_file(void *chunk, char *file, off_t offset, off_t len )
{

}

int chunk_mmap(void *chunk)
{
    if(CK_NMDATA(ptr) <= 0)
    {
        if(CK_MDATA(ptr))
        {
            fprintf(stdout, "%s::%d reset map[%p]\n", __FILE__, __LINE__, CK_MDATA(ptr));
            munmap(CK_MDATA(ptr), MMAP_CHUNK_SIZE);
	    CK_MDATA(ptr) = NULL;
        }
        if((CK_MDATA(ptr) = (char *)mmap(NULL, MMAP_CHUNK_SIZE, PROT_READ, MAP_PRIVATE, 
            CK_FD(ptr), (CK_OFFSET(ptr)/(off_t)MMAP_CHUNK_SIZE)*(off_t)MMAP_CHUNK_SIZE)) != (void *)-1)
        {
            CK_OFFMDATA(ptr) = (int)(CK_OFFSET(ptr)%(off_t)MMAP_CHUNK_SIZE);
            if(CK_LEFT(ptr) > MMAP_CHUNK_SIZE)
            {
                CK_NMDATA(ptr) = MMAP_CHUNK_SIZE - CK_OFFMDATA(ptr);
            }
            else 
            {
                CK_NMDATA(ptr) = CK_LEFT(ptr);
            }
            fprintf(stdout, "%s::%d mmap[%p] offset[%lld] off[%d] nmdata[%d]\n", __FILE__, __LINE__, 
                    CK_MDATA(ptr), (long long int)CK_OFFSET(ptr), CK_OFFMDATA(ptr), CK_NMDATA(ptr));
            return 1;
        }
        else 
        {
            fprintf(stdout, "%s::%d map[%p] failed, %s\n", __FILE__, __LINE__, CK_MDATA(ptr), strerror(errno));
            return 0;
        }
    }
    else  return 1;
}

