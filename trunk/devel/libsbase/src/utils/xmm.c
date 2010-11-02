#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
/* new memory */
void *xmm_new(size_t size)
{
    void *m = NULL;

#ifdef HAVE_MMAP
    if((m = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0)) && m != (void *)-1)
        memset(m, 0, size);
    else m = NULL;
#else
    m = calloc(1, size);
#endif
    return m;
}

/* remalloc */
void *xmm_renew(void *old, size_t old_size, size_t new_size)
{
    void *m = NULL;
    if(new_size > 0)
    {
#ifdef HAVE_MMAP
        if((m =  mmap(NULL, new_size, PROT_READ|PROT_WRITE,
                        MAP_ANON|MAP_PRIVATE, -1, 0)) && m != (void *)-1)
        {
            if(old) 
            {
                memcpy(m, old, old_size);
                munmap(old, old_size);
            }
        }
        else
        {
            munmap(old, old_size);
            m = NULL;
        }
#else
        m = realloc(old, new_size);
#endif
    }
    else 
    {
#ifdef HAVE_MMAP
        munmap(old, old_size);
#else
        free(old);
#endif
        m = NULL;
    }
    return m;
}

/* new shared memory */
void *xmms_new(size_t size)
{
    void *m = NULL;

#ifdef HAVE_MMAP
    if((m = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0)) && m != (void *)-1)
        memset(m, 0, size);
    else m = NULL;
#else
    m = calloc(1, size);
#endif
    return m;
}

/* remalloc shared memory */
void *xmms_renew(void *old, size_t old_size, size_t new_size)
{
    void *m = NULL;
    if(new_size > 0)
    {
#ifdef HAVE_MMAP
        if((m =  mmap(NULL, new_size, PROT_READ|PROT_WRITE,
                        MAP_ANON|MAP_SHARED, -1, 0)) && m != (void *)-1)
        {
            if(old) 
            {
                memcpy(m, old, old_size);
                munmap(old, old_size);
            }
        }
        else
        {
            munmap(old, old_size);
            m = NULL;
        }
#else
        m = realloc(old, new_size);
#endif
    }
    else 
    {
#ifdef HAVE_MMAP
        munmap(old, old_size);
#else
        free(old);
#endif
        m = NULL;
    }
    return m;
}

/* free memory */
void xmm_free(void *m, size_t size)
{
    if(m)
    {
#ifdef HAVE_MMAP
        munmap(m, size);
#else
        free(m);
#endif
    }
    return ;
}
