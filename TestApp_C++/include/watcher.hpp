#ifndef WATHCER_H_
#define WATHCER_H_
#include <thread>
#include <memory>
#include <atomic>
#include "sys/inotify.h"
#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <vector>

class FileDescriptor {
	private:
		int _fd;
	public:
		FileDescriptor(); 
		FileDescriptor(int fd);
		~FileDescriptor();	
		FileDescriptor(const FileDescriptor&) = delete;
		FileDescriptor& operator=(const FileDescriptor&) = delete;
		FileDescriptor& operator=(FileDescriptor&& other) noexcept;
		int get() const;
};

class InotifyWatch {
    private:
        int _inotify_fd;
        int _wd;
    public:
        InotifyWatch();  
        InotifyWatch(int inotify_fd, const std::string& dir);  
        ~InotifyWatch();
        InotifyWatch(const InotifyWatch&) = delete;
        InotifyWatch& operator=(const InotifyWatch&) = delete;
        InotifyWatch& operator=(InotifyWatch&& other) noexcept;
};

class Watcher {
	private:
    	static constexpr int MAX = 3000;
    	static constexpr int MAX_EVENTS = 1024;
    	static constexpr int EVENT_SIZE = sizeof(struct inotify_event);
    	static constexpr int BUF_LEN = MAX_EVENTS * (EVENT_SIZE + 16);
		FileDescriptor _inotify_fd;
		FileDescriptor _in_fd;
		FileDescriptor _out_fd;
		InotifyWatch _wd;
		int _num_bytes;
		ssize_t _bytes_read;
		int _poll_timeout;
		int _poll_result;
		struct pollfd _fds;
		std::atomic<bool> _exit_flag;
		std::vector<char>_buffer;
    	std::vector<char> _event_buf;
    	std::string _indata;
    	std::string _outdata;
    	std::string _dirname;

		std::thread less_thread;
		std::thread fw_thread;	
		void cleanup() noexcept;
 	public:
		Watcher();
		Watcher(const std::string &in, const std::string& out, const std::string &dirn);
		Watcher(const std::string& in, const std::string& out, const std::string& dirn, bool start_threads);
		~Watcher() noexcept;
		
		void file_watcher();
		void less();
		void join_threads();
};

#endif
