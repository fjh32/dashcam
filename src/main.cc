#include <iostream>
#include <signal.h>
#include <memory>
#include <execinfo.h>
#include <csignal>
#include <unistd.h>

#include "CamService.h"
#include "RecordingPipeline.h"
using namespace std;

unique_ptr<CamService> camservice;
void catch_signal(int signum);
void crashHandler(int sig);

int main(int argc, char* argv[]) {
    camservice = make_unique<CamService>( &argc, &argv);
    signal(SIGSEGV, crashHandler);
    signal(SIGINT, catch_signal);
    signal(SIGTERM, catch_signal); // Termination request
    signal(SIGQUIT, catch_signal); // Ctrl+\ (Quit)
    signal(SIGHUP, catch_signal);  // Terminal closed
    
    camservice->mainLoop();
}

void catch_signal(int signum)
{
    cout << "Exiting cleanly. Received signal " << strsignal(signum) << endl;
    camservice->killMainLoop();
    exit(signum);
}

void crashHandler(int sig) {
    void *array[30];
    size_t size = backtrace(array, 30);
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}