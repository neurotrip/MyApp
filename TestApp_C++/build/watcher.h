#ifndef WATHCER_H_
#define WATHCER_H_
#include <thread>
#include <memory>
#include <atomic>
#include "sys/inotify.h"

class Watcher {
	private:
    	static constexpr int MAX = 3000;
    	static constexpr int MAX_EVENTS = 1024;
    	static constexpr int EVENT_SIZE = sizeof(struct inotify_event);
    	static constexpr int BUF_LEN = MAX_EVENTS * (EVENT_SIZE + 16);
		int _inotify_fd;
		int _in_fd;
		int _out_fd;
		int _wd;
		std::unique_ptr<char[]> _buffer;
    	std::unique_ptr<char[]> _event_buf;
    	std::unique_ptr<char[]> _indata;
    	std::unique_ptr<char[]> _outdata;
    	std::unique_ptr<char[]> _dirname;

		std::thread less_thread;
		std::thread fw_thread;
		std::atomic<bool> exit_flag{true};	
		void cleanup() noexcept;
 	public:
		Watcher();
		Watcher(const char *in, const char *out, const char *dirn);
		~Watcher() noexcept;
		
		void file_watcher();
		void less();
		void create_threads();
		void join_threads();
};

#endif
