#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "memb.h"
#include "logger.h"
/* mem buffer initialize */
MEMB *mmb_init(int block_size)
{
    MEMB *mmb = NULL;
    if((mmb = (MEMB *)calloc(1, sizeof(MEMB))))
    {
        mmb->block_size = MB_BLOCK_SIZE;
        if(block_size > MB_BLOCK_SIZE) 
            mmb->block_size = block_size;
    }
    return mmb;
}

/* set logger */
void mmb_set_logger(void *mmb, void *logger)
{
    if(mmb)
    {
        MB(mmb)->logger = logger;
    }
    return ;
}

/* mem buffer set block size */
void mmb_set_block_size(void *mmb, int block_size)
{
    if(mmb) MB(mmb)->block_size = block_size;
    return ;
}

/* mem buffer increase */
int mmb_incre(void *mmb, int incre_size)
{
    int n = 0, size = 0;

    if(mmb && incre_size > 0 && MB(mmb)->block_size > 0)
    {
        size = MB(mmb)->ndata + incre_size;
        if(size > MB(mmb)->size) 
        {
            if(MB(mmb)->data)
            {
                n = size - MB(mmb)->size;
                MB(mmb)->size = size;
                MB(mmb)->left += n;
                MB(mmb)->end = MB(mmb)->data + MB(mmb)->ndata;

            }
            n = (size/MB(mmb)->block_size); 
            if(size % MB(mmb)->block_size) ++n;
            size = n * MB(mmb)->block_size;
            if(MB(mmb)->size > 0 && MB(mmb)->data == NULL)
            {
                ERROR_LOGGER(MB(mmb)->logger, "Invalid memory buffer[%p]->size:%d", mmb, MB(mmb)->size);
                _exit(-1);
            }
            //if(MB(mmb)->data) MB(mmb)->data = realloc(MB(mmb)->data, size);
            //else MB(mmb)->data = (char *)calloc(1, size);
            MB(mmb)->data = (char *)realloc(MB(mmb)->data, size);
            if(MB(mmb)->data)
            {
                n = size - MB(mmb)->size;
                MB(mmb)->size = size;
                MB(mmb)->left += n;
                MB(mmb)->end = MB(mmb)->data + MB(mmb)->ndata;
            }
            else
            {
                MB(mmb)->size = MB(mmb)->ndata = MB(mmb)->left = 0;
                MB(mmb)->data = MB(mmb)->end = NULL;
                n = 0;
            }
            ACCESS_LOGGER(MB(mmb)->logger, "mmb-incre buffer[%p]->data:%p/size:%d end:%p", mmb, MB(mmb)->data, MB(mmb)->size, MB(mmb)->end);
        }
    }
    return n;
}

/* mem buffer check */
void mmb_check(void *mmb)
{
    if(mmb && MB(mmb)->left <= 0 && MB(mmb)->block_size > 0)
    {
        mmb_incre(mmb, MB_BLOCK_SIZE); 
    }
    return ;
}

/* mem buffer receiving  */
int mmb_recv(void *mmb, int fd, int flag)
{
    int n = -1;

    if(mmb && fd > 0)
    {
        mmb_check(mmb);
        if(MB(mmb)->left > 0 && MB(mmb)->end && MB(mmb)->data 
                && (n = recv(fd, MB(mmb)->end, MB(mmb)->left, flag)) > 0)
        {
            MB(mmb)->end   += n;
            MB(mmb)->left  -= n;
            MB(mmb)->ndata += n;
        }
    }
    return n;
}

/* mem buffer reading  */
int mmb_read(void *mmb, int fd)
{
    int n = -1;

    if(mmb && fd > 0)
    {
        mmb_check(mmb);
        if(MB(mmb)->left > 0 && MB(mmb)->end && MB(mmb)->data 
                && (n = read(fd, MB(mmb)->end, MB(mmb)->left)) > 0)
        {
            MB(mmb)->end   += n;
            MB(mmb)->left  -= n;
            MB(mmb)->ndata += n;
        }
    }
    return n;
}

/* mem buffer reading with SSL */
int mmb_read_SSL(void *mmb, void *ssl)
{
    int n = -1;
#ifdef HAVE_SSL
    if(mmb && ssl)
    {
        mmb_check(mmb);
        if(MB(mmb)->left > 0 && MB(mmb)->end && MB(mmb)->data 
                && (n = SSL_read(XSSL(ssl), MB(mmb)->end, MB(mmb)->left)) > 0)
        {
            MB(mmb)->end   += n;
            MB(mmb)->left  -= n;
            MB(mmb)->ndata += n;
        }
    }
#endif
    return n;
}

/* push mem buffer */
int mmb_push(void *mmb, char *data, int ndata)
{
    if(mmb)
    {
        if(MB(mmb)->left < ndata) mmb_incre(mmb, ndata);
        if(MB(mmb)->left > ndata && MB(mmb)->data && MB(mmb)->end)
        {
            memcpy(MB(mmb)->end, data, ndata);
            MB(mmb)->end += ndata;
            MB(mmb)->ndata += ndata;
            MB(mmb)->left -= ndata;
            return ndata;
        }
        else
        {
            fprintf(stderr, "%s::%d mem_push(%p, %p, %d) failed, %s\n", __FILE__, __LINE__, mmb, data, ndata, strerror(errno));
        }
    }
    return -1;
}

/* delete mem buffer */
int mmb_del(void *mmb, int ndata)
{
    char *p = NULL, *s = NULL, *end = NULL;

    if(mmb && ndata > 0 &&  MB(mmb)->ndata > 0
            && (end = MB(mmb)->end) && (p = MB(mmb)->data))
    {
        if(ndata >= MB(mmb)->ndata)
        {
            if(ndata > MB(mmb)->ndata) fprintf(stderr, "%s::%d Invalid buffer[%p]->ndata:%d need:%d size:%d\n", __FILE__, __LINE__, mmb, MB(mmb)->ndata, ndata, MB(mmb)->size);
            MB(mmb)->end = MB(mmb)->data;
            MB(mmb)->ndata = 0;
            MB(mmb)->left = MB(mmb)->size;
        }
        else
        {
            s =  MB(mmb)->data + ndata;
            while(s < end)
            {
                *p++ = *s++;
            }
            MB(mmb)->end = p;
            MB(mmb)->ndata -= ndata;
            MB(mmb)->left += ndata;
        }
        return 0;
    }
    return -1;
}

/* reset mem buffer */
#ifdef _SBASE_MIN_MM_
void mmb_reset(void *mmb)
{
    if(mmb)
    {
        if(MB(mmb)->data) free(MB(mmb)->data);
        MB(mmb)->size = MB(mmb)->left = MB(mmb)->ndata = 0;
        MB(mmb)->end = MB(mmb)->data = NULL;
    }
    return ;
}
#else
void mmb_reset(void *mmb)
{
    if(mmb && MB(mmb)->size > 0)
    {
        if(MB(mmb)->size > MB_BLOCK_SIZE)
        {
            //fprintf(stderr, "reset buffer[%p]->size:%d ndata:%d\n", mmb, MB(mmb)->size, MB(mmb)->ndata);
            if(MB(mmb)->data) free(MB(mmb)->data);
            MB(mmb)->size = MB(mmb)->left = MB(mmb)->ndata = 0;
            MB(mmb)->end = MB(mmb)->data = NULL;
        }
        else
        {
            MB(mmb)->ndata = 0;
            MB(mmb)->end = MB(mmb)->data;
            MB(mmb)->left = MB(mmb)->size;
        }
    }
    return ;
}
#endif

/* clean mem buffer */
void mmb_clean(void *mmb)
{
    if(mmb)
    {
        if(MB(mmb)->data) 
        {
            ACCESS_LOGGER(MB(mmb)->logger, "mmb-clean buffer[%p]->data:%p/size:%d end:%p", mmb, MB(mmb)->data, MB(mmb)->size, MB(mmb)->end);
            MB(mmb)->ndata = MB(mmb)->size = MB(mmb)->left = 0;
            free(MB(mmb)->data);
            MB(mmb)->data = NULL;
        }
        free(mmb);
    }
    return ;
}

#ifdef _DEBUG_MEMB
#include <fcntl.h>
int main()
{
    int fd = 0, i = 0, n = 0;
    void *mmb = NULL, *logger = NULL;
    char *s = NULL, *logfile = "/tmp/log.txt";
    MEMB *list[20480];

    if((fd = open("/tmp/memb.text", O_CREAT|O_RDWR, 0644)) > 0)
    {
        LOGGER_INIT(logger, logfile);
        s = "asmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
        n = strlen(s);
        write(fd, s, n);
        write(fd, s, n);
        write(fd, s, n);
        write(fd, s, n);
        fprintf(stdout, "Ready for initialize list sizeof(MEMB):%d\n", sizeof(MEMB));
        for(i = 0; i < 20480; i++)
        {
            if((list[i] = (MEMB *)mmb_init(MB_BLOCK_SIZE)))
            {
                mmb_set_logger(list[i], logger);
                lseek(fd, 0, SEEK_SET);
                n = mmb_read(list[i], fd);
                ACCESS_LOGGER(logger, "read %d  to list[%d][%p]\n", n, i, list[i]);
                //mmb_push(list[i], s, n);
                //mmb_del(list[i], n);
            }
        }
        fprintf(stdout, "Ready for clean list\n");
        for(i = 0; i < 20480; i++)
        {
            if(list[i])
            {
                mmb_clean(list[i]);
                ACCESS_LOGGER(logger, "clean list[%d][%p]\n", i, list[i]);
            }
        }
        fprintf(stdout, "over-clean list\n");
        close(fd);
        LOGGER_CLEAN(logger);
    }
    while(1) sleep(1);
    return 0;
    if((mmb = mmb_init(MB_BLOCK_SIZE)))
    {
        if((fd = open("/tmp/memb.text", O_CREAT|O_RDWR, 0644)) > 0)
        {
            s = "asmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "bsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "csmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "dsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "esmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "fsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "gsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "hsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "ismafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "jsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "ksmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "lsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "msmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "nsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "osmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "psmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "qsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "rsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "ssmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "tsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "usmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "vsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "wsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "xsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "ysmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            s = "zsmafjkldsfjkdsfjklsdfjklasdfjlkadsjflkdklfafr\n";
            n = strlen(s);write(fd, s, n);mmb_push(mmb, s, n);
            fprintf(stdout, "----------1---------\n%s\n", MB_DATA(mmb));
            mmb_reset(mmb);
            fprintf(stdout, "----------2---------\n%s\n", MB_DATA(mmb));
            lseek(fd, 0, SEEK_SET);
            mmb_read(mmb, fd);
            fprintf(stdout, "----------3---------\n%s\n", MB_DATA(mmb));
        }
        mmb_clean(mmb);
    }
    return ;
}
//gcc -o mb memb.c -D_DEBUG_MEMB && ./mb
#endif
