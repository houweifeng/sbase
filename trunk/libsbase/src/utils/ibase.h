#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "tree.h"
#include "md5.h"
#ifndef _IBASE_H
#define _IBASE_H
#define WORD_MAX	256
struct _INODE;
typedef struct _INODE
{
	unsigned char *word;
	unsigned char key[_MD5_N];	
	RB_ENTRY(_INODE) rbnode;
}INODE;
typedef struct _INDEX
{
	unsigned char key[_MD5_N];
	size_t nword;	
}INDEX;
typedef struct _IBASE
{
	unsigned int count ;
	size_t	    size;
	unsigned char *idxfile;
	RB_HEAD(ITABLE, _INODE) index;
	
	int (*addword)(struct _IBASE *, unsigned char *, unsigned char *, size_t);	
	int (*readdict)(struct _IBASE *, unsigned char *);
	int (*resume)(struct _IBASE *);
	int (*dump)(struct _IBASE *);
	int (*list)(struct _IBASE *, FILE *);
	INODE *(*new_inode)(struct _IBASE *, unsigned char *, unsigned char *, size_t);	
	void (*clean_inode)(struct _IBASE *, INODE **);
	void (*clean)(struct _IBASE **);
}IBASE;
/* Initialize IBASE */
IBASE *ibase_init();
/* Read Dict */
int ibase_readdict(IBASE *ibase, unsigned char *dictfile);
/* Add word to Index Table */
int ibase_addword(IBASE *ibase, unsigned char *key, unsigned char *word, size_t nword);
/* Resume Index Table from dump file */
int ibase_resume(IBASE *ibase);
/* Dump Index Table to dump file */
int ibase_dump(IBASE *ibase);
/* List Index Table */
int ibase_list(IBASE *ibase, FILE *pout);
/* Clean IBASE */
void ibase_clean(IBASE **ibase);
/* Compare 2 INODE */
int ibase_compare(INODE *_inode, INODE *inode);
/* Initialize NEW INODE in IBASE */
INODE *ibase_new_inode(IBASE *ibase, unsigned char *key, unsigned char *word, size_t nword);
/* Clean INODE in IBASE */
void ibase_clean_inode(IBASE *ibase, INODE **inode);

#endif
