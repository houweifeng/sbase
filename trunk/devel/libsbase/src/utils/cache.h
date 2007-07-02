#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <db.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "tree.h"
#ifndef _DB_CACHE_H
#define _DB_CACHE_H
#ifndef FILE_MAX
#define FILE_MAX 256
#define DB_NAME_MAX  64
#endif
#define ENV_DIR  "env/"
#define DATA_DIR "data/"
#define CACHE_EXT ".cache"
#define IDX_DIR  "idx/"
#define IDX_EXT	 ".idx"
#define DEF_CACHE_SIZE  33554432
#define DEF_PAGE_SIZE	1024
#define CACHE_DEFINE(base, name, type, ptrname)                          			\
struct _##name;											\
typedef struct  _##name 									\
{                                								\
                unsigned char key[16];                  					\
                type prop;                               					\
                size_t size;									\
		RB_ENTRY ( _##name ) ptrname;							\
}name;                                								\
typedef struct _DB_CACHE									\
{												\
	DB_ENV  *dbenv;										\
	char	cachefile[FILE_MAX];								\
	char	idxfile[FILE_MAX];								\
	size_t	count ;										\
	DB	*db;										\
	RB_HEAD(base, _##name) index;								\
												\
	int  (*add)(struct _DB_CACHE *, name *, void *);					\
	int  (*get)(struct _DB_CACHE *, name *, void **);					\
	int  (*update)(struct _DB_CACHE *, name *, void *);					\
	int  (*del)(struct _DB_CACHE *, name *);						\
	int  (*resume)(struct _DB_CACHE *);							\
	int  (*dump)(struct _DB_CACHE *);							\
	int  (*list)(struct _DB_CACHE *, FILE *);						\
	void (*clean)(struct _DB_CACHE **);							\
}DB_CACHE;											\
name *name##_INIT();                             						\
void  name##_CLEAN(name **);                 							\
/* DB_CACHE Initialize */									\
DB_CACHE *db_cache_init(const char *home, const char *db,					\
	size_t cache_size, size_t page_size, FILE *);						\
/* Add key/value to DB_CACHE */									\
int db_cache_add(DB_CACHE *, name *, void *);							\
/* Get data */											\
int db_cache_get(DB_CACHE *, name *, void **);							\
/* Update data */										\
int db_cache_update(DB_CACHE *, name *, void *);						\
/* Delete data */										\
int db_cache_del(DB_CACHE *, name *);								\
/* Resume cache */										\
int db_cache_resume(DB_CACHE *);								\
/* Dump cache */										\
int db_cache_dump(DB_CACHE *);									\
/* List cache */										\
int db_cache_list(DB_CACHE *, FILE *);								\
/* Clean cache */										\
void db_cache_clean(DB_CACHE **);								\
/* NODE compare */  										\
int compare(name *p, name *p1);

#define CACHE_CODE(base, name, type)                           					\
name *name##_INIT()                              						\
{                                                       					\
       return (name *)calloc(1, sizeof(name));   						\
}                                                       					\
void name##_CLEAN(name **node)                  						\
{                                                       					\
        if(*node)                                       					\
        {                                               					\
                free(*node);                            					\
                *node = NULL;                           					\
        }                                               					\
}												\
/* DB_CACHE Initialize */									\
DB_CACHE *db_cache_init(const char *home, const char *db,					\
	 size_t cache_size, size_t page_size, FILE *errout)					\
{												\
	char data_dir[FILE_MAX];								\
	char env_dir[FILE_MAX];									\
	size_t cachesize = (cache_size > 0 ) ? cache_size : DEF_CACHE_SIZE;			\
	size_t pagesize	 = (page_size > 0 ) ? page_size : DEF_PAGE_SIZE;			\
	DB_CACHE *cache = (DB_CACHE *)calloc(1, sizeof(DB_CACHE));				\
	if(cache)										\
	{											\
		RB_INIT(&(cache->index));							\
		cache->add 	= db_cache_add;							\
		cache->get 	= db_cache_get;							\
		cache->update 	= db_cache_update;						\
		cache->del 	= db_cache_del;							\
		cache->resume 	= db_cache_resume;						\
		cache->list 	= db_cache_list;						\
		cache->dump 	= db_cache_dump;						\
		cache->clean 	= db_cache_clean;						\
		sprintf(data_dir, "%s/%s/", home, DATA_DIR);					\
		sprintf(env_dir, "%s/%s/", home, ENV_DIR);					\
		sprintf(cache->cachefile, "%s/%s/%s%s", home, DATA_DIR, db, CACHE_EXT);		\
		sprintf(cache->idxfile, "%s/%s/%s%s", home, IDX_DIR, db, IDX_EXT);		\
		/* Create db_env */								\
		if(db_env_create(&(cache->dbenv), 0) != 0)					\
		{										\
			fprintf(errout, "Initialize DB_ENV failed, %s", strerror(errno));	\
			goto err_end;								\
		}										\
		/* Set db_env cache size */							\
		if(cache->dbenv->set_cachesize(cache->dbenv, 0, cachesize, 0) != 0)		\
		{										\
			fprintf(errout, "Initlize DB_ENV set_cachesize failed,			\
				%s", strerror(errno));						\
			goto err_end;								\
		}										\
		/* Set data dir */								\
		cache->dbenv->set_data_dir(cache->dbenv, data_dir);				\
		/* Open db_env */								\
		if(cache->dbenv->open(cache->dbenv, env_dir,					\
					DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL , 0) != 0)	\
		{										\
			fprintf(errout, "Open db_env[%s] failed, %s",				\
				 env_dir, strerror(errno));					\
			goto err_end;								\
		}										\
		/* Initialize db */								\
		if(db_create(&(cache->db), cache->dbenv, 0) != 0 ) 				\
		{										\
			fprintf(errout, "Initialize database failed, %s", strerror(errno));	\
			goto err_end;								\
		}										\
		/* Set DB page size*/								\
		cache->db->set_pagesize(cache->db, pagesize);					\
		/* Open DB */									\
		if(cache->db->open(cache->db, NULL, cache->cachefile,				\
					NULL, DB_BTREE, DB_CREATE, 0644) != 0 )			\
		{										\
			fprintf(errout, "Open DATABASE:%s failed, %s",				\
					cache->cachefile, strerror(errno));			\
			goto err_end;								\
		}										\
		return cache;									\
	}											\
err_end:											\
	if(cache && cache->db) cache->db->close(cache->db, 0);					\
	if(cache && cache->dbenv) cache->dbenv->close(cache->dbenv, 0);				\
	if(cache) free(cache);									\
        return NULL;										\
}												\
/* Add key/value to DB_CACHE */									\
int db_cache_add(DB_CACHE *cache, name *s_node, void *data)					\
{												\
	DBT k, v;										\
	name *node = NULL;									\
	if(cache && s_node)									\
	{											\
		if(node = RB_FIND(base, &(cache->index), s_node)) return 0;			\
		if((node = name##_INIT()))							\
		{										\
			memcpy(node, s_node, sizeof( name));					\
			RB_INSERT(base, &(cache->index), node);					\
			memset(&k, 0, sizeof(DBT));						\
			k.data = node->key;							\
			k.size = _MD5_N;							\
			memset(&v, 0, sizeof(DBT));						\
			v.data = data;								\
			v.size = node->size;							\
			return cache->db->put(cache->db,					\
				NULL, &k, &v, DB_NOOVERWRITE);					\
		}										\
	}											\
	return -1;										\
}												\
/* Get data */											\
int db_cache_get(DB_CACHE *cache, name *node, void **data)					\
{												\
	name *p = NULL;										\
	DBT k, v;										\
	if(cache && node)									\
	{											\
		if((p = RB_FIND(base, &(cache->index), node)))					\
		{										\
			memcpy(node, p, sizeof(name));						\
			memset(&k, 0, sizeof(DBT));						\
			k.data = node->key;							\
                        k.size = _MD5_N;							\
			memset(&v, 0, sizeof(DBT));						\
			if(cache->db->get(cache->db, NULL, &k, &v, 0) == 0 )			\
			{									\
				(*data) = v.data;						\
				return 0;							\
			}									\
		}										\
	}											\
	return -1;										\
}												\
/* Update data */										\
int db_cache_update(DB_CACHE *cache, name *node, void *data)					\
{												\
	name *p = NULL;										\
        DBT k, v;										\
        if(cache && node)									\
        {											\
               if(p = RB_FIND(base, &(cache->index), node))					\
                {										\
			p->size = node->size;							\
			memcpy(&(p->prop), &(node->prop), sizeof(type));			\
                        memset(&k, 0, sizeof(DBT));						\
                        k.data = node->key;							\
                        k.size = _MD5_N;							\
                        memset(&v, 0, sizeof(DBT));						\
			v.data = data;								\
			v.size = node->size;							\
                        if(cache->db->del(cache->db, NULL, &k, 0) == 0 )			\
			{									\
				return cache->db->put(cache->db, NULL, &k, &v, 0);		\
			}									\
                }										\
        }											\
	return -1;										\
}												\
/* Delete data */										\
int db_cache_del(DB_CACHE *cache, name *node)							\
{												\
	name *p = NULL;										\
        DBT k;											\
        if(cache && node)									\
        {											\
                if(p = RB_FIND(base, &(cache->index), node))					\
                {										\
                        memset(&k, 0, sizeof(DBT));						\
                        k.data = node->key;							\
                        k.size = _MD5_N;							\
			cache->count++;								\
                        return cache->db->del(cache->db, NULL, &k, 0);				\
                }										\
        }											\
        return -1;										\
}												\
/* Resume cache */										\
int db_cache_resume(DB_CACHE *cache)								\
{												\
	int fd = 0;										\
	name node , *p = NULL;									\
	if(cache)										\
	{											\
		if((fd = open(cache->idxfile, O_RDONLY|O_CREAT, 0644)) > 0 )			\
		{										\
			while(read(fd, &node, sizeof(name)) == sizeof(name))			\
			{									\
				if((p = name##_INIT()))						\
				{								\
					MEMCPY(p, &node, sizeof(name));				\
					RB_INSERT(base, &(cache->index), p);			\
					cache->count++;						\
				}								\
			}									\
			close(fd);								\
			return 0;								\
		}										\
		else                                                                            \
                {                                                                               \
                        fprintf(stderr, "Open idxfile[%s] resume failed, %s\n",			\
                                 cache->idxfile, strerror(errno));                              \
                }										\
	}											\
	return -1;										\
}												\
/* Dump cache */										\
int db_cache_dump(DB_CACHE *cache)								\
{												\
	int fd = 0;										\
	name *p = NULL;										\
	if(cache)										\
	{											\
		if((fd = open(cache->idxfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) > 0 )	\
		{										\
			RB_FOREACH(p, base, &(cache->index))					\
			{									\
				if(p)write(fd, p, sizeof(name));				\
			}									\
			close(fd);								\
			return 0;								\
		}										\
		else										\
		{										\
			fprintf(stderr, "Open idxfile[%s] failed, %s\n",			\
				 cache->idxfile, strerror(errno));				\
		}										\
	}											\
	return -1;										\
}												\
/* List cache */										\
int db_cache_list(DB_CACHE *cache, FILE *pout)							\
{												\
	name *p = NULL;										\
	u_int32_t total = 0, total_size = 0;							\
	void *s = NULL;										\
	if(cache )										\
	{											\
		RB_FOREACH(p, base, &(cache->index))						\
		{										\
			if(p)									\
			{									\
				fprintf(pout, "key:");						\
				MD5OUT(p->key, pout);						\
				/* */if(cache->get(cache, p, &s) == 0 )/**/			\
				/* */{				/**/				\
				/* */	fprintf(pout, " %s\n", s);/**/				\
				/* */}				/**/				\
				total++;							\
				total_size += p->size;						\
			}									\
		}										\
		if(total) fprintf(pout, "TOTAL:%u SIZE:%u AVG:%u\n",				\
			total, total_size, (total_size / total));				\
	}											\
}												\
/* Clean cache */										\
void  db_cache_clean(DB_CACHE **cache)								\
{												\
	if(*cache)										\
	{											\
		free(*cache);									\
		*cache = NULL;									\
	}											\
}												\
/* NODE compare */										\
int compare(name *p, name *p1)									\
{												\
	int i = 0;										\
	if(p && p1)										\
	{											\
		while(i < _MD5_N) 								\
		{										\
			if(p->key[i] == p1->key[i]) i++;					\
			else return(p->key[i] - p1->key[i]);					\
		}										\
	}											\
	return 0;										\
}
#endif
