#include "watcher.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>

Watcher::Watcher()
{
	_inotify_fd = 0;
	_in_fd = 0;
	_out_fd = 0;
	_indata = nullptr;
	_outdata = nullptr;
	_dirname = nullptr;
	_wd = 0;	
}

Watcher::Watcher(const char *in, const char *out, const char *dirn)
{
	try {
		_indata = std::make_unique<char[]>(strlen(in) +1);
		_outdata = std::make_unique<char[]>(strlen(out) +1);
		_dirname = std::make_unique<char[]>(strlen(dirn) +1);

		strcpy(_indata.get(), in);
    	strcpy(_outdata.get(), out);
    	strcpy(_dirname.get(), dirn);

		 _inotify_fd = inotify_init();
        if (_inotify_fd == -1) {
            throw std::runtime_error("Failed to initialize inotify");
        }

        _in_fd = open(_indata.get(), O_RDONLY);
        if (_in_fd == -1) {
            throw std::runtime_error("Failed to open input file");
        }

        _wd = inotify_add_watch(_inotify_fd, _dirname.get(), IN_MODIFY);
        if (_wd == -1) {
            throw std::runtime_error("Failed to add inotify watch");
        }

        _out_fd = open(_outdata.get(), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        if (_out_fd == -1) {
            throw std::runtime_error("Failed to open output file");
        }
	} catch (const std::runtime_error& e) {
			syslog(LOG_ERR, "Error initializing Watcher: %s", e.what());
			throw;
	}

}	

Watcher::~Watcher()
{
    try {
        if (_wd != 0) {
            inotify_rm_watch(_inotify_fd, _wd);
        }

        if (_inotify_fd != 0) {
            close(_inotify_fd);
        }

        if (_in_fd != 0) {
            close(_in_fd);
        }

        if (_out_fd != 0) {
            close(_out_fd);
            remove(_outdata.get());
        }
    } catch (const std::exception &e) {
        syslog(LOG_ERR, "Error during Watcher destruction: %s", e.what());
    }
};
void Watcher::file_watcher() 
{
    auto buffer = std::make_unique<char[]>(MAX);
    ssize_t bytes_read;
    struct inotify_event *event;

    while (exit_flag != false)
	{
  	try {
            char event_buf[BUF_LEN];
            int num_bytes = read(_inotify_fd, event_buf, BUF_LEN);
            if (num_bytes == -1) {
                throw std::runtime_error("Failed to read inotify_fd");
            }

            int i = 0;
            while (i < num_bytes)
            {
                event = (struct inotify_event *)&event_buf[i];
                if ((event->mask & IN_MODIFY) && (strcmp(event->name, "indata.txt") == 0))
                {
                    bytes_read = read(_in_fd, buffer.get(), MAX);
                    if (bytes_read == -1) {
                        throw std::runtime_error("Failed to read indata.txt");
                    }

                    if (write(_out_fd, buffer.get(), bytes_read) == -1) {
                        throw std::runtime_error("Failed to write to outdata.txt");
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        } catch (const std::runtime_error &e) {
            syslog(LOG_ERR, "%s", e.what());
            break;  // Выйти из цикла в случае ошибки
        }
	}
	std::cout<<"File Watcher done!\n";
}
void Watcher::less() 
{
	system("/usr/bin/less +F /home/alex/MyApp/TestApp_C++/tmp/outdata.txt");
	exit_flag = false;
    printf("Less done!\n");
}

void Watcher::create_threads()
{
	less_thread = std::thread(&Watcher::less, this);
	fw_thread = std::thread(&Watcher::file_watcher, this);
}

void Watcher::join_threads() 
{
    if (less_thread.joinable())
        less_thread.join();
    if (fw_thread.joinable())
        fw_thread.join();
}

