#include <iostream>       // std::cout
#include <thread>         // std::thread
extern "C"
{
#include <fileserver_example.h>
}

int main(int argc, char** argv)
{
        int port=7777;
	std::thread httpThread(fileserver, port);
	std::cout << "main, mesos and http now execute concurrently...\n";

	// synchronize threads:
	httpThread.join();               // pauses until first finishes

	std::cout << "Thread completed.\n";
	std::getchar();
	return 0;
}

