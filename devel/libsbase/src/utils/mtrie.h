#ifndef _MTRIE_H_
#define _MTRIE_H_
#ifdef __cplusplus
extern "C" {
#endif
#define MTRIE_PATH_MAX             256
#define MTRIE_LINE_MAX             256
#define MTRIE_INCREMENT_NUM        5000000
#define MTRIE_NODES_MAX            20000000
#define MTRIE_WORD_MAX             4096
typedef struct _MTRLIST
{
    int count;
    int head;
}MTRLIST;
/* trie node */
typedef struct _MTRNODE
{
    short key;
    short nchilds;
    int data;
    int childs;
}MTRNODE;
/* state */
typedef struct _MTRSTATE
{
    int id;
    int current;
    int total;
    int left;
    MTRLIST list[MTRIE_LINE_MAX];
}MTRSTATE;
/* MEM trie */
typedef struct _MTRIE
{
    MTRSTATE    *state;
    MTRNODE     *nodes;
    void        *map;
    void        *oldmap;
    off_t       size;
    off_t       file_size;
    void        *mutex;

    int  (*add)(struct _MTRIE *, char *key, int nkey, int data);
    int  (*xadd)(struct _MTRIE *, char *key, int nkey);
    int  (*get)(struct _MTRIE *, char *key, int nkey);
    int  (*del)(struct _MTRIE *, char *key, int nkey);
    int  (*find)(struct _MTRIE *, char *key, int nkey, int *len);
    int  (*maxfind)(struct _MTRIE *, char *key, int nkey, int *len);
    int  (*radd)(struct _MTRIE *, char *key, int nkey, int data);
    int  (*rxadd)(struct _MTRIE *, char *key, int nkey);
    int  (*rget)(struct _MTRIE *, char *key, int nkey);
    int  (*rdel)(struct _MTRIE *, char *key, int nkey);
    int  (*rfind)(struct _MTRIE *, char *key, int nkey, int *len);
    int  (*rmaxfind)(struct _MTRIE *, char *key, int nkey, int *len);
    int  (*import)(struct _MTRIE *, char *dictfile, int direction);
    void (*clean)(struct _MTRIE *);
}MTRIE;
/* initialize */
MTRIE   *mtrie_init(char *file);
/* add */
int   mtrie_add(struct _MTRIE *, char *key, int nkey, int data);
/* add return auto_increment_id */
int   mtrie_xadd(struct _MTRIE *, char *key, int nkey);
/* get */
int   mtrie_get(struct _MTRIE *, char *key, int nkey);
/* delete */
int   mtrie_del(struct _MTRIE *, char *key, int nkey);
/* find/min */
int   mtrie_find(struct _MTRIE *, char *key, int nkey, int *len);
/* find/max */
int   mtrie_maxfind(struct _MTRIE *, char *key, int nkey, int *len);
/* add/reverse */
int   mtrie_radd(struct _MTRIE *, char *key, int nkey, int data);
/* add/reverse return auto_increment_id */
int   mtrie_rxadd(struct _MTRIE *, char *key, int nkey);
/* get/reverse */
int   mtrie_rget(struct _MTRIE *, char *key, int nkey);
/* del/reverse */
int   mtrie_rdel(struct _MTRIE *, char *key, int nkey);
/* find/min/reverse */
int   mtrie_rfind(struct _MTRIE *, char *key, int nkey, int *len);
/* find/max/reverse */
int   mtrie_rmaxfind(struct _MTRIE *, char *key, int nkey, int *len);
/* import dict if direction value is -1, add word reverse */
int   mtrie_import(struct _MTRIE *, char *dictfile, int direction);
/* destroy */
void mtrie_destroy(struct _MTRIE *);
/* clean/reverse */
void  mtrie_clean(struct _MTRIE *);
#define MTR(x) ((MTRIE *)x)
#ifdef __cplusplus
     }
#endif
#endif
