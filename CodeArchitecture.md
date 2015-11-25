# 介绍 #

> 本文分析服务器启动后,一系列的处理过程

# 详细 #

> 在上一篇文章中,介绍了服务器的事件处理方式:
> http://code.google.com/p/sbase/wiki/sbaseEvent
> 本篇详细解析服务器与客户端交互过程。

> Main函数位于Hibased.c,首先是一些全局静态变量
```
    static SBASE *sbase = NULL;
    static SERVICE *hibase = NULL;
```
> 一个SERVICE对应一个服务进程。调用sbase\_initialize进行一系列初始化操作,生成hibase以及sbase对象等,以及设置静态service里session成员的处理函数,如packet\_handler,packet\_reader,另外还调用set函数来对service初始化连接池,并在后面监听accept事件。
```
     service_set(service)
     {
     //生成连接池
     service->connections = (CONN **)calloc(size);
     service->fd = socket();
     fcntl(O_NONBLOCK);
     bind();
     listen();
     }
```
> 初始化完毕后,运行sbase->running并fork后,对sbase的事件对象初始化,这个事件对象是专门针对新增加的连接,随即进入服务器处理,来到service.c:
```
   if((service = sbase->services[i]))
       {
        service->evbase = sbase->evbase;
        service->run(service);
       }
```
> 在没有生成新线程之前,主线程开始了事件循环和消息队列的处理。
> 下面来看service的run做了什么工作。
> 在service\_run函数里,Service捕捉读事件,也就是accept事件:
```
   service->event->set(service->event, service->fd, E_READ|E_PERSIST,
     (void *)service, (void *)&service_event_handler);
   service->evbase->add(service->evbase, service->event);
```
> 上面代码的意思是,当套接字发生读消息后,将会调用service\_event\_handler函数,并将set函数倒数第2个值service作为参数传递进去。事件的调用底层实现参考服务器的事件处理方式:
> http://code.google.com/p/sbase/wiki/sbaseEvent
> 继续,service开始启动n个线程工作:
```
    for(i <n;i++){
        service->procthreads[i] = procthread_init();
        PROCTHREAD_SET(service, service->procthreads[i]);
        NEW_PROCTHREAD(service->procthreads[i]);
       }
```
> 在线程初始化过程中,每个线程都新生成一个消息队列,同时新生成一个事件管理类evbase,都作为线程内共享成员。
> 线程函数在Procthread.c中定义,主要的循环机制在上一篇已有介绍:
```
   while(pth->running_status)
     {
        pth->evbase->loop(pth->evbase, 0, NULL);
        message_handler(pth->message_queue, pth->logger);
     }
```
> 服务器启动的时候的时候,如果这个时候有客户连接上来,在主线程触发accept事件,激活service\_event\_handler处理函数,此函数将accept出来的套接字conn,设置session处理函数为service的session处理函数后,将conn加到位于线程数组的某个线程中,index下标由 fd%nprocthreads 决定
```
     conn->set_session(conn, session);
     msg.msg_id      = MESSAGE_NEW_SESSION;
     QUEUE_PUSH(pth->message_queue, MESSAGE, &msg);
```
> 此时,线程的消息队列里的消息类型全部为MESSAGE\_NEW\_SESSION
> 进入message\_handler处理函数。
```
     QUEUE_POP(message_queue, MESSAGE, &msg);
     case MESSAGE_NEW_SESSION :
     pth->add_connection(pth, conn);
```
> 于是调用线程的添加连接函数,将此连接的消息队列指向线程的公共队列,并设置连接的监听事件为读。
```
   conn->message_queue = pth->message_queue;
   conn->event->set(conn->event, conn->fd, flag, (void *)conn, &conn_event_handler);
   conn->evbase->add(conn->evbase, conn->event);
```
> 之后将此连接加到service的连接池中。
```
   pth->service->pushconn(pth->service, conn);
```
> 再次进入事件监听循环,下一次可读的时候,是用户发送了HTTP请求,触发上述已连接套接字的conn\_event\_handler事件处理函数,并相继调用Conn.c的read\_handler(conn),conn\_read\_handler(conn),开始读取客户数据:
```
    MB_READ(conn->buffer, conn->fd);
```
> 这条语句是把fd代表的套接字的数据读到buffer里,关于buffer的初始化,在conn初始化时设置最大buffer长度
```
    MB_INIT(conn->buffer, MB_BLOCK_SIZE);
```
> 而在MB\_READ宏里面对buffer初始或者调整分配了大小,接受完毕后调用conn->packet\_reader(conn)。
> 在conn->packet\_reader函数里:
```
    data = PCB(conn->buffer);
    conn->session.packet_reader(conn, data);
```
> 之前,session的packet\_reader已被设置为Hibase.c的hibase\_packet\_reader函数,返回正确值,最后执行压队列操作,此时conn的packet已经是客户的Http请求头,该消息标识为MESSAGE\_PACKET:
```
   MB_PUSH(conn->packet,MB_DATA(conn->buffer),len);
   conn->push_message(conn,MESSAGE_PACKET);
```
> 事件监听结束,进入消息队列处理message\_handler
> 假设弹出的消息正好是刚才压入的MESSAGE\_PACKET类型,依次进入
```
  conn_packet_handler();
  conn->session.packet_handler(conn, PCB(conn->packet));
  hibase_packet_handler(CONN *conn, CB_DATA *packet);
```
> 解析Http请求头:
```
  http_request_parse(p, end, &http_req);
```
> p是接受到的http请求包,end是数据区的尾位置。到Http.c的http\_request\_parse函数,读取http请求头的各个字段的值,最重要的是找出请求的path路径,因为里面含有用户的查询词。
```
    ps = http_req->path;
    for(i< HTTP_HEADER_NUM;i++)
       if(strncmp(s,http_headers[i].e,http_headers[i].elen) 
       {
         http_req->headers[i] = s;
       }   
```
