#ifndef WATHCER_H_
#define WATHCER_H_
#include <thread>
#include <atomic>
#include "sys/inotify.h"

class Watcher {
	private:
	   	static const int MAX = 3000;
    	static const int MAX_EVENTS = 1024;
    	static const int EVENT_SIZE = sizeof(struct inotify_event);
    	static const int BUF_LEN = MAX_EVENTS * (EVENT_SIZE + 16);	
		int _inotify_fd;
		int _in_fd;
		int _out_fd;
		std::thread less_thread;
		std::thread fw_thread;
		std::atomic<bool> exit_flag{true};	
 	public:
		Watcher(int inotify_fd = 0, int in_fd = 0, int out_fd = 0);
		
		void file_watcher();
		void less();
		void create_threads();
		void join_threads();
};

#endif
