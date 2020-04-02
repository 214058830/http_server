#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(){
	char size[64] = {0};
	char param[1024] = {0};
	if(getenv("CONTENT-LENGTH")){
		strcpy(size, getenv("CONTENT-LENGTH"));
		int cl = atoi(size);
		int i = 0;
		for(; i < cl; ++i){
			read(0, param + i, 1);
		}
		param[i] = 0;
		std::string buff(param);
		//std::cerr << "buff is:" << buff << std::endl;
		size_t pos = buff.find("fullName=");
		size_t pos2 = buff.find("&email");
		std::string fullName = buff.substr(pos+9, pos2-pos-9);
		pos = buff.find("email=");
		pos2 = buff.find("&subject");
		std::string email = buff.substr(pos+6, pos2-pos-6);
		pos = buff.find("subject=");
		pos2 = buff.find("&message");
		std::string subject = buff.substr(pos+8, pos2-pos-8);
		pos = buff.find("message=");
		pos2 = buff.find("&action");
		std::string message = buff.substr(pos+8, pos2-pos-8);
		std::cout << "<html>";
		std::cout << "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><head/>";
		std::cout << "<h4>" << fullName << "<h4/>";
		std::cout << "<h4>" << email << "<h4/>";
		std::cout << "<h4>" << subject << "<h4/>";
		std::cout << "<h4>" << message << "<h4/>";
		std::cout << "<html/>";
	}
	return 0;
}
