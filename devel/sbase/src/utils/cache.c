#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "cache.h"
#include "timer.h"
#include "md5.h"
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
		fprintf(stderr, "Usage:%s homedir dbname (dictfile) \n", argv[0]);
		_exit(-1);
	}
	char *home = argv[1];
	char *dbname = argv[2];
	char *dict = (argc > 3) ? argv[3] :  NULL;
	FILE  *fp = NULL;
	char *s = NULL;
	int ns = 0;
	int nbuf = 256;
	char buf[nbuf];
	TIMER *timer = timer_init();
	INODE inode;
	DB_CACHE *cache = db_cache_init(home, dbname, 0, 0, stderr);	
	if(cache)
	{
		timer->reset(timer);
		cache->resume(cache);	
		timer->sample(timer);
		fprintf(stdout, "Resume Time Used:%llu\n", timer->usec_used);
		if(dict && (fp = fopen(dict, "r")))
		{
			while((s = fgets(buf, nbuf, fp)))
			{
				memset(&inode, 0, sizeof(INODE));
				ns = strlen(buf);
				buf[--ns] = 0;
				MD5(s, ns, inode.key);	
				//fprintf(stdout, "%s\n", s);
				///strcpy(inode.prop.kw, s);
				inode.size = (size_t)(ns + 1);
				cache->add(cache, &inode, s);
			}
			fclose(fp);
		}
		fprintf(stdout, "List cache ...\n");
		cache->list(cache, stdout);
		timer->reset(timer);
		fprintf(stdout, "Dump cache ...\n");
		cache->dump(cache);
		timer->sample(timer);
		fprintf(stdout, "Dump Time Used:%llu\n", timer->usec_used);
		fprintf(stdout, "Clean cache ...\n");
		cache->clean(&cache);
	}
}
