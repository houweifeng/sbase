#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "memb.h"
#ifdef HAVE_SSL
#include "xssl.h"
#endif
/* mem buffer initialize */
void *mmb_init(int block_size)
{
    void *mmb = NULL;
    if((mmb = calloc(1, sizeof(MEMB))))
    {
        MB(mmb)->block_size = MB_BLOCK_SIZE;
        if(block_size > MB_BLOCK_SIZE) 
            MB(mmb)->block_size = block_size;
    }
    return mmb;
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
            n = (size/MB(mmb)->block_size); 
            if(size % MB(mmb)->block_size) ++n;
            size = n * MB(mmb)->block_size;
            if(MB(mmb)->data) MB(mmb)->data = (char *)realloc(MB(mmb)->data, size);
            else MB(mmb)->data = (char *)calloc(1, size);
            if(MB(mmb)->data)
            {
                MB(mmb)->size = size;
                MB(mmb)->left = size - MB(mmb)->ndata;
                MB(mmb)->end = MB(mmb)->data + MB(mmb)->ndata;
            }
            else
            {
                MB(mmb)->size = 0;
                MB(mmb)->left = 0;
                MB(mmb)->end = NULL;
            }
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
            MB(mmb)->end = MB(mmb)->data;
            MB(mmb)->ndata = 0;
            MB(mmb)->left = MB(mmb)->size;
        }
        else
        {
            s =  MB(mmb)->data + MB(mmb)->ndata;
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
    if(mmb)
    {
        if(MB(mmb)->size > MB_BLOCK_SIZE)
        {
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
        if(MB(mmb)->data) free(MB(mmb)->data);
        free(mmb);
    }
    return ;
}

#ifdef _DEBUG_MEMB
#include <fcntl.h>
int main()
{
    int fd = 0, n = 0;
    void *mmb = NULL;
    char *s = NULL;

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
#endif
