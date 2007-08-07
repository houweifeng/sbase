#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "db.h"
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
#define ENV_DIR  "env"
#define DATA_DIR "data"
#define CACHE_EXT ".cache"
#define IDX_DIR  "idx"
#define IDX_EXT	 ".idx"
#define DEF_CACHE_SIZE  33554432
#define DEF_PAGE_SIZE	1024
#define NKEY	16
#ifdef HAVE_PTHREAD
#define CACHE_MUTEX_INIT(mutex) if((mutex = calloc(1, sizeof(pthread_mutex_t)))) 		\
	pthread_mutex_init((pthread_mutex_t *)mutex, NULL);	
#define CACHE_MUTEX_LOCK(mutex) if(mutex) pthread_mutex_lock((pthread_mutex_t *)mutex);
#define CACHE_MUTEX_UNLOCK(mutex) if(mutex) pthread_mutex_unlock((pthread_mutex_t *)mutex);
#define CACHE_MUTEX_DESTROY(mutex) if(mutex) pthread_mutex_destroy((pthread_mutex_t *)mutex); 	\
	free(mutex); mutex = NULL;
#else 
#define CACHE_MUTEX_INIT(mutex)
#define CACHE_MUTEX_LOCK(mutex)
#define CACHE_MUTEX_UNLOCK(mutex)
#define CACHE_MUTEX_DESTROY(mutex)
#endif
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
	void    *mutex;										\
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
	size_t page_size, FILE *);								\
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
	size_t page_size, FILE *errout)								\
{												\
	size_t pagesize	 = (page_size > 0 ) ? page_size : DEF_PAGE_SIZE;			\
	int n = 0;										\
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
		CACHE_MUTEX_INIT(cache->mutex);							\
		sprintf(cache->cachefile, "%s/%s%s", home,  db, CACHE_EXT);			\
		sprintf(cache->idxfile, "%s/%s%s", home, db, IDX_EXT);				\
		/* Initialize db */								\
		if(db_create(&(cache->db), NULL, 0) != 0 ) 					\
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
	if(cache) free(cache);									\
        return NULL;										\
}												\
int db_cache_add(DB_CACHE *cache, name *s_node, void *data)					\
{												\
	DBT k, v;										\
	name *node = NULL;									\
	int ret = -1;										\
	if(cache && s_node)									\
	{											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
		if((node = RB_FIND(base, &(cache->index), s_node))){ret = 0;goto end;}		\
		if((node = name##_INIT()))							\
		{										\
			memcpy(node, s_node, sizeof( name));					\
			RB_INSERT(base, &(cache->index), node);					\
			memset(&k, 0, sizeof(DBT));						\
			k.data = node->key;							\
			k.size = NKEY;								\
			memset(&v, 0, sizeof(DBT));						\
			v.data = data;								\
			v.size = node->size;							\
			cache->count++;								\
			ret = cache->db->put(cache->db,						\
				NULL, &k, &v, DB_NOOVERWRITE);					\
			goto end;								\
		}										\
end :												\
		CACHE_MUTEX_UNLOCK(cache->mutex);						\
	}											\
	return ret;										\
}												\
/* Get data */											\
int db_cache_get(DB_CACHE *cache, name *node, void **data)					\
{												\
	name *p = NULL;										\
	DBT k, v;										\
	int ret = -1;										\
	if(cache && node)									\
	{											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
		if((p = RB_FIND(base, &(cache->index), node)))					\
		{										\
			memcpy(node, p, sizeof(name));						\
			memset(&k, 0, sizeof(DBT));						\
			k.data = node->key;							\
                        k.size = NKEY;								\
			memset(&v, 0, sizeof(DBT));						\
			if(cache->db->get(cache->db, NULL, &k, &v, 0) == 0 )			\
			{									\
				(*data) = v.data;						\
				ret = 0;							\
			}									\
		}										\
		CACHE_MUTEX_UNLOCK(cache->mutex);							\
	}											\
        return ret;              								\
}												\
/* Update data */										\
int db_cache_update(DB_CACHE *cache, name *node, void *data)					\
{												\
	name *p = NULL;										\
        DBT k, v;										\
        int ret = -1;                                                                           \
        if(cache && node)									\
        {											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
                if(p = RB_FIND(base, &(cache->index), node))					\
                {										\
			p->size = node->size;							\
			memcpy(&(p->prop), &(node->prop), sizeof(type));			\
                        memset(&k, 0, sizeof(DBT));						\
                        k.data = node->key;							\
                        k.size = NKEY;								\
                        memset(&v, 0, sizeof(DBT));						\
			v.data = data;								\
			v.size = node->size;							\
                        if(cache->db->del(cache->db, NULL, &k, 0) == 0 )			\
			{									\
				ret = cache->db->put(cache->db, NULL, &k, &v, 0);		\
			}									\
                }										\
		CACHE_MUTEX_UNLOCK(cache->mutex);						\
        }											\
        return ret;										\
}												\
/* Delete data */										\
int db_cache_del(DB_CACHE *cache, name *node)							\
{												\
	name *p = NULL;										\
        DBT k;											\
	int ret = -1;										\
        if(cache && node)									\
        {											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
                if(p = RB_FIND(base, &(cache->index), node))					\
                {										\
                        memset(&k, 0, sizeof(DBT));						\
                        k.data = node->key;							\
                        k.size = NKEY;								\
			cache->count--;								\
                        ret = cache->db->del(cache->db, NULL, &k, 0);				\
                }										\
		CACHE_MUTEX_UNLOCK(cache->mutex);						\
        }											\
        return ret;										\
}												\
/* Resume cache */										\
int db_cache_resume(DB_CACHE *cache)								\
{												\
	int fd = 0;										\
	name node , *p = NULL;									\
	int ret = -1;										\
	if(cache)										\
	{											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
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
			ret = 0;								\
		}										\
		else                                                                            \
                {                                                                               \
                        fprintf(stderr, "Open idxfile[%s] resume failed, %s\n",			\
                                 cache->idxfile, strerror(errno));                              \
			ret = -1;								\
                }										\
		CACHE_MUTEX_UNLOCK(cache->mutex);						\
	}											\
	return ret;										\
}												\
/* Dump cache */										\
int db_cache_dump(DB_CACHE *cache)								\
{												\
	int fd = 0;										\
	name *p = NULL;										\
	int ret = -1;										\
	if(cache)										\
	{											\
		CACHE_MUTEX_LOCK(cache->mutex);							\
		if((fd = open(cache->idxfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) > 0 )	\
		{										\
			RB_FOREACH(p, base, &(cache->index))					\
			{									\
				if(p)write(fd, p, sizeof(name));				\
			}									\
			close(fd);								\
			ret = 0;								\
		}										\
		else										\
		{										\
			fprintf(stderr, "Open idxfile[%s] failed, %s\n",			\
				 cache->idxfile, strerror(errno));				\
		}										\
		CACHE_MUTEX_UNLOCK(cache->mutex);						\
	}											\
	return ret;										\
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
		CACHE_MUTEX_DESTROY((*cache)->mutex);						\
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
		while(i < NKEY) 								\
		{										\
			if(p->key[i] == p1->key[i]) i++;					\
			else return(p->key[i] - p1->key[i]);					\
		}										\
	}											\
	return 0;										\
}
#endif
