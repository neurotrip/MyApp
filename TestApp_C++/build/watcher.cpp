#include "watcher.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>

Watcher::Watcher() 
    : _buffer(std::make_unique<char[]>(MAX)),
      _event_buf(std::make_unique<char[]>(BUF_LEN)),
      _inotify_fd(-1),
      _in_fd(-1),
      _out_fd(-1),
      _wd(-1),
      _indata(nullptr),
      _outdata(nullptr),
      _dirname(nullptr)
{
}

Watcher::Watcher(const char *in, const char *out, const char *dirn)
    : _buffer(std::make_unique<char[]>(MAX)),
      _event_buf(std::make_unique<char[]>(BUF_LEN)),
      _indata(std::make_unique<char[]>(strlen(in) + 1)),
      _outdata(std::make_unique<char[]>(strlen(out) + 1)),
      _dirname(std::make_unique<char[]>(strlen(dirn) + 1)),
      _inotify_fd(-1),
      _in_fd(-1),
      _out_fd(-1),
      _wd(-1)
{
    try {
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
        syslog(LOG_ERR, "Error in Watcher constructor: %s", e.what());
        cleanup();  
        throw;  
    } catch (...) {
        syslog(LOG_ERR, "Unknown error in Watcher constructor");
        cleanup();  
        throw;  
    }
}

Watcher::~Watcher() noexcept {
    cleanup();
}

void Watcher::cleanup() noexcept {
     try {
        if (_wd != -1) {
            if (inotify_rm_watch(_inotify_fd, _wd) == -1) {
                syslog(LOG_ERR, "Failed to remove inotify watch: %s", strerror(errno));
            }
        }
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Exception during inotify_rm_watch: %s", e.what());
    } catch (...) {
        syslog(LOG_ERR, "Unknown exception during inotify_rm_watch");
    }

    try {
        if (_inotify_fd != -1) {
            if (close(_inotify_fd) == -1) {
                syslog(LOG_ERR, "Failed to close inotify_fd: %s", strerror(errno));
            }
        }
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Exception during close(_inotify_fd): %s", e.what());
    } catch (...) {
        syslog(LOG_ERR, "Unknown exception during close(_inotify_fd)");
    }

    try {
        if (_in_fd != -1) {
            if (close(_in_fd) == -1) {
                syslog(LOG_ERR, "Failed to close in_fd: %s", strerror(errno));
            }
        }
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Exception during close(_in_fd): %s", e.what());
    } catch (...) {
        syslog(LOG_ERR, "Unknown exception during close(_in_fd)");
    }

    try {
        if (_out_fd != -1) {
            if (close(_out_fd) == -1) {
                syslog(LOG_ERR, "Failed to close out_fd: %s", strerror(errno));
            }
            if (remove(_outdata.get()) == -1) {
                syslog(LOG_ERR, "Failed to remove outdata file: %s", strerror(errno));
            }
        }
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Exception during close(_out_fd) or remove(_outdata): %s", e.what());
    } catch (...) {
        syslog(LOG_ERR, "Unknown exception during close(_out_fd) or remove(_outdata)");
    }
}

void Watcher::file_watcher()
{
    ssize_t bytes_read;
    struct inotify_event *event;

    while (exit_flag != false)
    {
        try {
            int num_bytes = read(_inotify_fd, _event_buf.get(), BUF_LEN);
            if (num_bytes == -1) {
                throw std::runtime_error("Failed to read inotify_fd");
            }

            int i = 0;
            while (i < num_bytes)
            {
                event = (struct inotify_event *)&_event_buf[i];
                if ((event->mask & IN_MODIFY) && (strcmp(event->name, "indata.txt") == 0))
                {
                    bytes_read = read(_in_fd, _buffer.get(), MAX);
                    if (bytes_read == -1) {
                        throw std::runtime_error("Failed to read indata.txt");
                    }

                    if (write(_out_fd, _buffer.get(), bytes_read) == -1) {
                        throw std::runtime_error("Failed to write to outdata.txt");
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        } catch (const std::runtime_error &e) {
            syslog(LOG_ERR, "%s", e.what());
            break;
        }
    }
    std::cout << "File Watcher done!\n";
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

