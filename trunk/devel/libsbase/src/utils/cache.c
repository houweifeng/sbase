#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "cache.h"
#include "timer.h"
#include "md5.h"
#define	BUF_SIZE	1024 * 1024 * 20
struct  _INODE;
typedef struct _IPRO
{
	unsigned char *word;
}IPRO;
CACHE_DEFINE(IBASE, INODE, IPRO, rbpname);
RB_PROTOTYPE(IBASE, _INODE, rbpname, compare);
CACHE_CODE(IBASE, INODE, IPRO);
RB_GENERATE(IBASE, _INODE, rbpname, compare);
int main(int argc, char **argv)
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage:%s homedir dbname (dictfile) (datafile) \n", argv[0]);
		_exit(-1);
	}
	char *home = argv[1];
	char *dbname = argv[2];
	char *dict = (argc > 3) ? argv[3] :  NULL;
	char *dfile = (argc > 4) ? argv[4] :  NULL;
	FILE  *fp = NULL;
	char *s = NULL;
	int ns = 0;
	int nbuf = 256;
	char buf[nbuf];
	char *p = (char *)calloc(1, BUF_SIZE);
	int np = 0;
	uint64_t total = 0, md5_total = 0;
	int n = 0;
	int fd = 0;

	TIMER *timer = timer_init();
	INODE inode;
	DB_CACHE *cache = db_cache_init(home, dbname, 0, 0, stderr);
	if(cache)
	{
		if(dfile && (fd = open(dfile, O_RDONLY)))
		{
			np = read(fd, p, BUF_SIZE);
			close(fd);
		}
		timer->reset(timer);
		cache->resume(cache);	
		timer->sample(timer);
		fprintf(stdout, "Resume Time Used:%llu\n", timer->usec_used);
		if(dict && (fp = fopen(dict, "r")))
		{
			while((s = fgets(buf, nbuf, fp)))
			{
				timer->reset(timer);
				memset(&inode, 0, sizeof(INODE));
				ns = strlen(buf);
				buf[--ns] = 0;
				MD5(s, ns, inode.key);	
				timer->sample(timer);
				fprintf(stdout, "MD5:%llu\n", timer->usec_used);
				md5_total += timer->usec_used;
				timer->reset(timer);
				//fprintf(stdout, "%s\n", s);
				///strcpy(inode.prop.kw, s);
				inode.size = (size_t)np;
				cache->add(cache, &inode, p);
				timer->sample(timer);
				fprintf(stdout, "CACHE:%llu\n", timer->usec_used);
				total += timer->usec_used;
				n++;	
			}
			fclose(fp);
		}
		if(n > 0)
		{
			fprintf(stdout, "MD5 TIME USED:%llu AVG:%llu\n", md5_total, (md5_total/n));
			fprintf(stdout, "CACHE TIME USED:%llu AVG:%llu\n", total, (total/n));
		}
		//fprintf(stdout, "List cache ...\n");
		//cache->list(cache, stdout);
		timer->reset(timer);
		fprintf(stdout, "Dump cache ...\n");
		cache->dump(cache);
		timer->sample(timer);
		fprintf(stdout, "Dump Time Used:%llu\n", timer->usec_used);
		fprintf(stdout, "Clean cache ...\n");
		cache->clean(&cache);
	}
}
