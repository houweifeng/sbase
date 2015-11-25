> # libsbase是一个对服务, 线程, 连接等与业务逻辑无关的封装, 可以用于服务端和客户端的网络应用开发. #

> ## sbase包括服务管理(service), 线程管理(procthread), 连接管理(connection), 采用libevbase(类似libevent)的网络IO读写通知模式, 对业务逻辑数据的处理采用消息队列(message\_queue)的方式运行, 每个连接(connection)可以定义独立的会话特性(SESSION), 附着与连接上的业务逻辑采用回调的方式执行, 具体的回调函数封装于session内. ##
```
typedef struct _SESSION
{
    /* packet */
    int timeout; //超时设置
    int childid;//代理子连接ID
    void *child;//代理子连接指针
    int  parentid;//代理连接父连接ID
    void *parent;//代理连接父连接指针
    int  packet_type;
    int  packet_length;
    char *packet_delimiter;
    int  packet_delimiter_length;
    int  buffer_size;


    /* methods */
    /* 当连接发生读写错误或者对端断开的时候 用于错误处理 */
    int (*error_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    /* 从连接的数据buffer区内读取数据头 */
    int (*packet_reader)(struct _CONN *, CB_DATA *buffer);
    /* 数据头被读取后的处理 */
    int (*packet_handler)(struct _CONN *, CB_DATA *packet);
    /* 数据头之外的数据处理 */
    int (*data_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    /* 接受完的文件处理 */
    int (*file_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache);
    /* 带外数据处理 */
    int (*oob_handler)(struct _CONN *, CB_DATA *oob);
    /* 超时处理(需要通过conn->set_timeout()) */
    int (*timeout_handler)(struct _CONN *, CB_DATA *packet, CB_DATA *cache, CB_DATA *chunk);
    /* 事务处理(需要通过service->new_transaction()注册事务 )*/
    int (*transaction_handler)(struct _CONN *, int tid);
}SESSION;

```

> ## sbase总体结构 sbase->services[.md](.md) service->procthreads[.md](.md) procthread->connections[.md](.md). ##
> ## sbase 可以同时运行多个service (sbase->add\_service()).    sbase主线程负责管理所有service下的监听端口的文件描数字, 如果sbase通过libevbase发现有fd可读会通知service下的成员负责accept操作和添加新连接操作. ##


```
typedef struct _SBASE
{
    /* base option */
    /* 多进程模式下进程数 */
    int nchilds;
    /* 连接数限制 */
    int connections_limit;
    /* while 循环usleep的微妙数 */
    int usec_sleep;
    /* 运行状态 1 为运行 0 为停止 */
    int running_status;
    /* libevbse */
    EVBASE *evbase;
    /* services[] 列表 */
    struct _SERVICE **services;
    /* 当前运行service个数 */
    int running_services;
    /* 心跳计数 */
    long long nheartbeat;

    /* timer && logger */
    void *logger;
    void *timer;

    /* evtimer 超时检查器 */
    void *evtimer;

    /* message queue for proc mode 多进程模式下的消息队列 */
    void *message_queue;

    int  (*set_log)(struct _SBASE *, char *);
    int  (*set_evlog)(struct _SBASE *, char *);
    
    /* 添加服务 service需要通过service_init() 初始化以后 */
    int  (*add_service)(struct _SBASE *, struct _SERVICE *);    
    /* 运行sbase下的所有服务 */
    int  (*running)(struct _SBASE *, int time_usec);
    /* 停止sbase下的所有服务 */
    void (*stop)(struct _SBASE *);
    void (*clean)(struct _SBASE **);
}SBASE;
```



> ## service 负责管理服务下的所有线程以及连接, service本身不参与主循环的工作, service只是接受sbase的调用以及连接(connection)的调用, 与service相关的业务逻辑都是运行在service->procthreads[.md](.md)线程之上, 另外任务(task)运行在service->daemons[.md](.md)线程之上. service之上的所有连接根据文件描数字(fd)散列到procthreads[.md](.md)之上. ##

```
typedef struct _SERVICE
{
    /* global */
    int lock;
    /* service下所有线程运行usleep 微妙数*/
    int usec_sleep;
    /* sbase指针 */
    SBASE *sbase;
    /* service下所有线程访问service公共资源的锁 */
    void *mutex;

    /* heartbeat */
    /* running heartbeat_handler when looped hearbeat_interval times 心跳间隔 */
    int heartbeat_interval;
    void *heartbeat_arg;
    /* 心跳回调 */
    CALLBACK *heartbeat_handler;
    /* 设置服务心跳心跳控制由sbase->evtimer完成 */
    void (*set_heartbeat)(struct _SERVICE *, int interval, CALLBACK *handler, void *arg);
    /* 心跳操作激活 会调用service->hearbeat_handler()  */
    void (*active_heartbeat)(struct _SERVICE *);

    /* working mode 运行模式*/
    int working_mode;
    /* 多进程模式下使用 */
    struct _PROCTHREAD *daemon;
    /* 线程数 */
    int nprocthreads;
    /* 连接所在线程池 */
    struct _PROCTHREAD **procthreads;
    /* 后台线程个数 */
    int ndaemons;
    /* 用于task的后台线程池, 可选择是否用, 如果ndaemons为0表示不用 */
    struct _PROCTHREAD **daemons;


    /* socket and inet addr option  网络连接或本地监听相关的参数 */
    int family;
    int sock_type;
    struct  sockaddr_in sa;
    char *ip;
    int port;
    int fd;
    int backlog;

    /* service option 服务运行相关 */
    int service_type;
    char *service_name;
    /* 设置服务包括本地监听设置, 初始化远程连接参数 */
    int  (*set)(struct _SERVICE *service);
    /* 运行服务, 初始化线程 */
    int  (*run)(struct _SERVICE *service);
    /* 停止服务 */
    void (*stop)(struct _SERVICE *service);

    /* event option libevbase 相关 */
    EVBASE *evbase;
    EVENT *event;

    /* message queue for proc mode 多进程模式下的消息队列, 等同sbase->message_queue */
    void *message_queue;

    /* chunks queue */
    /* chunks queue 用于回收存放使用过的chunk片段循环使用， 减少内存分配，提高总体性能。*/
    void *chunks_queue;
    struct _CHUNK *(*popchunk)(struct _SERVICE *service);
    int (*pushchunk)(struct _SERVICE *service, struct _CHUNK *cp);

    /* connections option 连接相关参数 */
    /* 连接数限制 */
    int connections_limit;
    /* 当前connecions位置最大值 */
    int index_max;
    /* 当前运行连接总数 */
    int running_connections;
    /* service下的所有连接列表 用于统一管理连接 */
    struct _CONN **connections;
    /* C_SERVICE ONLY 客户端服务操作方法 */
    /* 客户端连接数 */
    int client_connections_limit;
    /* 发起新连接 */
    struct _CONN *(*newconn)(struct _SERVICE *service, int inet_family, int sock_type,
            char *ip, int port, SESSION *session);
    /* 添加新连接 */
    struct _CONN *(*addconn)(struct _SERVICE *service, int sock_type, int fd,
            char *remote_ip, int remote_port, char *local_ip, int local_port, SESSION *);
    /* 获取空闲连接 */
    struct _CONN *(*getconn)(struct _SERVICE *service);
    /* 添加正常正确的连接到service->connecions */
    int     (*pushconn)(struct _SERVICE *service, struct _CONN *conn);
    /* 从service->connections 删除指定连接 */
    int     (*popconn)(struct _SERVICE *service, struct _CONN *conn);
    /* 代理 */
    struct _CONN *(*newproxy)(struct _SERVICE *service, struct _CONN * parent, int inet_family,
            int sock_type, char *ip, int port, SESSION *session);
       struct _CONN *(*findconn)(struct _SERVICE *service, int index);
    /* evtimer 超时检查器 */
    void *evtimer;
    int evid;

    /* timer and logger */
    void *timer;
    void *logger;
    int  is_inside_logger;
    int (*set_log)(struct _SERVICE *service, char *logfile);

    /* transaction and task */
    int ntask;
    /* 运行新任务 任务运行散列通过ntask散列到service->daemons[]之上 */
    int (*newtask)(struct _SERVICE *, CALLBACK *, void *arg);
    /* 在指定连接上执行事务 通过conn->index定位service->connections[] 上 */
    int (*newtransaction)(struct _SERVICE *, struct _CONN *, int tid);

    /* service default session option 设置服务默认业务逻辑的会话参数 */
    SESSION session;
    int (*set_session)(struct _SERVICE *, SESSION *);

    /* clean */
    void (*clean)(struct _SERVICE **pservice);

}SERVICE;
```
> ## procthread负责管理线程之上的所有连接, 包括通过libevbase管理连接的读写通知, 管理新连接添加, 数据头读, 数据头处理, 数据处理, 事务处理, 错误处理, 超时处理等消息. ##
> ### 新连接操作流程: ###
> ### 1. sbase->evbase->loop() 发现某个连接; ###
> ### 2. service->event\_handler() 完成accept, conn\_init() 初始化连接, 然后添加NEW\_CONNECTION消息到i = fd%service->nprocthreads, service->procthreads[i](i.md)->message\_queue 对应的消息队列; ###
> ### 3. procthread 通过消息队列循环获得NEW\_CONNECTION消息,然后procthread->add\_connection()加入新连接, 添加连接fd的读写事件到procthread->evbase, 同时service->pushconn() 注册连接到service->connections[.md](.md); ###
> ### 4. procthread->evbase->loop() 检查连接状态, 同时通过conn->event\_handler激活读写; ###
> ### 5. conn->read\_handler()读取数据到conn->buffer, 同时调用conn->packet\_reader()读取数据头; ###
> ### 6. 数据头读取后添加MESSAGE\_PACKET\_HANDLE消息, procthread通过消息队列循环调用conn->packet\_handler()处理数据头; ###
> ### 7. conn->recv\_chunk()或者conn->recv\_file()可以接收指定长度的数据块或者文件; ###
> ### 8. 数据接收完成以后添加新的数据处理消息MESSAGE\_DATA\_HANDLE, procthread通过消息队列循环调用conn->data\_handler(); ###
> ### 9. 数据处理完毕, 如果需要发送数据的话可以通过conn->push\_chunk()发送数据块或者通过conn->push\_file()发送文件, 同时添加fd写事件到procthread->evbase, 添加数据参数到conn->send\_queue; ###
> ### 10. procthread->evbase->loop()通知连接写数据, 每次根据连接buffer最大写, 然后等待下一次loop()直到完成数据发送, 连接的初始化和一个会话到此完成. ###

> ### 客户端连接建立: ###
> ### 1. 调用service->newconn(), 此处的采用非阻塞连接, 连接并非马上建立, 标记conn->status = CONN\_STATUS\_READY, 然后进入连接添加流程; ###
> ### 2. 进入之前服务连接流程 4 的时候, 如果碰到conn->status == CONN\_STATUS\_READY, 检查socket是否有错, 如果有错当做错误处理, 关闭连接. ###
> ### 3. 其他处理类似. ###

```
/* procthread */
typedef struct _PROCTHREAD
{
    /* global */
   int lock;
    SERVICE *service;
    int running_status;
    int usec_sleep;
    int index;
    long threadid;

    /* message queue */
    void *message_queue;

    /* evbase */
    EVBASE *evbase;

    /* connection */
    struct _CONN **connections;
    /* 添加新连接消息 */
    int (*addconn)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    /* 新连接添加到procthread */
    int (*add_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);
    /* 结束连接  */
    int (*terminate_connection)(struct _PROCTHREAD *procthread, struct _CONN *conn);

    /* logger */
    void *logger;

    /* task and transaction 处理新的事务 和 任务 */
    int (*newtask)(struct _PROCTHREAD *, CALLBACK *, void *arg);
    int (*newtransaction)(struct _PROCTHREAD *, struct _CONN *, int tid);

    /* heartbeat */
    void (*active_heartbeat)(struct _PROCTHREAD *,  CALLBACK *handler, void *arg);
    void (*state)(struct _PROCTHREAD *,  CALLBACK *handler, void *arg);

    /* normal */
    void (*run)(void *arg);
    void (*stop)(struct _PROCTHREAD *procthread);
    void (*terminate)(struct _PROCTHREAD *procthread);
    void (*clean)(struct _PROCTHREAD **procthread);
}PROCTHREAD;

typedef struct _CONN
{
    /* global */
    int index;
    void *parent;

    /* conenction */
    int sock_type;
    int fd;
    char remote_ip[SB_IP_MAX];
    int remote_port;
    char local_ip[SB_IP_MAX];
    int  local_port;
    /* 初始化设置 */
    int (*set)(struct _CONN *);
    int (*close)(struct _CONN *);
    int (*over)(struct _CONN *);
    int (*terminate)(struct _CONN *);

    /* connection bytes stats */
    long long   recv_oob_total;
    long long   sent_oob_total;
    long long   recv_data_total;
    long long   sent_data_total;

    /* evbase */
    EVBASE *evbase;
    EVENT *event;

    /* evtimer */
    void *evtimer;
    int evid;

    /* buffer */
    void *buffer;
    void *packet;
    void *cache;
    void *chunk;
    void *oob;

    /* logger and timer */
    void *logger;
    void *timer;

    /* queue */
    void *send_queue;



    /* message queue */
    void *message_queue;
    /* client transaction state */
    int status;
    int istate;
    int c_state;
    int c_id;
    int (*start_cstate)(struct _CONN *);
    int (*over_cstate)(struct _CONN *);

    /* transaction */
    int s_id;
    int s_state;

    /* event state */
    int evstate;
#define EVSTATE_INIT   0
#define EVSTATE_WAIT   1 
    int (*wait_evstate)(struct _CONN *);
    int (*over_evstate)(struct _CONN *);

    /* timeout */
    int timeout;
    /*  设置读写超时时间 */
    int (*set_timeout)(struct _CONN *, int timeout_usec);
    /*  超时处理 */
    int (*timeout_handler)(struct _CONN *);
    /* message */
    int (*push_message)(struct _CONN *, int message_id);

    /* session */
    /* 从网络IO读数据 */
    int (*read_handler)(struct _CONN *);
    /* 写数据到网络IO */
    int (*write_handler)(struct _CONN *);
    /* 读数据头 */
    int (*packet_reader)(struct _CONN *);
    /* 数据头处理 */
    int (*packet_handler)(struct _CONN *);
    /* 带外数据处理 */
    int (*oob_handler)(struct _CONN *);
    /*  数据处理 */
    int (*data_handler)(struct _CONN *);
    /* 事务处理 */
    int (*transaction_handler)(struct _CONN *, int );
    /* 缓存少量数据不适合存放大量数据 */
    int (*save_cache)(struct _CONN *, void *data, int size);
    int (*bind_proxy)(struct _CONN *, struct _CONN *);
    int (*proxy_handler)(struct _CONN *);
    int (*close_proxy)(struct _CONN *);
    int (*push_exchange)(struct _CONN *, void *data, int size);


    /* chunk */
    /* 用于从buffer或者网络读取数据到chunk或者文件 */
    int (*chunk_reader)(struct _CONN *);
    /* 准备接收数据块 */
    int (*recv_chunk)(struct _CONN *, int size);
    /* 准备接收文件 */
    int (*recv_file)(struct _CONN *, char *file, long long offset, long long size);
    /* 添加数据块到发送队列 */
    int (*push_chunk)(struct _CONN *, void *data, int size);
    /* 添加文件到发送队列 */
    int (*push_file)(struct _CONN *, char *file, long long offset, long long size);

    /* session option and callback  设置会话参数 */
    SESSION session;
    int (*set_session)(struct _CONN *, SESSION *session);

    /* normal */
    void (*clean)(struct _CONN **pconn);
}CONN;
```
## 编译安装: ##
#### homepage : http://sbase.googlecode.com ####

### install : ###
./configure --enable-debug
make

### test & demo : ###

./src/lechod -c doc/rc.lechod.ini

required libevbase 0.0.14 from http://sbase.googlecode.com/files/libevbase-0.0.15.tar.gz

NOTE: if you want to build rpm , look at doc/libsbase.spec