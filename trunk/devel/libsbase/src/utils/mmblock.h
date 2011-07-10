#ifndef _MMBLOCK_H_
#define _MMBLOCK_H_
#define MMBLOCK_BITS    1024
typedef struct _MMBLOCK
{
   	char *data;
   	int  ndata;
  	int  size;
	int  left;
    int  bit;
	char *end;
}MMBLOCK;
#define  MMBLOCK_BASE 	    8192
//#define  MMBLOCK_BASE 	32768
//#define  MMBLOCK_BASE 	131072
//#define  MMBLOCK_BASE 	524288
#define  MMBLOCK_MIN 	    1024
#define  MMBLOCK_MAX 	    262144
//#define  MMBLOCK_MAX 	    1048576
/* initialize() */
MMBLOCK *mmblock_init();
/* recv() */
int mmblock_recv(MMBLOCK *mmblock, int fd, int flag);
/* read() */
int mmblock_read(MMBLOCK *mmblock, int fd);
/* SSL_read() */
int mmblock_read_SSL(MMBLOCK *mmblock, void *ssl);
/* push() */
int mmblock_push(MMBLOCK *mmblock, char *data, int ndata);
/* del() */
int mmblock_del(MMBLOCK *mmblock, int ndata);
/* reset() */
void mmblock_reset(MMBLOCK *mmblock);
/* clean() */
void mmblock_clean(MMBLOCK *mmblock);
#define MMB(x) ((MMBLOCK *)x)
#define MMB_NDATA(x) ((MMBLOCK *)x)->ndata
#define MMB_SIZE(x) ((MMBLOCK *)x)->size
#define MMB_LEFT(x) ((MMBLOCK *)x)->left
#define MMB_DATA(x) ((MMBLOCK *)x)->data
#define MMB_END(x) ((MMBLOCK *)x)->end
#define MMB_RECV(x, fd, flag) mmblock_recv(MMB(x), fd, flag)
#define MMB_READ(x, fd) mmblock_read(MMB(x), fd)
#define MMB_READ_SSL(x, ssl) mmblock_read_SSL(MMB(x), ssl)
#define MMB_PUSH(x, pdata, ndata) mmblock_push(MMB(x), pdata, ndata)
#define MMB_DELETE(x, ndata) mmblock_del(MMB(x), ndata)
#define MMB_RESET(x) mmblock_reset(MMB(x))
#define MMB_CLEAN(x) mmblock_clean(MMB(x))
#endif
