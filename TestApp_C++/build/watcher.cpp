#include "watcher.h"
#include <cstring>
#include <unistd.h>
#include <syslog.h>

Watcher::Watcher(int inotify_fd, int in_fd, int out_fd)
{
	_inotify_fd = inotify_fd;
	_in_fd = in_fd;
	_out_fd = out_fd;
}

void Watcher::file_watcher() 
{
	const char *errmsg;
    char buffer[MAX];
    ssize_t bytes_read;
    struct inotify_event *event;

    while (exit_flag != false)
    {
        char event_buf[BUF_LEN];
        int num_bytes = read(_inotify_fd, event_buf, BUF_LEN);
        if (num_bytes == -1)
        {
            perror("read inotify_fd");
            errmsg = strerror(errno);
            syslog(LOG_ERR, "read inotify_fd | %s", errmsg);
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (i < num_bytes)
        {
            event = (struct inotify_event *)&event_buf[i];
            if ((event->mask & IN_MODIFY) && (strcmp(event->name, "indata.txt") == 0))
            {
                bytes_read = read(_in_fd, buffer, MAX);
                if (bytes_read == -1)
                {
                    perror("read indata.txt");
                    errmsg = strerror(errno);
                    syslog(LOG_ERR, "read in_fd | %s", errmsg);
                    exit(EXIT_FAILURE);
                }

                if (write(_out_fd, buffer, bytes_read) == -1)
                {
                    perror("write outdata.txt");
                    errmsg = strerror(errno);
                    syslog(LOG_ERR, "write outdata.txt | %s", errmsg);
                    exit(EXIT_FAILURE);
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
    printf("File Watcher done!\n");
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

