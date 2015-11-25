libevbase作为服务器的事件驱动库,开发的目的是为了封装不同操作系统的io事件操作，如select,epoll等

使得所有的事件处理模式为：

1.  注册一个事件(event)的处理操作       2. 将此事件添加到事件监听器(evbase)中

例如:

```
con->event->set(event,READ,event_handler); 
//con里有fd,代表已连接的套接字,且con->evbase=pth->evbase;
pth->evbase->add(pth->evbase,con->event);
```

当读事件发生时,便激活event\_handler函数,这个激活不是signal,实际上是遍历所有的可读fd,看哪个被激活了