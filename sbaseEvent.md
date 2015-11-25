# 服务器的架构模式 #
> 服务启动,主线程启动一定数目的子线程,每个子线程有一个消息队列,用来存放各种类型的消息。每个线程是一个循环,主要做2件事,一是等待事件发生,一是对消息队列进行处理。

# 详细 #
> libevbase作为服务器的事件驱动库,开发的目的是为了封装不同操作系统的io事件操作，如select,epoll等
使得所有的事件处理模式为： 1. 注册一个事件(event)的处理操作 2. 将此事件添加到事件监听器(evbase)中
例如:
```
 con->evbase=pth->evbase;
 con->event->set(event,READ,event_handler); 
 pth->evbase->add(pth->evbase,con->event);
```
> 当读事件发生时,便激活event\_handler函数,这个激活不是signal,实际上是遍历所有的可读fd,看哪个被激活了

> 服务器的工作由多个线程完成(Procthread.c),线程函数的伪代码如下:
```
  while(run){
  event_loop();
  message_handler();
  }
```

  * event\_loop
> event\_loop是事件监听函数,内部实现的原理是select等待任意一个事件发生后,激活该事件注册的事件函数。代码如下:
```
   for(i<maxfd;i++)
    {
      if(FD_ISSET(i, &rd_set))flags |= E_READ;
      if(FD_ISSET(i, &wr_set))flags |= E_WRITE;
      evlist[i]->active();
    }
```

> 说明:rd\_set和wr\_set中的文件描述符fd已经被select设置为有效,接下来检查从0到maxfd的所有fd,找出第一个read或者write有效的fd,然后调用此fd注册的事件处理函数。这里,用到一个技巧,evlist是一个以文件描述符的值为下标的事件数组,当fd=i可读或可写,就以i从事件数组中取出相应的事件,调用此事件的active函数,也就是激活此事件对应的处理程序。事件注册方式类似libevent。

  * message\_handler

> 事件处理完成后(可能添加消息到队列,也可能添加新增的套接字连接到主线程等),进入消息处理过程。每次从队列弹出一个消息
```
 QUEUE_POP(message_queue, MESSAGE, &msg);
```
> 之后进入消息类型判断：


> 根据不同的消息类型,进入不同的处理函数。判断消息类型的主要代码如下:
```
     switch(msg.msg_id)
        {
            case MESSAGE_NEW_SESSION :
                add_connection();
                break;
            case MESSAGE_QUIT :
                terminate_connection();
                break;
            case MESSAGE_INPUT :
                read_handler();
                break;
            case MESSAGE_OUTPUT :
                write_handler();
                break;
            case MESSAGE_PACKET :
                packet_handler();
                break;
            case MESSAGE_DATA :
                data_handler();
                break;
            case MESSAGE_TRANSACTION :
                transaction_handler();
                break;
            case MESSAGE_TIMEOUT :
                timeout_handler();
                break;
        
```

> 之后便是对各种数据进行处理的过程。