#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sched.h>
#include <signal.h>

#define MAX 3000
#define MAX_EVENTS 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (MAX_EVENTS * (EVENT_SIZE + 16))
#define LESS_FINISHED_SIGNAL SIGUSR1

struct ThreadArgs {
    int inotify_fd;
    int in_fd;
    int out_fd;
	pthread_t tfw_thread;
};

void handler(int sig) {
	pthread_exit(NULL);
}

static void *threadLessfunc(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    system("/usr/bin/less +F /home/code/TestApp/tmp/outdata.txt");


    printf("Less done!\n");
	pthread_kill(args->tfw_thread, LESS_FINISHED_SIGNAL);
    pthread_exit(NULL);
}

static void *threadFileWatcher(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    const char *errmsg;
    char buffer[MAX];
    ssize_t bytes_read;
    struct inotify_event *event;
	signal(LESS_FINISHED_SIGNAL, handler);
	
	for (;;)
    {
        char event_buf[BUF_LEN];
        int num_bytes = read(args->inotify_fd, event_buf, BUF_LEN);
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
                bytes_read = read(args->in_fd, buffer, MAX);
                if (bytes_read == -1)
                {
                    perror("read indata.txt");
                    errmsg = strerror(errno);
                    syslog(LOG_ERR, "read in_fd | %s", errmsg);
                    exit(EXIT_FAILURE);
                }

                if (write(args->out_fd, buffer, bytes_read) == -1)
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
    printf("Program done!\n");

    pthread_exit(NULL);
}

int main()
{
    openlog("program", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "LAUNCH");
    int in_fd = 0;
   	int	out_fd = 0;
    const char *errmsg;
    pthread_t less_thread, file_watcher_thread;
    int s_less,s_watcher;
    struct ThreadArgs thread_args;
	int inotify_fd = 0;
	int wd = 0;

	pthread_attr_t attr_lt, attr_fwt;
	pthread_attr_init(&attr_lt);
	pthread_attr_init(&attr_fwt);

	cpu_set_t cpuset1;
	CPU_ZERO(&cpuset1);
	CPU_SET(1, &cpuset1);
	
	cpu_set_t cpuset2;
	CPU_ZERO(&cpuset2);
	CPU_SET(2, &cpuset2);

	pthread_attr_setaffinity_np(&attr_lt, sizeof(cpu_set_t), &cpuset1);
    pthread_attr_setaffinity_np(&attr_fwt, sizeof(cpu_set_t), &cpuset2);

	for (;;) 
	{
		inotify_fd = inotify_init();
		if (inotify_fd == -1) {
			perror("inotify_init");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "inotify_init | %s",errmsg);
			break;
		}

		if ((in_fd = open("/home/code/TestApp/tmp/indata.txt", O_RDONLY)) == -1) {
			perror("open indata.txt");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "open indata.txt | %s", errmsg);
			break;
		}

		wd = inotify_add_watch(inotify_fd, "/home/code/TestApp/tmp/", IN_MODIFY);
		if (wd == -1) {
			perror("inotify_add_watch");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "inotify_add_watch | %s", errmsg);
			break;
		}

		if ((out_fd = open("/home/code/TestApp/tmp/outdata.txt", O_RDWR | O_TRUNC| O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
			perror("open outdata.txt");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "open outdata.txt | %s", errmsg);
			break;
		}
		thread_args.in_fd = in_fd;
		thread_args.inotify_fd = inotify_fd;
		thread_args.out_fd = out_fd;

		s_less = pthread_create(&less_thread, &attr_lt, threadLessfunc, (void *)&thread_args);
		if (s_less != 0) {
			perror("pthread_create");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "pthread_create s_less | %s", errmsg);
			break;
		}
		s_watcher = pthread_create(&file_watcher_thread, &attr_fwt, threadFileWatcher, (void *)&thread_args);
		if (s_watcher != 0) {
			perror("pthread_create");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "pthread_create s_watcher | %s", errmsg);
			break;
		}
		thread_args.tfw_thread = file_watcher_thread;

		s_less = pthread_join(less_thread, NULL);
		if (s_less != 0) {
			perror("pthread_join");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "pthread_join s_less | %s", errmsg);
			break;
		}

		s_watcher = pthread_join(file_watcher_thread, NULL);
		if (s_watcher != 0) {
			perror("pthread_join");
			errmsg = strerror(errno);
			syslog(LOG_ERR, "pthread_join s_watcher | %s", errmsg);
			break;
		}
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
    if (remove("/home/code/TestApp/tmp/outdata.txt") == -1) {
        errmsg = strerror(errno);
        syslog(LOG_ERR, "remove outdata.txt | %s", errmsg);
        perror("remove outdata.txt");
        exit(EXIT_FAILURE);
    }
	printf("PROGRAM CLOSE!\n");

    syslog(LOG_INFO, "CLOSE");
    exit(EXIT_SUCCESS);
}

