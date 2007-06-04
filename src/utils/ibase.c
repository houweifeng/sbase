#include "ibase.h"
#include "md5.h"
#include "timer.h"

RB_PROTOTYPE(ITABLE, _INODE, rbnode, ibase_compare);
RB_GENERATE(ITABLE, _INODE, rbnode, ibase_compare);
/* Initialize IBASE */
IBASE *ibase_init()
{
	IBASE *ibase = (IBASE *)calloc(1, sizeof(IBASE));	
	if(ibase)
	{
		RB_INIT(&(ibase->index));
		ibase->readdict		= ibase_readdict;
		ibase->addword		= ibase_addword;
		ibase->resume  		= ibase_resume;
		ibase->dump		= ibase_dump;
		ibase->list		= ibase_list;
		ibase->new_inode	= ibase_new_inode;
		ibase->clean_inode	= ibase_clean_inode;
		ibase->clean		= ibase_clean;
	}
}
/* Add word to Index Table */
int ibase_addword(IBASE *ibase, unsigned char *key, unsigned char *word, size_t nword)
{
	INODE inode;
	INODE *p = NULL;
	if(ibase && key && word && nword > 0)
	{
		memset(&inode, 0, sizeof(INODE));
		memcpy(inode.key, key, _MD5_N);
		//if(p = RB_FIND(ITABLE, &(ibase->index), &inode)) return 0;
		if((p = ibase->new_inode(ibase, key, word, nword)))
		{
			if(RB_INSERT(ITABLE, &(ibase->index), p))
			{
				fprintf(stdout, "ibase->count:%u inode[%02x%02x%02x%02x%02x%02x%02x"
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x][%s] is exists\n", ibase->count,
				p->key[0], p->key[1], p->key[2], p->key[3],
				p->key[4], p->key[5], p->key[6], p->key[7], 
				p->key[8], p->key[9], p->key[10], p->key[11],
				p->key[12], p->key[13], p->key[14], p->key[15], p->word);
				ibase->clean_inode(ibase, &p);
			}
			//MD5OUT(inode->key, stdout);
			
		}	
	}
}
/* Read word from Dict */
int ibase_readdict(IBASE *ibase, unsigned char *dictfile)
{
	unsigned char buf[WORD_MAX], key[_MD5_N], *s = NULL;
	int nbuf = WORD_MAX, ns = 0;
	FILE *fp = NULL;
	if(ibase)
	{
		if(dictfile && (fp = fopen(dictfile, "r")))
                {
                        while((s = fgets(buf, nbuf, fp)))
                        {
                                ns = strlen((char *)buf);
                                buf[--ns] = 0;
                                MD5(buf, ns, key);
                                ibase->addword(ibase, key, buf, ns);
                        }
                        fclose(fp);
			return 0;
                }
	}
	return -1;
}

/* Resume Index Table from dump file */
int ibase_resume(IBASE *ibase)
{
        int fd = 0;
        INDEX index ;
        int n = 0;
	char tmp[WORD_MAX];
        if(ibase && ibase->idxfile)
        {
                if((fd = open(ibase->idxfile, O_CREAT|O_RDONLY, 0644)) > 0)
                {
			while((read(fd, &index, sizeof(INDEX))))
			{
                                if(index.nword > 0 
					&& (n = read(fd, &tmp, index.nword)) == index.nword)
                                {
					ibase->addword(ibase, index.key, tmp, n);
                                }else break;
                        }
                        close(fd);
                        return 0;
                }
        }
	return -1;
}
/* Dump Index Table to dump file */
int ibase_dump(IBASE *ibase)
{
	INODE *p = NULL;
	int fd = 0;
	INDEX index ;
	int n = 0;
	if(ibase && ibase->idxfile)
	{
		if((fd = open(ibase->idxfile, O_CREAT|O_WRONLY, 0644)) > 0)
		{
			RB_FOREACH(p, ITABLE, &(ibase->index))
			{
				if(p && p->word)
				{
					memcpy(index.key, p->key, _MD5_N);
					index.nword = n = strlen((char *)(p->word));
					write(fd, &index, sizeof(INDEX));
					write(fd, p->word, n);
				} 
			}
			close(fd);
			return 0;
		}
	}
	return -1;
}
/* List Index Table */
int ibase_list(IBASE *ibase, FILE *pout)
{
	INODE *p = NULL;
	unsigned int total = 0;
	size_t size = 0;
	if(ibase)
	{
		RB_FOREACH(p, ITABLE, &(ibase->index))
		{
			if(p && p->word)
			{
				MD5OUT(p->key, pout);	
				fprintf(pout, " %s\n", p->word);
				total++;
				size += strlen((char *)(p->word));
			}	
		}
		if(total) fprintf(pout, "COUNT:%u MEM:%u TOTAL:%u SIZE:%u AVG:%f\n",
			ibase->count, ibase->size, total, size, (double) (size / total));
	}	
	return 0;
}
/* Clean IBASE */
void ibase_clean(IBASE **ibase)
{
	if((*ibase))
	{
		free((*ibase));
		(*ibase) = NULL;	
	}	
}
/* Compare 2 INODE */
int ibase_compare(INODE *p, INODE *p1)
{
	int i = 0;
	if(p && p1)	
	{
		//for(; i < _MD5_N; i += 4)
		for(i = 0; i < _MD5_N; i++)
		{
			//if( *((int *)&(p->key[i])) != *((int *)&(p1->key[i])) ) 
			if(p->key[i] != p1->key[i])
			{
				return (p->key[i] - p1->key[i]);
			}
		}
	}
	//fprintf(stdout, "node[%x]:node[%x]\n", _inode, inode);
	return 0;
}
/* Initialize NEW INODE in IBASE */
INODE *ibase_new_inode(IBASE *ibase, unsigned char *key, unsigned char *word, size_t nword)
{
	INODE *inode = (INODE *)calloc(1, sizeof(INODE));	
	if(ibase && inode)
	{
		inode->word = (unsigned char *)calloc(1, nword + 1);
		if(inode->word)
		{
			memcpy(inode->key, key, _MD5_N);
			memcpy(inode->word, word, nword);
			ibase->count++;
			ibase->size += sizeof(INODE);
			ibase->size += ((nword + 1) % 4) ? ((((nword + 1) / 4) + 1) * 4) : (nword + 1);
			return inode;
		}
	}
	if(inode)
	{
		if(inode->word) free(inode->word);
		free(inode);
	}
	return NULL;
}
/* Clean INODE in IBASE */
void ibase_clean_inode(IBASE *ibase, INODE **inode)
{
	int n = 0;
	if((*inode))
	{
		if((*inode)->word)
		{
			n = strlen((char *)((*inode)->word));
			if(ibase)ibase->size -= ((n + 1) % 4)? ((((n + 1) / 4) + 1) * 4) : (n + 1);
			free((*inode)->word);
		}
		if(ibase) ibase->size -= sizeof(INODE);
		free(*inode);
		*inode = NULL;
	}	
}

#ifdef _DEBUG_IBASE
int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage:%s idxfile (dictfile) \n", argv[0]);
		_exit(-1);
	}
	char *idxfile = argv[1];
	char *dictfile = (argc > 2) ? argv[2] :  NULL;
	FILE  *fp = NULL;
	char *s = NULL;
	int ns = 0;
	int nbuf = 256;
	char buf[nbuf];
	char key[_MD5_N];
	TIMER *timer = timer_init();
	IBASE *ibase = ibase_init();
	if(ibase)
	{
		ibase->idxfile = idxfile;
		timer->reset(timer);
		ibase->resume(ibase);	
		timer->sample(timer);
		fprintf(stdout, "Resume Time Used:%llu\n", timer->usec_used);
		if(dictfile)
		{
			ibase->readdict(ibase, dictfile);
		}
		fprintf(stdout, "ibase->count:%u ibase->size:%u\n", ibase->count, ibase->size);
		fprintf(stdout, "List cache ...\n");
		ibase->list(ibase, stdout);
		timer->reset(timer);
		fprintf(stdout, "Dump cache ...\n");
		ibase->dump(ibase);
		timer->sample(timer);
		fprintf(stdout, "Dump Time Used:%llu\n", timer->usec_used);
		fprintf(stdout, "Clean cache ...\n");
		ibase->clean(&ibase);
	}
}
#endif
