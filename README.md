#  Linux下的简易命令行聊天程序

> ## 已有功能

- [x] 聊天室用户信息收发
- [x] 用户昵称设置
- [x] 新用户上线提醒
- [x] 查询当前所有在线用户（客户端输入`/online`命令触发）

> ## 实现原理

### 服务器：
1. 开启监听套接字并加入epoll事件表
2. 循环调用`epoll_wait()`：
  ①若是listenfd监听套接字，则将accpet得到的连接套接字clientfd加入epoll事件表；
  ②若是clientfd套接字，则用`recv()`读取message并按逻辑要求调用send()发送响应消息

### 客户端
1. 创建sock连接套接字并调用`connect()`连接服务器
2. 连接成功后创建父子进程间的通信管道(`pipe(pipe_fd[2])`)
3. 将连接套接字和pipe_fd[0]放入epoll事件表
4. 调用`fork()`创建子进程
5. 父进程中循环调用`epoll_wait()`：
	①若是sock连接套接字，则打印`recv()`收到的message；
	②若是pipe_fd[0]管道套接字，则读取其内容并调用`send()`发送给服务器
6. 子进程中等待标准输入，输入完毕写入pipe_fd[1]供父进程读取