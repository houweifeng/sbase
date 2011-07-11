#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "xmm.h"
#define MM_PAGESIZE 4096
#define MMSIZE(xxx) (((xxx/MM_PAGESIZE) + ((xxx%MM_PAGESIZE) != 0)) * MM_PAGESIZE)
/* new memory */
void *xmm_new(size_t size)
{
    void *m = NULL;
    if(size > 0)
    {
        /*
        if(size < MM_PAGESIZE)
            m = calloc(1, size);
        else
        {
        */
            m = mmap(NULL, MMSIZE(size), PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
            if(m == (void *)-1) m = NULL;
        //}
    }
    return m;
}
void *xmm_mnew(size_t size)
{
    void *m = NULL;
    if(size > 0)
    {
        /*
        if(size < MM_PAGESIZE)
            m = calloc(1, size);
        else
        {
        */
            m = mmap(NULL, MMSIZE(size), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
            if(m == (void *)-1) m = NULL;
        //}
    }
    return m;
}

/* resize */
void *xmm_resize(void *old, size_t old_size, size_t new_size)
{
    void *m = NULL;
    if(new_size > 0 && new_size > old_size)
    {
        /*
        if(new_size < MM_PAGESIZE)
        {
            m = calloc(1, new_size);
        }
        else
        {
        */
            m = mmap(NULL, MMSIZE(new_size), PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
            if(m == (void *)-1) m = NULL;
        //}
        if(old)
        {
            if(old_size > 0)
            {
                if(m) memcpy(m, old, old_size);
                //if(old_size < MM_PAGESIZE) free(old);
                //else 
                munmap(old, MMSIZE(old_size));
            }
        }
    }
    return m;
}

/* resize */
void *xmm_mresize(void *old, size_t old_size, size_t new_size)
{
    void *m = NULL;
    if(new_size > 0 && new_size > old_size)
    {
        /*
        if(new_size < MM_PAGESIZE)
        {
            m = calloc(1, new_size);
        }
        else
        {
        */
            m = mmap(NULL, MMSIZE(new_size), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
            if(m == (void *)-1) m = NULL;
        //}
        if(old)
        {
            if(old_size > 0)
            {
                if(m) memcpy(m, old, old_size);
                //if(old_size < MM_PAGESIZE) free(old);
                //else 
                    munmap(old, MMSIZE(old_size));
            }
        }
    }
    return m;
}

/* remalloc */
void *xmm_renew(void *old, size_t old_size, size_t new_size)
{
    xmm_free(old, old_size);
    return xmm_new(new_size);
}

/* remalloc */
void *xmm_mrenew(void *old, size_t old_size, size_t new_size)
{
    xmm_free(old, old_size);
    return xmm_mnew(new_size);
}


/* free memory */
void xmm_free(void *m, size_t size)
{
    if(m && size > 0)
    {
        //if(size < MM_PAGESIZE) free(m);
        //else 
        munmap(m, MMSIZE(size));
    }
    return ;
}
