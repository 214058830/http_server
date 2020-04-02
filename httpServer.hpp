#pragma once

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "protocolUtil.hpp"
#include "threadPool.hpp"

class httpServer{
public:
	httpServer(int port_)
		:port(port_),
		listen_sock(-1)
	{}
	void initServer(){
		listen_sock = socketApi::_socket();
		socketApi::_bind(listen_sock, port);
		socketApi::_listen(listen_sock);
	}
	void start(){
		signal(SIGPIPE, SIG_IGN);//忽略信号 防止服务端关闭
		while(1){
			std::string peer_ip; //读取客户端的信息
			int peer_port;
		
			int sock = socketApi::_accept(listen_sock, peer_ip, peer_port);
			if(sock >= 0){
			//	std::cout << "get a new tcp client:" << "[" << peer_ip << "][" << peer_port << "]"<< std::endl;
				task t(sock, entry::handlerRequest);
				singleton::getInstance()->pushTask(t);
			}
		}
	}
	~httpServer(){
		if(listen_sock >= 0){
			close(listen_sock);
		}
		
	}
private:
	int listen_sock;
	int port;
};
