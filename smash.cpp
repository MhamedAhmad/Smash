#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    
    struct sigaction time_out;
    sigemptyset(&time_out.sa_mask);
    time_out.sa_flags = SA_RESTART;
    time_out.sa_handler = alarmHandler;
    if(sigaction(SIGALRM , &time_out , nullptr) == -1 )
    {
	    perror("smash error: failed to set timeOut handler");	
	}
    

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();

    while(true) {
        std::cout << smash.name << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if(cmd_line.empty())
            continue;
        smash.executeCommand(cmd_line.c_str());
       
    }
    return 0;
}

