#include "../include/watcher.hpp"

int main(int argc, char *argv[])
{
	openlog("program", LOG_PID, LOG_USER);
    syslog(LOG_INFO, "LAUNCH");

	int mode = 0;
	std::string indata;
	std::string outdata;
	std::string dirname;

	std::cout << "Enter programm mode: " << std::endl;
	std::cout << "Default(0)/Custom(1) " << std::endl;
	std::cin >> mode;
	Watcher* watcher = nullptr;
	if (mode == 0) {
		watcher = new Watcher();
	} else if (mode == 1) {
		std::cout << "Enter <input_file> " << std::endl;
		std::cin >> indata;
		std::cout << "Enter <output_file> " << std::endl;
		std::cin >> outdata;
		std::cout << "Enter <directory_name> " << std::endl;
		std::cin >> dirname;
		std::cout << "Input file: " << indata << std::endl;
		std::cout << "Output file: " << outdata << std::endl;
		std::cout << "Directory: " << dirname << std::endl;
		watcher = new Watcher(indata, outdata, dirname, true);
	}

	if (watcher) {
		watcher->join_threads();
		delete watcher;
	}

	exit(EXIT_SUCCESS);
}
