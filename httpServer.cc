#include <iostream>
#include "httpServer.hpp"

// ./httpServer 8080
static void usage(std::string proc){
	std::cout << "usage:" << proc << " port" << std::endl;
} 
int main(int argc, char *argv[]){
	if(argc != 2){
		usage(argv[0]);
		exit(1);
	}
	httpServer* ser = new httpServer(atoi(argv[1]));
	ser->initServer();
	ser->start();
	delete ser;
	return 0;
}
