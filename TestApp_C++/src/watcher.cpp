#include "../include/watcher.hpp"

FileDescriptor::FileDescriptor() 
    : _fd(-1) 
{ }

FileDescriptor::FileDescriptor(int fd) 
    : _fd(fd) 
{
    if (_fd == -1) {
        throw std::runtime_error(std::string("Failed to create file descriptor: ") + strerror(errno));
    }
}

FileDescriptor::~FileDescriptor() 
{
    if (_fd != -1) {
        if (close(_fd) == -1) {
            syslog(LOG_ERR, "Failed to close file descriptor: %s", strerror(errno));
        }
    }
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept 
{
    if (this != &other) {
        if (_fd != -1) {
            close(_fd);
        }
        _fd = other._fd;
        other._fd = -1;
    }
    return *this;
}

int FileDescriptor::get() const 
{ 
    return _fd; 
}

InotifyWatch::InotifyWatch() 
    : _inotify_fd(-1), _wd(-1) 
{
}

InotifyWatch::InotifyWatch(int inotify_fd, const std::string& dir) 
    : _inotify_fd(inotify_fd), _wd(-1) 
{
    _wd = inotify_add_watch(_inotify_fd, dir.c_str(), IN_MODIFY);
    if (_wd == -1) {
        throw std::runtime_error("Failed to add inotify watch for directory: " + dir);
    }
}

InotifyWatch::~InotifyWatch() {
    if (_wd != -1) {
        if (inotify_rm_watch(_inotify_fd, _wd) == -1) {
            syslog(LOG_ERR, "Failed to remove inotify watch: %s", strerror(errno));
        }
    }
}

InotifyWatch& InotifyWatch::operator=(InotifyWatch&& other) noexcept {
    if (this != &other) {
        if (_wd != -1) {
            inotify_rm_watch(_inotify_fd, _wd);
        }
        _inotify_fd = other._inotify_fd;
        _wd = other._wd;
        other._inotify_fd = -1;
        other._wd = -1;
    }
    return *this;
}


Watcher::Watcher(const std::string& in, const std::string& out, const std::string& dirn)
    : _buffer(MAX),
      _event_buf(BUF_LEN),
      _poll_timeout(5000),
      _poll_result(0),
      _exit_flag(false)
{
    _indata = in;
    _outdata = out;
    _dirname = dirn;
    
    _inotify_fd = FileDescriptor(inotify_init());
    if (_inotify_fd.get() == -1) {
        throw std::runtime_error("Failed to initialize inotify: " + std::string(strerror(errno)));
    }

    _in_fd = FileDescriptor(open(_indata.c_str(), O_RDONLY));
    if (_in_fd.get() == -1) {
        throw std::runtime_error("Failed to open input file: " + _indata + " - " + strerror(errno));
    }

    _out_fd = FileDescriptor(open(_outdata.c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR));
    if (_out_fd.get() == -1) {
        throw std::runtime_error("Failed to open output file: " + _outdata + " - " + strerror(errno));
    }

    _wd = InotifyWatch(_inotify_fd.get(), _dirname);
    _fds.fd = _inotify_fd.get(); 
    _fds.events = POLLIN;
}

Watcher::Watcher()
    : Watcher("../tmp/indata.txt", "../tmp/outdata.txt", "../tmp")
{
	less_thread = std::thread(&Watcher::less, this);
	fw_thread = std::thread(&Watcher::file_watcher, this);     
}

Watcher::Watcher(const std::string& in, const std::string& out, const std::string& dirn, bool start_threads)
    : Watcher(in, out, dirn) 
{
	less_thread = std::thread(&Watcher::less, this);
	fw_thread = std::thread(&Watcher::file_watcher, this);    
}

Watcher::~Watcher() noexcept {
    if (less_thread.joinable()) {
        less_thread.join();
    }
    if (fw_thread.joinable()) {
        fw_thread.join();
    }
}

void Watcher::file_watcher()
{
    struct inotify_event *event;
    while (_exit_flag == false)
    {
        try {
    
			_poll_result = poll(&_fds, 1, _poll_timeout);

            if (_poll_result == -1) {
                throw std::runtime_error("Failed to poll inotify_fd");
            } else if (_poll_result == 0) {
                std::cout << "No events, exiting watcher loop.\n";
                break;
            }
        
            _num_bytes = read(_inotify_fd.get(), _event_buf.data(), BUF_LEN);
            if (_num_bytes == 0) {
                break;
            } else if (_num_bytes == -1) {
                std::cout << "Error reading inotify_fd: " << strerror(errno) << std::endl;
                throw std::runtime_error("Failed to read inotify_fd");
            }

            int i = 0;
            while (i < _num_bytes)
            {
                event = (struct inotify_event *)&_event_buf[i];
				if ((event->mask & IN_MODIFY) && (strcmp(event->name, "indata.txt") == 0))
                {
                    _bytes_read = read(_in_fd.get(), _buffer.data(), MAX);
                    if (_bytes_read == -1) {
                        throw std::runtime_error("Failed to read indata.txt");
                    }

                    if (write(_out_fd.get(), _buffer.data(), _bytes_read) == -1) {
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
	system("/usr/bin/less +F ../tmp/outdata.txt");
    _exit_flag = true;
    printf("Less done!\n");
}

void Watcher::join_threads() {
	if (less_thread.joinable()) {
		less_thread.join();
	}
	if (fw_thread.joinable()) {
		fw_thread.join();
	}
}