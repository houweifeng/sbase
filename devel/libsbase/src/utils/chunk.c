#include <stdio.h>
#include <fcntl.h>
#include "chunk.h"
#ifdef _DEBUG_CHUNK
const char *str="dslfasd;lfl;sdfl;sdfl;ds;dfl;dskfa;ldskfl;dskfl;dkfl;dkfl;dfl;dskfl;dskf;d\n";
int main(int argc, char **argv)
{
    void *chunk = NULL;
    int fd = -1, tmpfd = -1, n = 0;
    char *file = "/tmp/chunk.txt", *tempfile = "/tmp/temp.txt";

    CK_INIT(chunk);
    /* mem chunk push */
    CK_MEM(chunk, 1024);
    CK_MEM_COPY(chunk, str, strlen(str));
    if((fd = open(file, O_CREAT|O_RDWR, 0644)) > 0)
    {
        if((n = CK_WRITE(chunk, fd)) > 0)
        {
            fprintf(stdout, "Wrote %d to file[%s] left:%lld status:%d\n", 
                    n, file, CK_LEFT(chunk), CHUNK_STATUS(chunk));
        }
        close(fd);
    }
    /* mem chunk read */
    CK_RESET(chunk);
    CK_MEM(chunk, 1024);
    if((fd = open(file, O_CREAT|O_RDWR, 0644)) > 0)
    {
        if((n = CK_READ(chunk, fd)) > 0)
            fprintf(stdout, "Read %d from file[%s] left:%lld status:%d\n", 
                    n, file, CK_LEFT(chunk), CHUNK_STATUS(chunk));
        close(fd);
    }
    /* mem chunk fill */
    CK_RESET(chunk);
    n = strlen(str);
    CK_MEM(chunk, n);
    if((n = CK_MEM_FILL(chunk, str, n - 10)) > 0)
    {
        fprintf(stdout, "Filled %d bytes from %d block left %lld status %d\n", 
                n, strlen(str), CK_LEFT(chunk), CHUNK_STATUS(chunk));
    }

    CK_FILE(chunk, tempfile, 32, 32);
    /* file chunk read */
    if((fd = open(file, O_CREAT|O_RDONLY, 0644)) > 0)
    {
        fprintf(stdout, "Ready from read to file:%s\n",file);
        if((n = CK_READ_TO_FILE(chunk, fd)) > 0)
        {
            fprintf(stdout, "Read %d bytes from file %s to file %s left:%lld status:%d\n", 
                    n, file, tempfile, CK_LEFT(chunk), CHUNK_STATUS(chunk));
        }
        close(fd);
    }
    CK_RESET(chunk);
    CK_FILE(chunk, file, 0, 32);
    /* file chunk write */
    if((fd = open(tempfile, O_CREAT|O_WRONLY, 0644)) > 0)
    {
        fprintf(stdout, "Ready write from file:%s\n", file);
        if((n = CK_WRITE_FROM_FILE(chunk, fd)) > 0)
        {
            fprintf(stdout, "Wrote %d bytes from file %s to file %s left:%lld status:%d\n", 
                    n, file, tempfile, CK_LEFT(chunk), CHUNK_STATUS(chunk));
        }
        close(fd);
    }
    /* fill to chunk file from buffer */
    n = strlen(str);
    CK_RESET(chunk);
    CK_FILE(chunk, tempfile, 128, n);
    fprintf(stdout, "Ready  fill file:%s %d bytes\n", tempfile, n);
    if((n = CK_FILE_FILL(chunk, str, n)) > 0)
    {
        fprintf(stdout, "Filled %d bytes to file:%s left:%lld status:%d\n",
                n, tempfile, CK_LEFT(chunk), CHUNK_STATUS(chunk));
    }
    CK_CLEAN(chunk); 
    return 0;
}
#endif

