#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "chunk.h"
int ck_mmap(void *ptr)
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

