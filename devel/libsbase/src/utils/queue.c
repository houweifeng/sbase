#include <stdio.h>
#include "queue.h"
#ifdef _DEBUG_QUEUE
#define INT_BYTE  4
typedef struct _ABC
{
    int a;
    int b;
}ABC;
int main()
{
    int i = 0, intd = 0;
    int max = 1024;
    ABC abc = {0};
    void *queue = NULL;

    if((queue = QUEUE_INIT()))
    {
        for(i = 0; i < max; i++)
        {
            QUEUE_PUSH(queue, int, &i);
        }
        while(QUEUE_POP(queue, int, &intd) == 0)
        {
            fprintf(stdout, "%d\n", intd);
        }
        QUEUE_RESET(queue);
        for(i = 0; i < max; i++)
        {
           abc.a = i;
           abc.b = max - i;
           QUEUE_PUSH(queue, ABC, &abc);
        }
        while(QUEUE_POP(queue, ABC, &abc) == 0)
        {
            fprintf(stdout, "a:%d b:%d\n", abc.a, abc.b);
        }
        QUEUE_CLEAN(queue);
    }
}
#endif
