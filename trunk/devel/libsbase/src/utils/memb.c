#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "memb.h"
#define  BLOCK_SIZE 1024
#ifdef _DEBUG_MEMB
int main(int argc, char **argv)
{
    int i = 0;
    char s[1024], c = 0;
    void *buf =  NULL;
    int n = 0;
    int fd = 0;
    
    MB_INIT(buf, BLOCK_SIZE);
    if(buf)
    {
        for(i = 0; i < 1024; i++) 
        {
           c = 'a' + (i%26); 
           MB_PUSH(buf, &c, 1); 
        }
        if((fd = open("/tmp/abc.txt", O_CREAT|O_RDONLY, 0644)) > 0)
        {
            n = MB_READ(buf, fd);
            close(fd);
        }
        MB_STREND(buf);
        MB_DEL(buf, 2);
        fprintf(stdout, "n:%d size:%d left:%d ndata:%d %s\n", 
                n, MB_SIZE(buf), MB_LEFT(buf), MB_NDATA(buf), MB_DATA(buf));
        MB_CLEAN(buf);
    }
}
#endif
