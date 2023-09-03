#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h> 

using namespace std;

void ctrlZHandler(int sig_num) {
    
    cout << "smash: got ctrl-Z\n";
    pid_t foreground_pid = (SmallShell::getInstance()).foreground_pid;
    Command* foreground_command = (SmallShell::getInstance()).foreground_command;
    
    if(foreground_pid == -1)
        return;
    (SmallShell::getInstance()).jobs->addJob(foreground_pid, foreground_command, true);
    kill(foreground_pid, SIGSTOP);
    cout << "smash: process " << foreground_pid << " was stopped" << endl;
    (SmallShell::getInstance()).foreground_pid = -1;
    
}

void ctrlCHandler(int sig_num) {
	
    cout << "smash: got ctrl-C\n";
    pid_t foreground_pid = (SmallShell::getInstance()).foreground_pid;
    if(foreground_pid == -1)
        return;
    kill(foreground_pid, SIGKILL);
    cout << "smash: process " << foreground_pid << " was killed" << endl;
    (SmallShell::getInstance()).foreground_pid = -1;
    
}



void alarmHandler(int sig_num) {
	
    int min = 0;
     
    pid_t pid_req = (SmallShell::getInstance()).min_time(&min);   
    
    while(min == 0)
    {
		
	   if(pid_req ==-1 || (SmallShell::getInstance()).findByPid(pid_req)== nullptr)	{  
            return;
       }     
		
       cout << "smash: got an alarm" << endl;
    
       (SmallShell::getInstance()).jobs->removeFinishedJobs();
       if(pid_req!=(SmallShell::getInstance()).foreground_pid && (SmallShell::getInstance()).jobs->getJobByPid(pid_req) == nullptr)
       {
		
	      (SmallShell::getInstance()).removeTimedJob(pid_req);
	      pid_req = (SmallShell::getInstance()).min_time(&min);
	      
          continue; 
	   }
		
		cout << "smash: " <<(SmallShell::getInstance()).findByPid(pid_req)->cmd_line << " timed out!" << endl;  
		
		if(pid_req == (SmallShell::getInstance()).foreground_pid) {
			(SmallShell::getInstance()).foreground_pid = -1;
			(SmallShell::getInstance()).foreground_command = nullptr;
		}
		else{
			(SmallShell::getInstance()).jobs->removeJobById(((SmallShell::getInstance()).jobs->getJobByPid(pid_req)->job_id));
		}
		
		kill(pid_req , SIGKILL);
		
	   (SmallShell::getInstance()).removeTimedJob(pid_req);
	   pid_req = (SmallShell::getInstance()).min_time(&min);
	}
	
 
     alarm(min); 
    
}


