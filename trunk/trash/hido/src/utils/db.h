#ifndef _DB_H_
#define _DB_H_
#define DB_LNK_MAX          65536
#define DB_LNK_INCREMENT    65536
#define DB_DBX_MAX          20000000
#define DB_DBX_INCREMENT    100000
#define DB_PAGE_SIZE        512
#define DB_PATH_MAX         1024
#define DB_MBLOCKS_MAX      256
#define DB_MBLOCK_MAX       1048576
//#define DB_MFILE_SIZE       2097152
//#define DB_MFILE_SIZE       536870912
#define DB_MFILE_SIZE       1073741824
#define DB_MFILE_MAX        2048
typedef struct _DBX
{
    int block_size;
    int blockid;
    int ndata;
    int index;
}DBX;
typedef struct _XIO
{
    int     fd;
    void    *map;
    off_t   end;
    off_t   size;
}XIO;
typedef struct _XLNK
{
    int index;
    int blockid;
    int count;
}XLNK;
typedef struct _XSTATE
{
    int is_mmap;
    int last_id;
    int last_off;
}XSTATE;
typedef struct _DB
{
    void    *mutex;
    void    *kmap;
    void    *logger;
    XSTATE  *state;
    XIO     stateio;
    XIO     lnkio;
    XIO     dbxio;
    XIO     dbsio[DB_MFILE_MAX];
    char    *qmblocks[DB_MBLOCKS_MAX];
    int     nqmblocks;
    char    basedir[DB_PATH_MAX];
}DB;
/* initialize db */
DB* db_init(char *dir, int is_mmap);
/* get data id */
int db_data_id(DB *db, char *key, int nkey);
/* set data return blockid */
int db_set_data(DB *db, int id, char *data, int ndata);
/* set data */
int db_xset_data(DB *db, char *key, int nkey, char *data, int ndata);
/* add data */
int db_add_data(DB *db, int id, char *data, int ndata);
/* xadd data */
int db_xadd_data(DB *db, char *key, int nkey, char *data, int ndata);
/* get data */
int db_get_data(DB *db, int id, char **data);
/* get data len */
int db_get_data_len(DB *db, int id);
/* xget data */
int db_xget_data(DB *db, char *key, int nkey, char **data, int *ndata);
/* xget data len */
int db_xget_data_len(DB *db, char *key, int nkey);
/* read data */
int db_read_data(DB *db, int id, char *data);
/* pread data */
int db_pread_data(DB *db, int id, char *data, int len, int off);
/* xread data */
int db_xread_data(DB *db, char *key, int nkey, char *data);
/* xpread data */
int db_xpread_data(DB *db, char *key, int nkey, char *data, int len, int off);
/* free data */
void db_free_data(DB *db, char *data);
/* delete data */
int db_del_data(DB *db, int id);
/* delete data */
int db_xdel_data(DB *db, char *key, int nkey);
/* destroy */
void db_destroy(DB *db);
/* clean db */
void db_clean(DB *db);
#define PDB(x) ((DB *)x)
#endif
