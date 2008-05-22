#include "message.h"
int get_msg_no(int message_id)
{
    int msg_no = 0;
    int n = message_id;
    while((n /= 2))++msg_no;
    return msg_no;
}
