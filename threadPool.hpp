#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
typedef void (*handler_t)(int);
class task{
public:
	task(int sock_, handler_t handler_)
		:sock(sock_),handler(handler_)
	{}
	void run(){
		handler(sock);
	}
	~task()
	{}
private:
	int sock;
	handler_t handler;
};
class threadPool{
public:
	threadPool(int num_):num(num_),idle_num(0)
	{
		pthread_mutex_init(&lock, NULL);
		pthread_cond_init(&cond, NULL);
	}
	void initThreadPool(){
		pthread_t tid;
		for(auto i = 0; i < num; ++i){
			pthread_create(&tid, NULL, threadRoutine, (void*)this);
		}
	}
	bool isTaskQueueEmpty(){
		return task_queue.size() == 0? true : false;
	}
	void idle(){
		++idle_num;
		pthread_cond_wait(&cond, &lock);
		--idle_num;
	}
	void lockQueue(){
		pthread_mutex_lock(&lock);
	}
	void unlockQueue(){
		pthread_mutex_unlock(&lock);
	}
	task popTask(){
		task t = task_queue.front();
		task_queue.pop();
		return t;
	}
	void wakeup(){
		pthread_cond_signal(&cond);
	}
	void pushTask(task& t){
		lockQueue();
		task_queue.push(t);
		unlockQueue();
		wakeup();
	}
	static void* threadRoutine(void* arg){
		pthread_detach(pthread_self());
		threadPool* tp = (threadPool*)arg;
		for(;;){
			tp->lockQueue();
			while(tp->isTaskQueueEmpty()){
				tp->idle();
			}
			task t = tp->popTask();
			tp->unlockQueue();
			//std::cout << "task is handler by: " << pthread_self() << std::endl;
			t.run();
		}
	}
	~threadPool(){
		pthread_mutex_destroy(&lock);
		pthread_cond_destroy(&cond);
	}
private:
	int num;
	std::queue<task> task_queue;
	int idle_num;
	pthread_mutex_t lock;
	pthread_cond_t cond; 
};

class singleton{
public:
	static threadPool* getInstance(){ //单例模式 懒汉模式
		if(NULL == p){
			pthread_mutex_lock(&lock);
			if(NULL == p){
				p = new threadPool(5);
				p->initThreadPool();
			}
			pthread_mutex_unlock(&lock);
		}
		return p;
	}
private:
	singleton(){}
	static threadPool* p;
	static pthread_mutex_t lock;
};
pthread_mutex_t singleton::lock = PTHREAD_MUTEX_INITIALIZER;
threadPool* singleton::p = NULL;
