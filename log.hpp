#pragma once
#include <iostream>
#include <string>

#define NORMAL 0
#define WORNING 1
#define ERROR 2

const char *log_level[] = {
	"NORMAL",
	"WORNING",
	"ERROR",
	NULL,
};//把等级与意思对应起来vector等也可以
//file line 用系统自带变量代替 防止参数过多 _FILE_ _LINE_ gettimetoday
void _log(std::string msg, int level, std::string file, int line){
	std::cout << '[' << log_level[level] << ']' << '[' << msg << ']' << file << ':' << line << std::endl;
}
#define _log(msg, level) _log(msg, level, __FILE__, __LINE__)
