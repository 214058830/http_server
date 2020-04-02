#pragma once
#include<iostream>
#include<strings.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include"log.hpp"
#include<unistd.h>
#include<stdlib.h>
#include <sstream>
#include <string>
#include<vector>
#include<algorithm>
#include<unordered_map>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MESSAGE 1024
#define BACKLOG 5

#define HTML_404 "wwwroot/404.html"
#define WEBROOT "wwwroot"
#define HOMEPAGE "index.html"

class util;
class Connect;
class httpRequest;
class httpResponse;
class entry;

class socketApi{
public:
	static int _socket(){//套接字类型type
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock < 0){
			_log("_socket is error...", ERROR);
			exit(2);
		}
		//防止服务端主动断开进入time_wait状态 其他没法访问 绑定 现在设置可以立即重启，虽然还是会进入time_wait状态
		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		return sock;
	}
	static void _bind(int socket, int port){
		struct sockaddr_in server_socket;
		bzero(&server_socket, sizeof(server_socket));
		server_socket.sin_family = AF_INET;
		server_socket.sin_addr.s_addr = htonl(INADDR_ANY);
		server_socket.sin_port = htons(port);
		if(bind(socket, (struct sockaddr*)&server_socket, sizeof(struct sockaddr_in)) < 0){
			_log("_bind is error...", ERROR);
			exit(3);
		}
	}
	static void _listen(int socket){
		if(listen(socket, BACKLOG) < 0){
			_log("_listen is error...", ERROR);
			exit(4);
		}
	}
	//ip port同时拿出ip和port
	static int _accept(int listen_socket, std::string &ip, int &port){
		struct sockaddr_in client_sockaddr;//要保存的客户端地址
		socklen_t len = sizeof(client_sockaddr);
		int client_socket = accept(listen_socket, (struct sockaddr *)&client_sockaddr, &len);
		if(client_socket < 0){
			_log("_accept is error...", WORNING);
			return -1;
		}
		//用IP 和 PORT保存好客户端的信息传出
		ip = inet_ntoa(client_sockaddr.sin_addr);
		port = ntohs(client_sockaddr.sin_port);
		return client_socket;
	}
	static bool _connect(const int& client_socket, const std::string& server_ip,const int& server_port){
		struct sockaddr_in server_sock;
		//memset(&server_sock, 0, sizeof(server_sock));
		server_sock.sin_family = AF_INET;
		server_sock.sin_addr.s_addr = inet_addr(server_ip.c_str());
		server_sock.sin_port = htons(server_port);
		int ret = connect(client_socket, (struct sockaddr*)&server_sock, sizeof(server_sock));
		if(ret < 0){
			_log("_connect is error...", WORNING);
			return false;
		}
		return true;
	}
};
class util{
public:
	static void makeKv(const std::string& s, std::string& k, std::string& v){
		std::size_t pos = s.find(": ");
		k = s.substr(0, pos);
		v = s.substr(pos+2);
	}
	static std::string intToString(int x){
		std::stringstream ss;
		ss << x;
		return ss.str();
	}
	static std::string codeToDesc(int code){
		switch (code)
		{
			case 404:
				return "Not Found";
			case 200:
				return "OK";
			case 400:
				return "Bad Request";
			case 500:
				return "Internal Server Error";
			default:
				break;
		}
		return "Unknow";
	}
	static std::string codeToExceptFile(int code){
		switch(code){
			case 404:
				return HTML_404;
			defaule:
				return "";
		}
	}
	static std::string suffixToContent(std::string& suffix){
		if(suffix == ".css"){
			return "text/css";
		}
		if(suffix == ".js"){
			return "application/x-javascript";
		}
		if(suffix == ".jpg"){
			return "image/jpg";
		}
		if(suffix == ".gif"){
			return "image/gif";
		}
		if(suffix == ".png"){
			return "image/png";
		}
		return "text/html";
	}
	static int fileSize(std::string& except_path){
		struct stat st;
		stat(except_path.c_str(), &st);
		return st.st_size;
	}
	static short int hexChar2dec(char c) {
		if ( '0'<=c && c<='9' ) {
			return short(c-'0');
		} else if ( 'a'<=c && c<='f' ) {
			return ( short(c-'a') + 10 );
		} else if ( 'A'<=c && c<='F' ) {
			return ( short(c-'A') + 10 );
		} else {
			return -1;
		}
	}

	static std::string deescapeURL(const std::string &URL) {
		std::string result = "";
		for ( unsigned int i=0; i<URL.size(); i++ ) {
			char c = URL[i];
			if ( c != '%' ) {
				result += c;
			} else {
				char c1 = URL[++i];
				char c0 = URL[++i];
				int num = 0;
				num += util::hexChar2dec(c1) * 16 + util::hexChar2dec(c0);
				result += char(num);
			}
		}
		return result;
	}
};
class httpResponse{ //相应
public:
	std::string status_line;
	std::vector<std::string> response_header;
	std::string blank;
	std::string response_text;
	int code;
	
	httpResponse()
		:blank("\r\n")
		,code(200)
	{}
	void setRecourceSize(int rs){
		//std::cout << "updata recource_size:" << rs << std::endl;
		recource_size = rs;
	}
	int recourceSize(){
		return recource_size;
	}
	std::string& path_(){
		return path;
	}
	void setPath(std::string& path_){
		path = path_;
	}
	int& code_(){
		return code;
	}
	void makeStatuseLine(){
		status_line = "HTTP/1.0 ";
		status_line += " ";
		status_line += util::intToString(code);
		status_line += " ";
		status_line += util::codeToDesc(code);
		status_line += "\r\n";
	}
	void makeResponseHeader(){
		std::string line;
		std::string suffix; //后缀
		line = "Content-Type: ";
		std::size_t pos = path.rfind('.');
		if(std::string::npos != pos){ // 默认都可以找到文件格式
			suffix = path.substr(pos);
			transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
		}
		line += util::suffixToContent(suffix);
		line += "\r\n";
		response_header.push_back(line);

		line = "Content-Length: ";
		//std::cout << "202path: " << path << " " << recource_size << std::endl;
		line += util::intToString(recource_size);
		line += "\r\n";
		response_header.push_back(line);

		line = "\r\n";
		response_header.push_back(line);
	}
	~httpResponse(){}
private:
	std::string path;
	int recource_size;
};
class Connect{
public:
	Connect(int sock_):sock(sock_)
	{}
	int recvOneLine(std::string& line_){ //可能会出现只读取出一个空个的情况
		char buff[10240] = {0};
		char c = 'x'; // 初始化
		int i = 0;
		while(c != '\n' && i < 10240-1){
			ssize_t s = recv(sock, &c, 1, 0);
			if(s > 0){
				if(c == '\r'){ //\r\n MAC下的其他特殊换行符
					recv(sock, &c, 1, MSG_PEEK);//拿出来拷贝一份 不真的读出去
					if(c == '\n'){
						recv(sock, &c, 1, 0);
					}
					else{
						c = '\n';
					}
				}
				buff[i++] = c;
			}
			else{
				break;
			}
		}
		buff[i] = '\0';
		line_ = buff;
		return i;
	}
	void recvRequestHeader(std::vector<std::string>& v){
		std::string line = "";
		while(line != "\n"){
			recvOneLine(line);
			std::cout << "[debug]:" << line;
			if(line != "\n"){
				v.push_back(line);
			}
		}
	}
	void recvText(std::string& text, int content_length){
		char c = '\0';
		for(auto i = 0; i < content_length; ++i){
			recv(sock, &c, 1, 0);
			//std::cout << "[debug]:" << c;
			//c = util::hexChar2dec(c);
			///////////////////////////////////////////////////////deescapeURL();
			text.push_back(c);
		}
		text = util::deescapeURL(text);
	}
	void sendStatuseLine(httpResponse* rsp){
		std::string& sl = rsp->status_line;
		send(sock, (void*)sl.c_str(), sl.size(), 0);
		std::cout << "[debug]:" << sl;
	}
	void sendHeader(httpResponse* rsp){ // add \n
		std::vector<std::string>& v = rsp->response_header;
		for(auto it = v.begin(); it != v.end(); ++it){
			send(sock, it->c_str(), it->size(), 0);
			std::cout << "[debug]:" << *it;
		}	
	}
	void sendText(httpResponse* rsp, bool cgi_){
		if(!cgi_){
			std::string& path = rsp->path_();
			int fd = open(path.c_str(), 0, O_RDONLY);
			if(fd < 0){
				_log("open file error..", WORNING);
				return ;
			}
			sendfile(sock, fd, NULL, rsp->recourceSize());
			close(fd);
		}
		else{
			std::string& rsp_text = rsp->response_text;
			send(sock, rsp_text.c_str(), rsp_text.size(), 0);
		}
	}
	void clearRequest(){
		std::string line;
		while(line != "\n"){
			recvOneLine(line);
		}
	}
	~Connect(){
		close(sock);
	}
private:
	int sock;
};

class httpRequest{
public:
	httpRequest():path(WEBROOT),
		cgi(false),
		blank("\r\n"),
		recource_size(0),
		request_text("")
	
	{}
	void requestLineParse(){ //分字符串为每一个字段填充
		std::stringstream ss(request_line);
		ss >> method >> uri >> version; //依赖空格分开 为三个字段
		transform(method.begin(), method.end(), method.begin(), ::toupper); //转小写
	}
	std::string getParam(){
		if(method == "GET"){
			return query_string;
		}
		else{
			return request_text;
		}
	}
	void uriParse(){
		if(method == "GET"){
			std::size_t pos = uri.find('?');
			if(pos != std::string::npos){ //有参数
				cgi = true;
				path += uri.substr(0, pos);
				query_string = uri.substr(pos+1); //一直截取到结尾
			}
			else{ // 无参数
				path += uri; //wwwroot/a/b/c
			}
		}
		else{
			cgi = true;
			path += uri; //POST方法参数在征文中
		}
		if(path[path.size() - 1] == '/'){
			path += HOMEPAGE; //如果请求根目录的情况 wwwroot/
		}
	}
	void headerParse(){
		for(auto it = request_header.begin(); it != request_header.end(); ++it){
			std::string k, v;
			util::makeKv(*it, k, v);
			header_kv.insert({k, v});
		}
	}
	bool isMethodLegal(){
		if(method != "GET" && method != "POST"){
			return false;
		}
		return true;
	}
	int isPathLegal(httpResponse* rsp){ //  wwwroot/a/b/cd.html
		struct stat st;
		int rs = 0;
		if(stat(path.c_str(), &st) < 0){
			// 异常处理
			return 404;
		}
		else{
			rs = st.st_size;
			if(S_ISDIR(st.st_mode)){ //判断文件是否是目录格式
				path += "/";
				path += HOMEPAGE;
				stat(path.c_str(), &st);
				rs = st.st_size;
			}
			else if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH){
				//可执行文件
				cgi = true;
			}
		//	else{
		//		//请求的资源存在且非可执行文件
		//	}
		}
		//std::cout << "rs: " << rs << std::endl;
		rsp->setRecourceSize(rs);
		rsp->setPath(path);
		//std::cout << "382recourse_size():  " << rsp->recourceSize() << std::endl;
		return 200; //正常退出码
	}
	bool isNeedRecv(){
		return method == "POST" ? true : false;
	}
	int contentLength(){
		int content_length = -1;
		std::string cl = header_kv["Content-Length"];
		std::stringstream ss(cl);
		ss >> content_length;
		return content_length;
	}
	bool isCgi(){
		return cgi;
	}
	
	~httpRequest()
	{}
	//基本协议字段
	std::string request_line;
	std::vector<std::string> request_header;
	std::string blank;
	std::string request_text;
private:
	//解析字段
	std::string method;
	std::string uri; //path?arg
	std::string version;
	std::string path; //请求文件所在路径
	int recource_size;
	std::string query_string; //参数
	std::unordered_map<std::string, std::string> header_kv;
	bool cgi; //参数处理
};

class entry{
public:
	static int processCgi(Connect* conn, httpRequest* rq, httpResponse* rsp){
		std::string bin = rsp->path_();
		//std::cout << "bin :" << bin << std::endl;
		std::string param = rq->getParam();
		//std::cout << "param :" << param << std::endl;
		std::string param_size = "CONTENT-LENGTH=";
		int size = param.size();
		param_size += util::intToString(size);
		std::cout << param_size << std::endl;
		std::string& response_text = rsp->response_text;

		//std::cout << "----------------" << std::endl;
		int input[2];
		int output[2];
		pipe(input);
		pipe(output);
		pid_t id = fork();
		if(id < 0){
			_log("fork error", WORNING);
			return 503;
		}
		else if(id == 0){
			//exec + ipc
			close(input[1]);
			close(output[0]);
			putenv((char*)param_size.c_str());
			dup2(input[0], 0);
			dup2(output[1], 1);

			execl(bin.c_str(), bin.c_str(), NULL);
			exit(1);
		}
		else{
			//wait
			close(input[0]);
			close(output[1]);
			char c;

			//std::cout << "----------------" << std::endl;
			for(auto i = 0; i < size; ++i){
				c = param[i];
				write(input[1], &c, 1);
			}
			//std::cout << "----------------" << std::endl;
			//std::cout << "waitpid " << std::endl;
			waitpid(id, NULL, 0);
			//std::cout << "waitpid is ok" << std::endl;
			while(read(output[0], &c, 1) > 0){
				response_text.push_back(c);
			}

			//std::cout << "----------------" << std::endl;
			rsp->makeStatuseLine();
			rsp->setRecourceSize(response_text.size());
			rsp->makeResponseHeader();
			
			conn->sendStatuseLine(rsp); //发送状态行
			conn->sendHeader(rsp); // 发送头部
			conn->sendText(rsp, true); // 发送正文
		}
		return 200;
	}
	static int processNonCgi(Connect* conn, httpRequest* rq, httpResponse* rsp){
		//std::cout << "------------" << std::endl;
		rsp->makeStatuseLine(); //相应状态行
		rsp->makeResponseHeader(); //响应头部

		conn->sendStatuseLine(rsp);
		conn->sendHeader(rsp); // 发送头部
		conn->sendText(rsp, false); // 发送正文
		return 200;
	}
	static int processResponse(Connect* conn, httpRequest* rq, httpResponse* rsp){
		if(rq->isCgi()){
			return processCgi(conn, rq, rsp);
		}
		else{
			return processNonCgi(conn, rq, rsp);
		}
	}
	static void handlerRequest(int sock){
		pthread_detach(pthread_self());
		Connect* conn = new Connect(sock);
		httpRequest *rq = new httpRequest;
		httpResponse *rsp = new httpResponse;
		int& code = rsp->code_();

		conn->recvOneLine(rq->request_line);
		std::cout << "[debug]:" << rq->request_line;
		rq->requestLineParse(); //填充 mothed uri version
		if(!rq->isMethodLegal()){ // 判断方法合法性
			code = 400;
			conn->clearRequest();
			_log("request method error", ERROR);
			goto end;
		}
		rq->uriParse(); //填充path文件地址 和 quiery_string参数
		if((code = rq->isPathLegal(rsp)) != 200){
			conn->clearRequest();
			_log("file is not exist", WORNING);
			goto end;
		}

		conn->recvRequestHeader(rq->request_header); //读取到了包头 放入了request_header blank部分没有放入
		//std::cout << "----------------" << std::endl;
		rq->headerParse(); // 把request_header中的键值对放入kv数组中
		if(rq->isNeedRecv()){ // 判断是不是POST方法
			conn->recvText(rq->request_text, rq->contentLength()); // 接受正文
			std::cout << "[debug]:" << rq->request_text << std::endl;
		}
		//std::cout << "next is response" << std::endl;
		code = processResponse(conn, rq, rsp); //回复响应
	end:
		if(code != 200){
			std::string except_path = util::codeToExceptFile(code);
			int rs = util::fileSize(except_path);
			rsp->setPath(except_path);
			rsp->setRecourceSize(rs);
			processNonCgi(conn, rq, rsp);
		}

		delete conn;
		delete rq;
		delete rsp;
	}
};
