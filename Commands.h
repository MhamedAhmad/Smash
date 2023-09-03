#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <ctime>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_JOBS (100)
#define MAX_COMMAND_LENGTH (80)


class Command {
public:
    char** arguments;
    int number_of_arguments;
    const char* cmd_line;
    int job_id;
    bool is_timed;
    char* name_with_timeout;
    int duration;
public:
    explicit Command(const char* cmd_line);
    Command(const Command& copy)=delete;
    virtual ~Command();
    virtual void execute() = 0;
};


class JobEntry {
public:
    pid_t pid;
    int job_id;
    bool isStopped;
    const char* cmd_line;
    time_t time;
    JobEntry(pid_t pid, int job_id, const char* cmd_line, bool isStopped = false);
    ~JobEntry();
    bool operator==(const JobEntry& job) const;
    bool operator!=(const JobEntry& job) const;
};

struct timed_job{
public:
    pid_t pid;
    char* cmd_line;
    int duration;
    time_t timestamp;
    
    timed_job(pid_t pid , char* cmd_line , int duration ) {
		
		 this->pid = pid; 
		 this->cmd_line = (char*)malloc(sizeof(char*)*(MAX_COMMAND_LENGTH+1));
		 strcpy(this->cmd_line , cmd_line);
		 this->duration = duration;
	     timestamp = std::time(nullptr);	
	}  
	
	~timed_job(){
	 free(cmd_line);	
	}	
};


class JobsList {
public:
    std::vector<JobEntry*> jobs;
    JobsList() = default;
    ~JobsList() = default;
    void addJob(pid_t pid,Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry *getJobById(int jobId);
    JobEntry* getJobByPid(int pid);
    void removeJobById(int jobId);
    JobEntry *getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
};

class SmallShell {
private:
    SmallShell();
public:
    pid_t my_pid;
    static SmallShell& instance;
    JobsList *jobs;
    std::vector<timed_job*> timed_jobs; 
    int foreground_pid;
    std::string name;
    char* previous_cwd;
    Command* foreground_command;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    pid_t min_time(int* min);
    void removeTimedJob(pid_t pid);
    void addTimedJob(pid_t pid , char* cmd_line , int duration);
    timed_job* findByPid(pid_t zft);

};

/*...........................COMMAND KINDS.........................*/
class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char* cmd_line) : Command(cmd_line){}
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    explicit ExternalCommand(const char* cmd_line): Command(cmd_line) {}
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
public:
    explicit PipeCommand(const char* cmd_line) : Command(cmd_line) {}
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}
    virtual ~RedirectionCommand() {}
    void execute() override;
};



/*.................................BUILT_IN COMMANDS.......................*/
class ChpromptCommand : public BuiltInCommand{
public:
    explicit ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~ChpromptCommand(){}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    explicit ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
public:
    explicit QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
public:
    explicit JobsCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    explicit KillCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    explicit ForegroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
public:
    explicit BackgroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
    virtual ~BackgroundCommand() {}
    void execute() override;
};


/*.................................SPECIAL COMMANDS......................*/
class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~TailCommand() {}
    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~TouchCommand() {}
    void execute() override;
};


class TimeoutCommand : public Command {
public:	
	TimeoutCommand(const char* cmd_line) : Command(cmd_line) {}
    virtual ~TimeoutCommand() {}
    void execute() override;
	
};



#endif //SMASH_COMMAND_H_
