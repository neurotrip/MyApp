#include "watcher.h"
#include <errno.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/syslog.h>
#include <fcntl.h>
#include <cstring>

int main()
{
	openlog("program", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "LAUNCH");
    int in_fd = 0;
    int out_fd = 0;
    const char *errmsg;
	int inotify_fd = 0;
    int wd = 0;
	
	for (;;) {
		inotify_fd = inotify_init();
        if (inotify_fd == -1) {
            perror("inotify_init");
            errmsg = strerror(errno);
            syslog(LOG_ERR, "inotify_init | %s",errmsg);
            break;
        }

        if ((in_fd = open("/home/alex/MyApp/TestApp_C++/tmp/indata.txt", O_RDONLY)) == -1) {
            perror("open indata.txt");
            errmsg = strerror(errno);
            syslog(LOG_ERR, "open indata.txt | %s", errmsg);
            break;
        }

        wd = inotify_add_watch(inotify_fd, "/home/alex/MyApp/TestApp_C++/tmp/", IN_MODIFY);
        if (wd == -1) {
            perror("inotify_add_watch");
            errmsg = strerror(errno);
            syslog(LOG_ERR, "inotify_add_watch | %s", errmsg);
            break;
        }

        if ((out_fd = open("/home/alex/MyApp/TestApp_C++/tmp/outdata.txt", O_RDWR | O_TRUNC| O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
            perror("open outdata.txt");
            errmsg = strerror(errno);
            syslog(LOG_ERR, "open outdata.txt | %s", errmsg);
            break;
        }
		
		Watcher watcher(inotify_fd, in_fd, out_fd);
		watcher.create_threads();
		watcher.join_threads();
		break;

	}

	inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    if (close(out_fd) == -1) {
        perror("close out_fd");
        errmsg = strerror(errno);
        syslog(LOG_ERR, "close out_fd | %s", errmsg);
        exit(EXIT_FAILURE);
    }
    if (close(in_fd) == -1) {
        perror("close in_fd");
        errmsg = strerror(errno);
        syslog(LOG_ERR, "close in_fd | %s", errmsg);
        exit(EXIT_FAILURE);
    }
    if (remove("/home/alex/MyApp/TestApp_C++/tmp/outdata.txt") == -1) {
        errmsg = strerror(errno);
        syslog(LOG_ERR, "remove outdata.txt | %s", errmsg);
        perror("remove outdata.txt");
        exit(EXIT_FAILURE);
    }
    printf("PROGRAM CLOSE!\n");

    syslog(LOG_INFO, "CLOSE");
    exit(EXIT_SUCCESS);
}
