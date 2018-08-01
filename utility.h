#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED
#include <iostream>
#include <list>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;
 
// clients_list save all the clients's socket
list<int> clients_list;
unordered_map<int, string> clients_alias_map;
 
/**********************   macro defintion **************************/
// server ip
#define SERVER_IP "127.0.0.1"
 
// server port
#define SERVER_PORT 8888
 
//epoll size
#define EPOLL_SIZE 5000
 
//message buffer size
#define BUF_SIZE 0xFFFF
 
#define SERVER_WELCOME "Welcome to the chat room! Your chat ID is: Client #%d! Your chat alias is: %s"
 
#define SERVER_MESSAGE "%s says : %s"

#define SERVER_ONLINE_INFORM "%s is online now!"

#define SERVER_ONLINE_QUERY "Except you, %d person online : %s"
 
// exit
#define EXIT "/EXIT"

// query user online
#define QUERY_ONLINE "/ONLINE"
 
#define CAUTION "There is only you in the char room!"
 
/**********************   some function **************************/
/**
  * @param sockfd: socket descriptor
  * @return 0
**/
int setnonblocking(int sockfd)
{
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)| O_NONBLOCK);
    return 0;
}
 
/**
  * @param epollfd: epoll handle
  * @param fd: socket descriptor
  * @param enable_et : enable_et = true, epoll use ET; otherwise LT
**/
void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
    // printf("fd added to epoll!\n\n");
}
 
/**
  * @param clientfd: socket descriptor
  * @return : len
**/
int work(int clientfd)
{
    // buf[BUF_SIZE] receive new chat message
    // message[BUF_SIZE] save format message
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);
 
    // receive message
    printf("read from client(clientID = %d)\n", clientfd);
    int len = recv(clientfd, buf, BUF_SIZE, 0);
 
    if(len == 0)  // len = 0 means the client closed connection
    {

        close(clientfd);
        clients_list.remove(clientfd); 
        clients_alias_map.erase(clientfd);//server remove the client
        printf("ClientID = %d closed.\n now there are %d client in the char room\n", clientfd, (int)clients_list.size());
 
    }
    else  // broadcast message / query user online / set alias
    {
   /*      if(clients_list.size() == 1) { // this means There is only you int the char room
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        } */

        if(clients_alias_map.count(clientfd) == 0){ // store alias for user
            string alias(buf, strlen(buf));
            clients_alias_map.insert(pair<int, string>(clientfd, alias));

            // 服务端发送欢迎信息  
            printf("welcome message\n");                
            sprintf(message, SERVER_WELCOME, clientfd, (char*)alias.c_str());
            int ret = send(clientfd, message, BUF_SIZE, 0);
            if(ret < 0) { perror("send error"); exit(-1); }

            // online inform
            bzero(message, BUF_SIZE);
            sprintf(message, SERVER_ONLINE_INFORM, (char*)alias.c_str());
            unordered_map<int, string>::iterator it;
            for(it = clients_alias_map.begin(); it != clients_alias_map.end(); ++it) {
                if(it->first != clientfd){
                    if( send(it->first, message, BUF_SIZE, 0) < 0 ) { perror("error"); exit(-1);}
                }
            }
        }
        else{
            // query all the user online
            if(strncasecmp(buf, QUERY_ONLINE, strlen(QUERY_ONLINE)) == 0){
                string onlineAlias = "";
                unordered_map<int, string>::iterator it;
                for(it = clients_alias_map.begin(); it != clients_alias_map.end(); ++it) {
                    if(it->first != clientfd) onlineAlias += (it->second + ", ");                        
                }
                onlineAlias.erase(onlineAlias.size() - 2);
                sprintf(message, SERVER_ONLINE_QUERY, clients_alias_map.size()-1, (char*)onlineAlias.c_str());
                if( send(clientfd, message, BUF_SIZE, 0 ) < 0) { perror("error"); exit(-1);}
            }                
            else{
                // format message to broadcast
                sprintf(message, SERVER_MESSAGE, (char*)clients_alias_map[clientfd].c_str(), buf);
 
                unordered_map<int, string>::iterator it;
                for(it = clients_alias_map.begin(); it != clients_alias_map.end(); ++it) {
                    if(it->first != clientfd){
                        if( send(it->first, message, BUF_SIZE, 0) < 0 ) { perror("error"); exit(-1);}
                    }
                }
            }
            
        }
    }
    return len;
}

#endif // UTILITY_H_INCLUDED