#ifndef _UDB_H_
#define _UDB_H_
/* DNS */
typedef struct _XDNS
{
    int status;
    char name[HTTP_IP_NAME];
}XDNS;
/* Uniqe DB */
typedef struct _UDB
{

    void *map;
    void *db;

}UDB;
#endif
