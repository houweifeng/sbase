#include <unistd.h>
#include "chunk.h"
int ck_mmap(void *ptr)
{
    if(CK_NMDATA(ptr) <= 0)
    {
        if(CK_MDATA(ptr))
        {
            fprintf(stdout, "%s::%d reset map[%p]\n", __FILE__, __LINE__, CK_MDATA(ptr));
            munmap(CK_MDATA(ptr), CK_BSIZE(ptr));
        }
        if((CK_MDATA(ptr) = (char *)mmap(NULL, CK_BSIZE(ptr), PROT_READ, MAP_PRIVATE, 
            CK_FD(ptr), (CK_OFFSET(ptr)/(off_t)CK_BSIZE(ptr))*(off_t)CK_BSIZE(ptr))) != (void *)-1)
        {
            CK_OFFMDATA(ptr) = (int)(CK_OFFSET(ptr)%(off_t)CK_BSIZE(ptr));
            if(CK_LEFT(ptr) > CK_BSIZE(ptr))
            {
                CK_NMDATA(ptr) = CK_BSIZE(ptr) - CK_OFFMDATA(ptr);
            }
            else 
            {
                CK_NMDATA(ptr) = CK_LEFT(ptr);
            }
            fprintf(stdout, "%s::%d mmap[%p] off[%d] nmdata[%d]\n", __FILE__, __LINE__, 
                    CK_MDATA(ptr), CK_OFFMDATA(ptr), CK_NMDATA(ptr));
            return 1;
        }
        else 
        {
            fprintf(stdout, "%s::%d map[%p] failed\n", __FILE__, __LINE__, CK_MDATA(ptr));
            return 0;
        }
    }
    else  return 1;
}

