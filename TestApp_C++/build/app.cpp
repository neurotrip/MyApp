#include "watcher.h"
#include <errno.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <fcntl.h>
#include <cstring>

int main(int argc, char *argv[])
{
	if (argc < 4) {
        syslog(LOG_ERR, "Usage: %s <input_file> <output_file> <directory>", argv[0]);
        return EXIT_FAILURE;
    }
	openlog("program", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "LAUNCH");
	
	std::unique_ptr<char[]> indata = std::make_unique<char[]>(strlen(argv[1]) + 1);
	std::unique_ptr<char[]> outdata = std::make_unique<char[]>(strlen(argv[2]) + 1);
	std::unique_ptr<char[]> dirname = std::make_unique<char[]>(strlen(argv[3]) + 1);

	strcpy(indata.get(), argv[1]);
    strcpy(outdata.get(), argv[2]);
    strcpy(dirname.get(), argv[3]);

	
	Watcher watcher(indata.get(), outdata.get(), dirname.get());
	watcher.create_threads();
	watcher.join_threads();
		

    exit(EXIT_SUCCESS);
}
