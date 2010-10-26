#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "db.h"
#include "mutex.h"
#include "mmtrie.h"
#include "logger.h"
#ifdef MAP_LOCKED
#define MMAP_SHARED MAP_SHARED|MAP_LOCKED
#else
#define MMAP_SHARED MAP_SHARED
#endif
#define DB_CHECK_MMAP(xdb, index)                                                           \
do                                                                                          \
{                                                                                           \
    if(xdb->state->is_mmap && xdb->dbsio[index].fd > 0)                                     \
    {                                                                                       \
        if(xdb->dbsio[index].map == NULL || xdb->dbsio[index].map == (void *)-1)            \
        {                                                                                   \
            xdb->dbsio[index].map = mmap(NULL, DB_MFILE_SIZE, PROT_READ|PROT_WRITE,         \
                    MAP_SHARED, xdb->dbsio[index].fd, 0);                                   \
        }                                                                                   \
    }                                                                                       \
}while(0)

int db_mkdir(char *path)
{
    struct stat st;
    char fullpath[DB_PATH_MAX];
    char *p = NULL;
    int level = -1, ret = -1;

    if(path)
    {
        strcpy(fullpath, path);
        p = fullpath;
        while(*p != '\0')
        {
            if(*p == '/' )
            {
                level++;
                while(*p != '\0' && *p == '/' && *(p+1) == '/')++p;
                if(level > 0)
                {
                    *p = '\0';                    memset(&st, 0, sizeof(struct stat));
                    ret = stat(fullpath, &st);
                    if(ret == 0 && !S_ISDIR(st.st_mode)) return -1;
                    if(ret != 0 && mkdir(fullpath, 0755) != 0) return -1;
                    *p = '/';
                }
            }
            ++p;
        }
        return 0;
    }
    return -1;
}

/* initialize dbfile */
DB *db_init(char *dbdir, int is_mmap)
{
    char path[DB_PATH_MAX];
    struct stat st = {0};
    DB *db = NULL;

    if(dbdir && (db = (DB *)calloc(1, sizeof(DB))))
    {
        MUTEX_INIT(db->mutex);
        strcpy(db->basedir, dbdir);
        /* initialize kmap */
        sprintf(path, "%s/%s", dbdir, "db.kmap");    
        db_mkdir(path);
        db->kmap = mmtrie_init(path);
        /* logger */
        sprintf(path, "%s/%s", dbdir, "db.log");    
        LOGGER_INIT(db->logger, path);
        /* open state */
        sprintf(path, "%s/%s", dbdir, "db.state");    
        if((db->stateio.fd = open(path, O_CREAT|O_RDWR, 0644)) > 0
                && fstat(db->stateio.fd, &st) == 0)
        {
            db->stateio.end = st.st_size;
            if(st.st_size == 0)
            {
                db->stateio.end = db->stateio.size = sizeof(XSTATE);
                if(ftruncate(db->stateio.fd, db->stateio.end) != 0)
                {
                    FATAL_LOGGER(db->logger, "ftruncate state %s failed, %s\n", path, strerror(errno));
                    _exit(-1);
                }
            }
            if((db->stateio.map = mmap(NULL, db->stateio.end, PROT_READ|PROT_WRITE,  
                MMAP_SHARED, db->stateio.fd, 0)) == NULL || db->stateio.map == (void *)-1)
            {
                FATAL_LOGGER(db->logger, "mmap state:%s failed, %s\n", path, strerror(errno));
                _exit(-1);
            }
            db->state = (XSTATE *)(db->stateio.map);
            db->state->is_mmap = is_mmap;
        }
        else
        {
            FATAL_LOGGER(db->logger, "open link file:%s failed, %s\n", path, strerror(errno));
            _exit(-1);
        }
        /* open link */
        sprintf(path, "%s/%s", dbdir, "db.lnk");    
        if((db->lnkio.fd = open(path, O_CREAT|O_RDWR, 0644)) > 0
                && fstat(db->lnkio.fd, &st) == 0)
        {
            db->lnkio.end = st.st_size;
            if(st.st_size == 0)
            {
                db->lnkio.end = db->lnkio.size = sizeof(XLNK) * DB_LNK_MAX;
                if(ftruncate(db->lnkio.fd, db->lnkio.end) != 0)
                {
                    FATAL_LOGGER(db->logger, "ftruncate %s failed, %s\n", path, strerror(errno));
                    _exit(-1);
                }
            }
            if((db->lnkio.map = mmap(NULL, db->lnkio.end, PROT_READ|PROT_WRITE,  
                MMAP_SHARED, db->lnkio.fd, 0)) == NULL || db->lnkio.map == (void *)-1)
            {
                FATAL_LOGGER(db->logger, "mmap link:%s failed, %s\n", path, strerror(errno));
                _exit(-1);
            }
        }
        else
        {
            FATAL_LOGGER(db->logger, "open link file:%s failed, %s\n", path, strerror(errno));
            _exit(-1);
        }
        /* open dbx */
        sprintf(path, "%s/%s", dbdir, "db.dbx");    
        if((db->dbxio.fd = open(path, O_CREAT|O_RDWR, 0644)) > 0
                && fstat(db->dbxio.fd, &st) == 0)
        {
            db->dbxio.end = st.st_size;
            if(st.st_size == 0)
            {
                db->dbxio.end = db->dbxio.size = sizeof(DBX) * DB_DBX_MAX;
                if(ftruncate(db->dbxio.fd, db->dbxio.end) != 0)
                {
                    FATAL_LOGGER(db->logger, "ftruncate %s failed, %s\n", path, strerror(errno));
                    _exit(-1);
                }

            }
            if((db->dbxio.map = mmap(NULL, db->dbxio.end, PROT_READ|PROT_WRITE,  
                    MMAP_SHARED, db->dbxio.fd, 0)) == NULL || db->dbxio.map == (void *)-1)
            {
                FATAL_LOGGER(db->logger,"mmap dbx:%s failed, %s\n", path, strerror(errno));
                _exit(-1);
            }
        }
        else
        {
            fprintf(stderr, "open link file:%s failed, %s\n", path, strerror(errno));
            _exit(-1);
        }
        /* open dbs */
        int i = 0;
        for(i = 0; i <= db->state->last_id; i++)
        {
            sprintf(path, "%s/%d.db", dbdir, i);    
            if((db->dbsio[i].fd = open(path, O_CREAT|O_RDWR, 0644)) > 0 
                    && fstat(db->dbsio[i].fd, &st) == 0)
            {
                if(st.st_size == 0)
                {
                    db->dbsio[i].size = DB_MFILE_SIZE;
                    if(ftruncate(db->dbsio[i].fd, db->dbsio[i].size) != 0)
                    {
                        FATAL_LOGGER(db->logger, "ftruncate db:%s failed, %s", path, strerror(errno));
                        _exit(-1);
                    }
                }
                else
                {
                    db->dbsio[i].size = st.st_size;
                }
                DB_CHECK_MMAP(db, i);
            }
            else
            {
                fprintf(stderr, "open db file:%s failed, %s\n", path, strerror(errno));
                _exit(-1);
            }
        }
    }
    return db;
}

/* push mblock */
void db_push_mblock(DB *db, char *mblock)
{
    int x = 0;

    if(db && mblock)
    {
        MUTEX_LOCK(db->mutex);
        if(db->nqmblocks < DB_MBLOCKS_MAX)
        {
            x = db->nqmblocks++;
            db->qmblocks[x] = mblock;
        }
        else
        {
            free(mblock);
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ;
}

/* db pop mblock */
char *db_pop_mblock(DB *db)
{
    char *mblock = NULL;
    int x = 0;

    if(db)
    {
        if(db->nqmblocks > 0)
        {
            x = --(db->nqmblocks);
            mblock = db->qmblocks[x];
            db->qmblocks[x] = NULL;
        }
        else
        {
#ifdef HAVE_MMAP
        if((mblock = (char *)mmap(NULL, DB_MBLOCK_MAX, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1,0)) == (void *)-1)
            mblock = NULL;
        if(mblock) memset(mblock, 0, DB_MBLOCK_MAX);

#else
            mblock = (char *)calloc(1, DB_MBLOCK_MAX);
#endif
        }
    }
    return mblock;
}

#define CHECK_XIO(xdb, rid)                                                                 \
do                                                                                          \
{                                                                                           \
    if((off_t)(rid * sizeof(XLNK)) >= xdb->lnkio.end)                                       \
    {                                                                                       \
        xdb->lnkio.end = (off_t)((rid / DB_LNK_INCREMENT)+1);                               \
        xdb->lnkio.end *= (off_t)(sizeof(XLNK) * DB_LNK_INCREMENT);                         \
        if(ftruncate(xdb->lnkio.fd, xdb->lnkio.end) != 0)break;                             \
    }                                                                                       \
    if(xdb->lnkio.end > xdb->lnkio.size)                                                    \
    {                                                                                       \
        if(xdb->lnkio.map && xdb->lnkio.size > 0)                                           \
        {                                                                                   \
            munmap(xdb->lnkio.map, xdb->lnkio.size);                                        \
        }                                                                                   \
        xdb->lnkio.map = NULL;                                                              \
        xdb->lnkio.size = xdb->lnkio.end;                                                   \
    }                                                                                       \
    if(xdb->lnkio.map == NULL)                                                              \
    {                                                                                       \
        if((xdb->lnkio.map = mmap(NULL, xdb->lnkio.size, PROT_READ|PROT_WRITE,              \
                        MMAP_SHARED, xdb->lnkio.fd, 0)) == NULL                             \
                || xdb->lnkio.map == (void *)-1)                                            \
        break;                                                                              \
    }                                                                                       \
}while(0)
#define DB_BLOCKS_COUNT(size) ((size/DB_PAGE_SIZE)+(size%DB_PAGE_SIZE > 0))
/* push block */
int db_push_block(DB *db, int index, int blockid, int block_size)
{
    XLNK *links = NULL, *link = NULL, lnk = {0};
    int x = 0;

    if(db && blockid >= 0 && (x = DB_BLOCKS_COUNT(block_size)) > 0
            && index >= 0 && index < DB_MFILE_MAX)
    {
        CHECK_XIO(db, x);
        if((links = (XLNK *)(db->lnkio.map)))
        {
            if(links[x].count > 0)
            {
                if(db->dbsio[index].map)
                {
                    link = (XLNK *)(((char *)db->dbsio[index].map)
                            +((off_t)blockid * (off_t)DB_PAGE_SIZE));
                    link->index = links[x].index;
                    link->blockid = links[x].blockid;
                }
                else
                {
                    lnk.index = links[x].index;
                    lnk.blockid = links[x].blockid;
                    if(pwrite(db->dbsio[index].fd, &lnk, sizeof(XLNK), 
                                (off_t)blockid * (off_t)DB_PAGE_SIZE) < 0)
                    {
                        FATAL_LOGGER(db->logger, "added link blockid:%d to index[%d] failed, %s",
                                blockid, index, strerror(errno));
                        return -1;
                    }
                }
            }
            links[x].index = index;
            links[x].blockid = blockid;
            ++(links[x].count);
            return 0;
        }
    }
    return -1;
}

/* pop block */
int db_pop_block(DB *db, int blocks_count, XLNK *lnk)
{
    XLNK *links = NULL, *plink = NULL, link = {0};
    int x = 0, index = -1, left = 0, ret = -1;
    char path[DB_PATH_MAX];

    if(db && (x = blocks_count) > 0 && lnk)
    {
        CHECK_XIO(db, blocks_count);
        if((links = (XLNK *)(db->lnkio.map)) && links[x].count > 0 
                && (index = links[x].index) >= 0 && index < DB_MFILE_MAX) 
        {
            lnk->index = index;
            lnk->blockid = links[x].blockid;
            ret = 0;
            if(--(links[x].count) > 0)
            {
                if((db->dbsio[index].map))
                {
                    plink = (XLNK *)((char *)(db->dbsio[index].map)
                        +(off_t)links[x].blockid * (off_t)DB_PAGE_SIZE);
                    links[x].index = plink->index;
                    links[x].blockid = plink->blockid;
                }
                else
                {
                    if(pread(db->dbsio[index].fd, &link, sizeof(XLNK), 
                                (off_t)links[x].blockid * (off_t)DB_PAGE_SIZE) > 0)
                    {
                        links[x].index = link.index;
                        links[x].blockid = link.blockid;
                    }
                }
            }
        }
        else
        {
            x = db->state->last_id;
            left = db->dbsio[x].size - db->state->last_off;
            if(left < (DB_PAGE_SIZE * blocks_count))
            {
                if(left > 0)db_push_block(db, x, (db->state->last_off/DB_PAGE_SIZE), left);
                db->state->last_off = DB_PAGE_SIZE * blocks_count;
                if((x = ++(db->state->last_id)) < DB_MFILE_MAX 
                        && sprintf(path, "%s/%d.db", db->basedir, x)
                        && (db->dbsio[x].fd = open(path, O_CREAT|O_RDWR, 0644)) > 0
                        && ftruncate(db->dbsio[x].fd, DB_MFILE_SIZE) == 0)
                {
                    db->dbsio[x].end = db->dbsio[x].size = DB_MFILE_SIZE;
                    DB_CHECK_MMAP(db, x);
                    lnk->index = x;
                    lnk->blockid = 0;
                    ret = 0;
                }
            }
            else
            {
                lnk->index = x;
                lnk->blockid = (db->state->last_off/DB_PAGE_SIZE);
                db->state->last_off += DB_PAGE_SIZE * blocks_count;
                ret = 0;
            }
        }
    }
    return ret;
}

#define CHECK_DBXIO(xdb, rid)                                                               \
do                                                                                          \
{                                                                                           \
    if((off_t)(rid * sizeof(DBX)) >= xdb->dbxio.end)                                        \
    {                                                                                       \
        xdb->dbxio.end = (off_t)((rid / DB_DBX_INCREMENT)+1);                               \
        xdb->dbxio.end *= (off_t)(sizeof(DBX) * DB_DBX_INCREMENT);                          \
        if(ftruncate(xdb->dbxio.fd, xdb->dbxio.end) != 0)break;                             \
    }                                                                                       \
    if(xdb->dbxio.end > xdb->dbxio.size)                                                    \
    {                                                                                       \
        if(xdb->dbxio.map && xdb->dbxio.size > 0)                                           \
        {                                                                                   \
            munmap(xdb->dbxio.map, xdb->dbxio.size);                                        \
        }                                                                                   \
        xdb->dbxio.map = NULL;                                                              \
        xdb->dbxio.size = xdb->dbxio.end;                                                   \
    }                                                                                       \
    if(xdb->dbxio.map == NULL)                                                              \
    {                                                                                       \
        if((xdb->dbxio.map = mmap(NULL, xdb->dbxio.size, PROT_READ|PROT_WRITE,              \
                    MMAP_SHARED, xdb->dbxio.fd, 0)) == NULL                                 \
                || xdb->dbxio.map == (void *)-1)                                            \
        break;                                                                              \
    }                                                                                       \
}while(0)

/* get data id */
int db_data_id(DB *db, char *key, int nkey)
{
    if(db && key && nkey > 0)
    {
        return mmtrie_xadd(MMTR(db->kmap), key, nkey);
    }
    return -1;
}

int db__set__data(DB *db, int id, char *data, int ndata)
{
    int ret = -1, index = 0, blocks_count = 0;
    DBX *dbx = NULL;
    XLNK lnk = {0};

    if(db && id >= 0 && data && ndata > 0)
    {
        CHECK_DBXIO(db, id);
        if((dbx = (DBX *)(db->dbxio.map)))
        {
            if(dbx[id].block_size < ndata)
            {
                if(dbx[id].block_size > 0)
                    db_push_block(db, dbx[id].index, dbx[id].blockid, dbx[id].block_size);
                dbx[id].block_size = 0;
                dbx[id].blockid = 0;
                dbx[id].ndata = 0;
                blocks_count = DB_BLOCKS_COUNT(ndata);
                if(db_pop_block(db, blocks_count, &lnk) == 0)
                {
                    dbx[id].index = lnk.index;
                    dbx[id].blockid = lnk.blockid;
                    dbx[id].block_size = blocks_count * DB_PAGE_SIZE;
                }
                else return -1;
            }
            //REALLOG(db->logger, "set__data(id:%d blockid:%d block_size:%d index:%d", id, dbx[id].blockid, dbx[id].block_size, dbx[id].index);
            index = dbx[id].index;
            if(db->state->is_mmap && dbx[id].blockid >= 0 && db->dbsio[index].map)
            {
                if(memcpy((char *)(db->dbsio[index].map) 
                    + (off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE, data, ndata))
                {
                    dbx[id].ndata = ndata;
                    ret = id;
                }
                else
                {
                    FATAL_LOGGER(db->logger, "update dbx[%d] ndata:%d to block[%d] block_size:%d failed, %s", id, ndata, dbx[id].blockid, dbx[id].block_size, strerror(errno));
                    dbx[id].ndata = 0;
                }
            }
            else
            {
                if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE, 
                            SEEK_SET) >= 0 && write(db->dbsio[index].fd,data,ndata)>0)
                {
                    dbx[id].ndata = ndata;
                    ret = id;
                }
                else 
                {
                    FATAL_LOGGER(db->logger, "update dbx[%d] ndata:%d to block[%d] block_size:%d failed, %s", id, ndata, dbx[id].blockid, dbx[id].block_size, strerror(errno));
                    dbx[id].ndata = 0;
                }
            }
        }
    }
    return ret;
}

/* db set data */
int db_xset_data(DB *db, char *key, int nkey, char *data, int ndata)
{
    int id = -1, ret = -1;

    if(db && key && nkey > 0 && data && ndata > 0)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_xadd(MMTR(db->kmap), key, nkey)) > 0)
        {
            ret = db__set__data(db, id, data, ndata);
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* db set data */
int db_set_data(DB *db, int id, char *data, int ndata)
{
    int ret = -1;

    if(db && id >= 0 && data && ndata > 0)
    {
        MUTEX_LOCK(db->mutex);
        ret = db__set__data(db, id, data, ndata);
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* add data */
int db__add__data(DB *db, int id, char *data, int ndata)
{
    int ret = -1, blockid = 0, size = 0, block_size = 0, blocks_count = 0, index = 0, x = 0;
    char *block = NULL, *old = NULL;
    DBX *dbx = NULL;
    XLNK lnk = {0};

    if(db && id >= 0 && data && ndata > 0)
    {
        CHECK_DBXIO(db, id);
        if((dbx = (DBX *)(db->dbxio.map)))
        {
            if((size = (dbx[id].ndata + ndata)) > dbx[id].block_size)
            {
                blocks_count = DB_BLOCKS_COUNT(size);
                if(db_pop_block(db, blocks_count, &lnk) == 0)
                {
                    blockid = lnk.blockid;
                    x = lnk.index;
                    block_size = blocks_count * DB_PAGE_SIZE;
                    //fprintf(stdout, "%s::%d new_block(index:%d blockid:%d)\n", __FILE__, __LINE__, x, blockid);
                }
                else 
                {
                    FATAL_LOGGER(db->logger, "pop_block(%d) failed, %s", blocks_count, strerror(errno));
                    return -1;
                }
                index = dbx[id].index;
                if(dbx[id].ndata > 0)
                {
                    if(db->state->is_mmap && db->dbsio[index].map && db->dbsio[x].map) 
                    {
                        old = (char *)(db->dbsio[index].map)
                            +(off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE;
                        block = (char *)(db->dbsio[x].map)
                            +(off_t)blockid *(off_t)DB_PAGE_SIZE;
                        memcpy(block, old, dbx[id].ndata);
                        db_push_block(db, dbx[id].index, dbx[id].blockid, dbx[id].block_size);
                        dbx[id].index = lnk.index;
                        dbx[id].blockid = blockid;
                        dbx[id].block_size = block_size;
                        memcpy(block+dbx[id].ndata, data, ndata);
                        dbx[id].ndata += ndata;
                        ret = id;
                    }
                    else
                    {
#ifdef HAVE_MMAP
                        if((old = (char *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1,0)) == (void *)-1)
                            old = NULL;
                        if(old) memset(old, 0, size);
#else
                        old = (char *)calloc(1, size);
#endif
                        if(old)
                        {
                            if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid
                                        *(off_t)DB_PAGE_SIZE, SEEK_SET) < 0 
                                    || read(db->dbsio[index].fd, old, dbx[id].ndata) <= 0)
                            {
                                FATAL_LOGGER(db->logger, "read index[%d] old[%d] data failed, %s", id, dbx[id].blockid, strerror(errno));
                                goto end;
                            }
                            db_push_block(db, dbx[id].index, dbx[id].blockid, dbx[id].block_size);
                            dbx[id].index = lnk.index;
                            dbx[id].blockid = blockid;
                            dbx[id].block_size = block_size;
                            memcpy(old+dbx[id].ndata, data, ndata);
                            if(lseek(db->dbsio[x].fd, (off_t)blockid * (off_t)DB_PAGE_SIZE, 
                                        SEEK_SET) >= 0 && write(db->dbsio[x].fd,
                                            old,dbx[id].ndata+ndata) > 0) 
                            {
                                dbx[id].ndata += ndata;
                                ret = id;
                            }
                            free(old);
                        }
                    }
                    goto end;
                }
                else
                {
                    dbx[id].index = lnk.index;
                    dbx[id].blockid = lnk.blockid;
                    dbx[id].block_size = block_size;
                }
            }
            index = dbx[id].index;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                block = (char *)(db->dbsio[index].map)+(off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE;
                memcpy(block+dbx[id].ndata, data, ndata);
                dbx[id].ndata += ndata;
                ret = id;
            }
            else
            {
                if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE 
                            + (off_t)dbx[id].ndata, SEEK_SET) >= 0 
                            && write(db->dbsio[index].fd, data, ndata) > 0)
                {
                    dbx[id].ndata += ndata;
                    ret = id;
                }
            }
            //fprintf(stdout, "(index:%d blockid:%d)%d\n",  dbx[id].index,  dbx[id].blockid, dbx[id].ndata);
        }
    }
end:
    return ret;
}

/* xadd data */
int db_xadd_data(DB *db, char *key, int nkey, char *data, int ndata)
{
    int id = -1, ret = -1;

    if(db && key && nkey > 0 && data && ndata > 0)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_xadd(MMTR(db->kmap), key, nkey)) > 0)
        {
            ret = db__add__data(db, id, data, ndata);
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* db add data */
int db_add_data(DB *db, int id, char *data, int ndata)
{
    int ret = -1;

    if(db && id >= 0 && data && ndata > 0)
    {
        MUTEX_LOCK(db->mutex);
        ret = db__add__data(db, id, data, ndata);
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* xget data  len*/
int db_xget_data_len(DB *db, char *key, int nkey)
{
    int id = -1, ret = -1;
    DBX *dbx = NULL;

    if(db && key)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_get(MMTR(db->kmap), key, nkey)) > 0 
                && (dbx = (DBX *)db->dbxio.map)) 
        {
            ret = dbx[id].ndata; 
        }
        MUTEX_UNLOCK(db->mutex);

    }
    return ret;
}

/* xget data */
int db_xget_data(DB *db, char *key, int nkey, char **data, int *ndata)
{
    int id = -1, index = 0;
    DBX *dbx = NULL;

    if(db && key)
    {
        MUTEX_LOCK(db->mutex);
        *ndata = 0;
        if((id = mmtrie_get(MMTR(db->kmap), key, nkey)) > 0 
                && (dbx = (DBX *)db->dbxio.map) && dbx[id].ndata > 0) 
        {
            if(dbx[id].block_size <= DB_MBLOCK_MAX)
                *data = db_pop_mblock(db);
            else
                *data = (char *)calloc(1, dbx[id].block_size);
            index = dbx[id].index;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                if(*data && db->dbsio[index].map && memcpy(*data, (char *)(db->dbsio[index].map)
                            +(off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE, dbx[id].ndata))
                {
                    *ndata = dbx[id].ndata;
                }
                else
                {
                    if(*data){free(*data);*data = NULL;}
                }
            }
            else
            {
                if(*data && lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE,
                            SEEK_SET) >= 0 && read(db->dbsio[index].fd, *data, dbx[id].ndata) > 0)
                    *ndata = dbx[id].ndata;
                else
                {
                    if(*data){free(*data);*data = NULL;}
                }
            }
        }
        else
        {
            id = mmtrie_xadd(MMTR(db->kmap), key, nkey);
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return id;
}

/* get data len*/
int db_get_data_len(DB *db, int id)
{
    DBX *dbx = NULL;
    int ret = -1;

    if(db && id >= 0)
    {
        MUTEX_LOCK(db->mutex);
        CHECK_DBXIO(db, id);
        if((dbx = (DBX *)(db->dbxio.map)))
        {
            ret = dbx[id].ndata;
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* get data */
int db_get_data(DB *db, int id, char **data)
{
    int n = -1, index = 0;
    DBX *dbx = NULL;

    if(db && id >= 0)
    {
        MUTEX_LOCK(db->mutex);
        CHECK_DBXIO(db, id);
        if((dbx = (DBX *)(db->dbxio.map)) && dbx[id].ndata > 0)
        {
            if(dbx[id].block_size <= DB_MBLOCK_MAX)
                *data = db_pop_mblock(db);
            else
                *data = (char *)calloc(1, dbx[id].block_size);
            index = dbx[id].index;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                if(*data && db->dbsio[index].map && memcpy(*data, (char *)(db->dbsio[index].map)
                            +(off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE, dbx[id].ndata))
                {
                    n = dbx[id].ndata;
                }
                else
                {
                    if(*data){free(*data);*data = NULL;}
                }
            }
            else
            {
                if(*data && lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE, 
                            SEEK_SET) >= 0 && read(db->dbsio[index].fd, *data, dbx[id].ndata) > 0)
                    n = dbx[id].ndata;
                else
                {
                    if(*data){free(*data);*data = NULL;}
                }
            }
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return n;
}

/* xread data */
int db_xread_data(DB *db, char *key, int nkey, char *data)
{
    int ret = -1, index = 0, n = -1, id = -1;
    DBX *dbx = NULL;

    if(db && key && nkey > 0 && data)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_get(MMTR(db->kmap), key, nkey)) > 0 
                && (dbx = (DBX *)(db->dbxio.map))
                && dbx[id].blockid >= 0 && (n = dbx[id].ndata) > 0) 
        {
            index = dbx[id].index;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                if(memcpy(data, (char *)(db->dbsio[index].map) + (off_t)dbx[id].blockid 
                        *(off_t)DB_PAGE_SIZE, n))
                    ret = n;
            }
            else
            {
                if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid * (off_t)DB_PAGE_SIZE, 
                            SEEK_SET) >= 0 && read(db->dbsio[index].fd, data, n)>0)
                    ret = n;
            }
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* xpread data */
int db_xpread_data(DB *db, char *key, int nkey, char *data, int len, int off)
{
    int ret = -1, index = 0 , n = -1, id = -1;
    DBX *dbx = NULL;

    if(db && key && nkey > 0 && data)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_get(MMTR(db->kmap), key, nkey)) > 0
                && (dbx = (DBX *)(db->dbxio.map)) && dbx[id].blockid >= 0
                && (n = dbx[id].ndata) > 0 && off < n)
        {
            index = dbx[id].index;
            n -= off;
            if(len < n) n = len;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                if(memcpy(data, (char *)(db->dbsio[index].map) + (off_t)dbx[id].blockid
                            *(off_t)DB_PAGE_SIZE + (off_t)off, n))
                    ret = n;
            }
            else
            {
                if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE+(off_t)off,
                            SEEK_SET)>= 0 && read(db->dbsio[index].fd, data, n)>0)
                    ret = n;
            }
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* read data */
int db_read_data(DB *db, int id, char *data)
{
    int ret = -1, n = -1, index = 0;
    DBX *dbx = NULL;

    if(db && id >= 0 && data)
    {
        MUTEX_LOCK(db->mutex);
        //REALLOG(db->logger, "0:mmap_read(%d) => %d", id, n);
        if(id < db->dbxio.end/sizeof(DBX) && (dbx = (DBX *)(db->dbxio.map)))
        {
            if(dbx[id].blockid >= 0 && (n = dbx[id].ndata) > 0) 
            {
                //REALLOG(db->logger, "1:mmap_read(%d) => %d", id, n);
                index = dbx[id].index;
                if(db->state->is_mmap && db->dbsio[index].map)
                {
                    if(memcpy(data, (char *)(db->dbsio[index].map) + (off_t)dbx[id].blockid 
                                *(off_t)DB_PAGE_SIZE, n) > 0)
                        ret = n;
                    //REALLOG(db->logger, "2:mmap_read(%d) => %d", id, n);
                }
                else
                {
                    if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid*(off_t)DB_PAGE_SIZE, 
                                SEEK_SET) >= 0 && read(db->dbsio[index].fd, data, n) > 0)
                        ret = n;
                }
            }
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* pread data */
int db_pread_data(DB *db, int id, char *data, int len, int off)
{
    int ret = -1, index = 0, n = -1;
    DBX *dbx = NULL;

    if(db && id >= 0 && data)
    {
        MUTEX_LOCK(db->mutex);
        if(id < db->dbxio.end/sizeof(DBX) && (dbx = (DBX *)(db->dbxio.map))
                && dbx[id].blockid >= 0 && (n = dbx[id].ndata) > 0 && off < n)
        {
            index = dbx[id].index;
            n -= off;
            if(len < n) n = len;
            if(db->state->is_mmap && db->dbsio[index].map)
            {
                if(memcpy(data, (char *)(db->dbsio[index].map) + (off_t)dbx[id].blockid
                            *(off_t)DB_PAGE_SIZE + off, n) > 0)
                    ret = n;
            }
            else
            {
                if(lseek(db->dbsio[index].fd, (off_t)dbx[id].blockid *(off_t)DB_PAGE_SIZE+off,
                            SEEK_SET)>= 0 && read(db->dbsio[index].fd, data, n) > 0)
                    ret = n;

            }
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* free data */
void db_free_data(DB *db, char *data)
{
    if(data) 
    {
        db_push_mblock(db, data);
    }
    return ;
}

/* delete data */
int db_del_data(DB *db, int id)
{
    DBX *dbx = NULL;
    int ret = -1;

    if(db && id >= 0)
    {
        MUTEX_LOCK(db->mutex);
        CHECK_DBXIO(db, id);
        if((dbx = (DBX *)(db->dbxio.map)))
        {
            db_push_block(db, dbx[id].index, dbx[id].blockid, dbx[id].block_size);
            dbx[id].block_size = 0;
            dbx[id].blockid = 0;
            dbx[id].ndata = 0;
            ret = id;
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* delete data */
int db_xdel_data(DB *db, char *key, int nkey)
{
    int id = -1, ret = -1;
    DBX *dbx = NULL;

    if(db && key && nkey > 0)
    {
        MUTEX_LOCK(db->mutex);
        if((id = mmtrie_get(MMTR(db->kmap), key, nkey)) >= 0
                && (dbx = (DBX *)(db->dbxio.map)))
        {
            db_push_block(db, dbx[id].index, dbx[id].blockid, dbx[id].block_size);
            dbx[id].block_size = 0;
            dbx[id].blockid = 0;
            dbx[id].ndata = 0;
            ret = id;
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ret;
}

/* destroy */
void db_destroy(DB *db)
{
    int ret = 0, i = 0, is_mmap = 0;
    char path[DB_PATH_MAX];

    if(db)
    {
        MUTEX_LOCK(db->mutex);
        mmtrie_destroy(db->kmap);
        /* dbx */
        if(db->dbxio.map) 
        {
            munmap(db->dbxio.map, db->dbxio.end);
            db->dbxio.map = NULL;
            db->dbxio.end = 0;
        }
        if(db->dbxio.fd > 0)
        {
            ret = ftruncate(db->dbxio.fd, 0);
            db->dbxio.end = db->dbxio.size = sizeof(DBX) * DB_DBX_MAX;
            ret = ftruncate(db->dbxio.fd, db->dbxio.end);
            if((db->dbxio.map = mmap(NULL, db->dbxio.end, PROT_READ|PROT_WRITE,
                MMAP_SHARED, db->dbxio.fd, 0)) == NULL || db->dbxio.map == (void *)-1)
            {
                FATAL_LOGGER(db->logger,"mmap dbx failed, %s", strerror(errno));
                _exit(-1);
            }
        }
        /* dbs */
        for(i = 0; i <= db->state->last_id; i++)
        {
            if(db->dbsio[i].map) 
            {
                munmap(db->dbsio[i].map, db->dbsio[i].end);
                db->dbsio[i].map = NULL;
                db->dbsio[i].end = 0;
            }
            if(db->dbsio[i].fd > 0)
            {
                close(db->dbsio[i].fd);
                db->dbsio[i].fd = 0;
            }
            sprintf(path, "%s/%d.db", db->basedir, i);
            unlink(path);
        }
        /* link */
        if(db->lnkio.map)
            memset(db->lnkio.map, 0, db->lnkio.end);
        /* state */
        if(db->stateio.map)
        {
            is_mmap = db->state->is_mmap;
            memset(db->stateio.map, 0, db->stateio.end);
            db->state->is_mmap = is_mmap;
        }
        /* open dbs */
        sprintf(path, "%s/0.db", db->basedir);    
        if((db->dbsio[0].fd = open(path, O_CREAT|O_RDWR, 0644)) > 0)
        {
            db->dbsio[0].size = DB_MFILE_SIZE;
            if(ftruncate(db->dbsio[0].fd, db->dbsio[0].size) != 0)
            {
                FATAL_LOGGER(db->logger, "ftruncate db:%s failed, %s", path, strerror(errno));
                _exit(-1);
            }
            DB_CHECK_MMAP(db, 0);
        }
        else
        {
            fprintf(stderr, "open db file:%s failed, %s\n", path, strerror(errno));
            _exit(-1);
        }
        MUTEX_UNLOCK(db->mutex);
    }
    return ;
}

/* clean */
void db_clean(DB *db)
{
    int i = 0;
    if(db)
    {
        for(i = 0; i <= db->state->last_id; i++)
        {
            if(db->dbsio[i].map)munmap(db->dbsio[i].map, db->dbsio[i].end);
            if(db->dbsio[i].fd)close(db->dbsio[i].fd);
        }
        if(db->dbxio.map)munmap(db->dbxio.map, db->dbxio.end);
        if(db->dbxio.fd)close(db->dbxio.fd);
        if(db->lnkio.map)munmap(db->lnkio.map, db->lnkio.end);
        if(db->lnkio.fd)close(db->lnkio.fd);
        if(db->stateio.map)munmap(db->stateio.map, db->stateio.end);
        if(db->stateio.fd)close(db->stateio.fd);
        for(i = 0; i < db->nqmblocks; i++){if(db->qmblocks[i])free(db->qmblocks[i]);}
        mmtrie_clean(MMTR(db->kmap));
        LOGGER_CLEAN(db->logger);
        MUTEX_DESTROY(db->mutex);
        free(db);
    }
    return ;
}

#ifdef _DEBUG_DB
int main(int argc, char **argv)
{
    char *dbdir = "/tmp/db";
    char *key = NULL, *data = NULL;
    int id = 0, n = 0;
    DB *db = NULL;

    if((db = db_init(dbdir, 1)))
    {
        db_destroy(db);
        key = "xxxxx";
        data = "askfjsdlkfjsdlkfasdf";
        /* set data with key */
        if((id = db_xset_data(db, key, strlen(key), data, strlen(data))) > 0)
        {
            fprintf(stdout, "%s::%d xset_data(%s):%s => id:%d\n", __FILE__, __LINE__, key, data, id);
            if((id = db_xget_data(db, key, strlen(key), &data, &n)) > 0 && n > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d xget_data(%s):%s\n",  __FILE__, __LINE__, key, data);
                db_free_data(db, data);
            }
        }
        else
        {
            fprintf(stderr, "%s::%d xset_data(%s) failed, %s\n",
                    __FILE__, __LINE__, key, strerror(errno));
            _exit(-1);
        }
        /* del data with key */
        if(db_xdel_data(db, key, strlen(key)) > 0)
        {
            fprintf(stdout, "%s::%d xdel_data(%s):%s\n", __FILE__, __LINE__, key, data);
            if((id = db_xget_data(db, key, strlen(key), &data, &n)) > 0 && n > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d xget_data(%s):%s\n", __FILE__, __LINE__, key, data);
                db_free_data(db, data);
            }
            else
            {
                fprintf(stdout, "%s::%d xget_data(%s) null\n", __FILE__, __LINE__, key);
            }
        }
        data = "dsfklajslfkjdsl;fj;lsadfklweoirueowir";
        /* set data with id */
        if((db_set_data(db, id, data, strlen(data))) > 0)
        {
            fprintf(stdout, "%s::%d set_data(%d):%s\n", __FILE__, __LINE__, id, data);
            if((n = db_get_data(db, id, &data)) > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d get_data(%d):%s\n", __FILE__, __LINE__, id, data);
                db_free_data(db, data);
            }
        }
        /* delete data with id */
        if(db_del_data(db, id) > 0)
        {
            fprintf(stdout, "%s::%d del_data(%d):%s\n", __FILE__, __LINE__, id, data);
            if((n = db_get_data(db, id, &data)) > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d get_data(%d):%s\n", __FILE__, __LINE__, id, data);
                db_free_data(db, data);
            }
            else
            {
                fprintf(stdout, "%s::%d get_data(%d) null\n",  __FILE__, __LINE__, id);
            }
        }
        data = "sadfj;dslfm;';qkweprkpafk;ldsfma;ldskf;lsdf;sdfk;";
        /* reset data with id */
        if((db_set_data(db, id, data, strlen(data))) > 0)
        {
            fprintf(stdout, "%s::%d set_data(%d):%s\n",  __FILE__, __LINE__, id, data);
            if((n = db_get_data(db, id, &data)) > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d get_data(%d):%s\n", __FILE__, __LINE__, id, data);
                db_free_data(db, data);
            }
    }
        db_destroy(db);
        data = "saddfadfdfak;";
        db_xadd_data(db, "keyxxxx", 7, data, strlen(data));
        /* add data */
        if((id = db_xadd_data(db, "keyx", 4, "data:0\n", 7)) > 0)
        {
            char line[1024], key[256];
            int i = 0, j = 0;
            for(i = 1; i < 10000; i++)
            {
                //sprintf(key, "key:%d\n", i);
                for(j = 0; j < 1000; j++)
                {
                    n = sprintf(line, "line:%d\n", j); 
                    db_add_data(db, i, line, n);
                }
            }
            //char *s = "dsfkhasklfjalksjfdlkasdjflsjdfljsadlf";
            //db_add_data(db, id, s, strlen(s));
            if((n = db_get_data(db, id, &data)) > 0)
            {
                data[n] = '\0';
                fprintf(stdout, "%s::%d get_data(%d):[%d]%s\n", __FILE__, __LINE__, id, n, data);
                db_free_data(db, data);
            }
        }
        db_clean(db);
    }
    return 0;
}
//gcc -o db db.c mmtrie.c -D_DEBUG_DB
#endif
