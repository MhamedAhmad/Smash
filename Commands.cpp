#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <ctime>
#include <ctype.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void _removeSignalNumber(char* arg){
    unsigned int i;
    for(i=1; i<strlen(arg); i++)
        arg[i-1]=arg[i];
    arg[i-1]=0;
}


enum PipeRedirectionSign{overriding_redirection, appending_redirection, stdout_pipe, stderr_pipe, none};

PipeRedirectionSign _isPipeRedirectionCommand(const char* cmd_line){
	std::string str_cmd_line(cmd_line);
	if(str_cmd_line.find(">>") != string::npos)
		return appending_redirection;
	if(str_cmd_line.find(">") != string::npos)
		return overriding_redirection;
	if(str_cmd_line.find("|&") != string::npos)
		return stderr_pipe;
	if(str_cmd_line.find("|") != string::npos)
		return stdout_pipe;
	return none;
}

void _assemblePipeRedirectionCommand(const char* cmd_line, const char* first_half, const char* second_half, PipeRedirectionSign sign)
{
	std::string str_cmd_line(cmd_line);
	std::string delimiter;
	switch(sign){
		case overriding_redirection:
			delimiter = ">";
			break;
		case appending_redirection:
			delimiter = ">>";
			break;
		case stdout_pipe:
			delimiter ="|";
			break;
		case stderr_pipe:
			delimiter = "|&";
			break;
		default:
			return;
	}
	size_t position = str_cmd_line.find(delimiter);
	char* temp_string = (char*) malloc(sizeof(char) * (strlen(cmd_line)+1));
	strcpy(temp_string, cmd_line);
	temp_string[position] = 0;
	strcpy(const_cast<char*>(first_half), temp_string);
	free(temp_string);
	strncpy(const_cast<char*>(second_half), cmd_line + position + delimiter.length(), strlen(cmd_line) - position - delimiter.length() + 1);
}

/*...........................COMMAND CONS'&DES'.........................*/
Command::Command(const char* cmd_line)
{
    this->cmd_line = (char*)malloc((strlen(cmd_line)+1)* sizeof(char*));
    strcpy(const_cast<char*>(this->cmd_line), cmd_line);
	//this->cmd_line = cmd_line;
	arguments = (char**) malloc(COMMAND_MAX_ARGS * sizeof(*arguments));
	number_of_arguments = _parseCommandLine(cmd_line, arguments);
	if(strcmp(arguments[number_of_arguments-1], "&") == 0)
	{
		free(arguments[number_of_arguments-1]);
		number_of_arguments--;
	}
	else if(arguments[number_of_arguments-1][strlen(arguments[number_of_arguments-1])-1] == '&')
		arguments[number_of_arguments-1][strlen(arguments[number_of_arguments-1])-1] = '\0';
	job_id = -1;
	if(string(arguments[0]) == "timeout")
	   is_timed = true;
	else
	   is_timed = false;   
   name_with_timeout = nullptr;
   duration = -1;
}

Command::~Command()
{
	for(int i=0;i<number_of_arguments;i++)
	{
		free(arguments[i]);
	}
	free(arguments);
	free((char*)cmd_line);
	free(name_with_timeout);
}

/*...............................BUILT-IN COMMANDS......................*/
void ChpromptCommand::execute() {
    if(number_of_arguments==1 ){
        (SmallShell::getInstance()).name="smash"; //we should supprot the chprompt & case
    }
    else {
		_removeBackgroundSign(arguments[1]);
        (SmallShell::getInstance()).name=arguments[1]; 
	}

}

void ShowPidCommand::execute() {
    cout << "smash pid is " << (SmallShell::getInstance()).my_pid << endl;

}

void GetCurrDirCommand::execute() {

    char buff[MAX_COMMAND_LENGTH+1];
    if(getcwd(buff,MAX_COMMAND_LENGTH+1)!= nullptr)
           cout << buff << endl;
    else{
        perror("smash error: getcwd failed");
    }
}

void ChangeDirCommand::execute() {

    if(number_of_arguments > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
	}
    if(number_of_arguments == 2)
		{
			_removeBackgroundSign(arguments[1]);
			if(strcmp(arguments[1],"-") == 0)
			{
				if((SmallShell::getInstance()).previous_cwd== nullptr)
					cerr << "smash error: cd: OLDPWD not set" << endl;
				else
				{
					char buff[COMMAND_ARGS_MAX_LENGTH + 1];
					if (getcwd(buff, COMMAND_ARGS_MAX_LENGTH + 1) == nullptr)
						perror("smash error: getcwd failed");
					else if(chdir((SmallShell::getInstance()).previous_cwd)==-1)
							perror("smash error: chdir failed");
					else 
					{
						(SmallShell::getInstance()).previous_cwd = (char*)realloc((SmallShell::getInstance()).previous_cwd, (strlen(buff)+1)*sizeof(char));
						for (int i = 0; i <= (int)strlen(buff); i++)
							(SmallShell::getInstance()).previous_cwd[i] = buff[i];
					}
				}
			}
			else
			{
				char buff[COMMAND_ARGS_MAX_LENGTH + 1];
				if (getcwd(buff, COMMAND_ARGS_MAX_LENGTH + 1) == nullptr)
					perror("smash error: getcwd failed");
				else if(chdir(arguments[1])==-1)
						perror("smash error: chdir failed");
				else 
				{
					(SmallShell::getInstance()).previous_cwd = (char*)realloc((SmallShell::getInstance()).previous_cwd, (strlen(buff)+1)*sizeof(char));
					for (int i = 0; i <= (int)strlen(buff); i++)
						(SmallShell::getInstance()).previous_cwd[i] = buff[i];
				}
			}
		}
	else
	{
		char buff[COMMAND_ARGS_MAX_LENGTH + 1];
		if (getcwd(buff, COMMAND_ARGS_MAX_LENGTH + 1) == nullptr)
			perror("smash error: getcwd failed");
		else if(chdir("")==-1)
				perror("smash error: chdir failed");
		else 
		{
			for (int i = 0; i <= (int)strlen(buff); i++)
				(SmallShell::getInstance()).previous_cwd[i] = buff[i];
		}
	}
}

void JobsCommand::execute() {

    (SmallShell::getInstance()).jobs->removeFinishedJobs();
    (SmallShell::getInstance()).jobs->printJobsList();
}

void KillCommand::execute() {

	if(number_of_arguments != 3) {
		cerr << "smash error: kill: invalid arguments" << endl;
		return;
	}
    if(arguments[1][0]!='-') {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
	}
	
	_removeBackgroundSign(arguments[2]);
	_removeSignalNumber(arguments[1]);
	
	for(unsigned int i=0; i<strlen(arguments[1]); i++)
	{
		if(!isdigit(arguments[1][i])){
			cerr << "smash error: kill: invalid arguments" << endl;
			return;
		}
	}
	
	for(unsigned int i=0; i<strlen(arguments[2]); i++)
	{
		if(!isdigit(arguments[2][i]) && !(i == 0 && arguments[2][i] == '-')){
			cerr << "smash error: kill: invalid arguments" <<endl;
			return;
		}
	}
	if(atoi(arguments[1]) > 31){
		cerr << "smash error: kill: invalid arguments" <<endl;
		return;
	
	}
	
	(SmallShell::getInstance()).jobs->removeFinishedJobs();
	
    if((SmallShell::getInstance()).jobs->getJobById(atoi(arguments[2]))== nullptr) {
        cerr << "smash error: kill: job-id " << atoi(arguments[2]) << " does not exist" << endl;
        return;
	}
    
    pid_t pid_s = (SmallShell::getInstance()).jobs->getJobById(atoi(arguments[2]))->pid;
    
    int success = kill(pid_s,atoi(arguments[1]));
    
    if(!success)
    {
        
        cout << "signal number " << atoi(arguments[1]) << " was sent to pid " << pid_s << endl; 
    }
    else {
        perror("smash error: kill failed");
    }
    
}

void QuitCommand::execute() {
	
    if(number_of_arguments > 1)
       _removeBackgroundSign(arguments[1]);
       
    if(number_of_arguments > 1 && string(arguments[1])=="kill")
    {	   
        (SmallShell::getInstance()).jobs->removeFinishedJobs();
        int size =(SmallShell::getInstance()).jobs->jobs.size();
        cout << "smash: sending SIGKILL signal to " << size << " jobs:" << endl;
        for(auto it = (SmallShell::getInstance()).jobs->jobs.begin(); it != (SmallShell::getInstance()).jobs->jobs.end(); ++it)
        {
            cout << (*it)->pid << ": " << (*it)->cmd_line << endl;
        }

        (SmallShell::getInstance()).jobs->killAllJobs();

    }
    for(auto it = (SmallShell::getInstance()).jobs->jobs.begin(); it != (SmallShell::getInstance()).jobs->jobs.end(); ++it)
    {
		delete *it;
    }
    (SmallShell::getInstance()).jobs->jobs.clear();
    delete this;
    exit(0);
}

void ForegroundCommand::execute() {
    (SmallShell::getInstance()).jobs->removeFinishedJobs();

    if((SmallShell::getInstance()).jobs->jobs.empty() && number_of_arguments == 1)
    {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    }
    if(number_of_arguments > 2){
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    if(number_of_arguments == 2)
    {
		for(unsigned int i=0; i<strlen(arguments[1]); i++)
		{
			if(!isdigit(arguments[1][i]) && !(i == 0 && arguments[1][i] == '-')){
				cerr << "smash error: fg: invalid arguments" << endl;
				return;
			}
		}
	}

    JobEntry* requested;

    if(number_of_arguments == 1)
        requested = (SmallShell::getInstance()).jobs->getLastJob(nullptr);
    else {
		_removeBackgroundSign(arguments[1]);
        requested = (SmallShell::getInstance()).jobs->getJobById(atoi(arguments[1]));
	}

    if(requested== nullptr)
    {
        cerr << "smash error: fg: job-id " << arguments[1] << " does not exist" << endl;
        return;
    }
    
    pid_t temp_pid = requested->pid;
    char* temp_cmd_line = (char*)malloc((strlen(requested->cmd_line)+1)* sizeof(char*));
    strcpy(temp_cmd_line, requested->cmd_line);
    int temp_job_id = requested->job_id;
    cout << requested->cmd_line << " : " << requested->pid << endl;

    if(kill(temp_pid , SIGCONT)==-1)
    {
        perror("smash error: SIGCONT failed");
        return;
    }
	//cout << "before: "<<requested->job_id << " " << requested->pid;
    (SmallShell::getInstance()).jobs->removeJobById(temp_job_id);
    (SmallShell::getInstance()).foreground_command = (SmallShell::getInstance()).CreateCommand(const_cast<const char*>(temp_cmd_line));
    if((SmallShell::getInstance()).findByPid(temp_pid)!=nullptr)
    {
	   (SmallShell::getInstance()).foreground_command->name_with_timeout=(char*)malloc(sizeof(char*)*MAX_COMMAND_LENGTH+1);
	   strcpy((SmallShell::getInstance()).foreground_command->name_with_timeout,temp_cmd_line);	
	}
    
    free(temp_cmd_line);
    (SmallShell::getInstance()).foreground_command->job_id = temp_job_id;
    (SmallShell::getInstance()).foreground_pid=temp_pid;
    //cout << "after: "<<requested->job_id << " " << requested->pid;
    waitpid(temp_pid , nullptr , WUNTRACED);
    (SmallShell::getInstance()).foreground_pid=-1;
    delete((SmallShell::getInstance()).foreground_command);
    (SmallShell::getInstance()).foreground_command = nullptr;
}

void BackgroundCommand::execute() {

    if (number_of_arguments > 2)
    {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    if(number_of_arguments == 2)
    {
		for(unsigned int i=0; i<strlen(arguments[1]); i++)
		{
			if(!isdigit(arguments[1][i]) && !(i == 0 && arguments[1][i] == '-')){
				cerr << "smash error: bg: invalid arguments" << endl;
				return;
			}
		}
	}
    (SmallShell::getInstance()).jobs->removeFinishedJobs();
    JobEntry* requested ;
    if(number_of_arguments==1)
    {
        requested =(SmallShell::getInstance()).jobs->getLastStoppedJob(nullptr);
        if(requested== nullptr)
        {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }

    else{
		_removeBackgroundSign(arguments[1]);
		
        requested = (SmallShell::getInstance()).jobs->getJobById(atoi(arguments[1]));

        if (requested== nullptr)
        {
            cerr << "smash error: bg: job-id " << arguments[1] << " does not exist" << endl;
            return;
        }

        if(!requested->isStopped)
        {
            cerr << "smash error: bg: job-id " << requested->job_id << " is already running in the background" << endl;
            return;
        }
    }

    cout << requested->cmd_line << " : " << requested->pid << endl;

    if(kill(requested->pid , SIGCONT)==-1)
    {
        perror("smash error: SIGCONT failed");
        return;
    }

    requested->isStopped = false;
}

/*................................EXTERNAL COMMAND......................*/
void ExternalCommand::execute()
{
	if(_isBackgroundComamnd(cmd_line))
	{
		pid_t p = fork();
		if(p == 0)
		{
			setpgrp();
			char* temp_line = const_cast<char*> (cmd_line);
			_removeBackgroundSign(temp_line);
			char* cmd[4];
			std::string str1 = "bash";
			cmd[0] = const_cast<char*> (str1.c_str());
			std::string str2 = "-c";
			cmd[1] = const_cast<char*> (str2.c_str());
			cmd[2] = const_cast<char*> (temp_line);
			cmd[3] = nullptr;
			execv("/bin/bash", cmd);
		}
		else
		{
			(SmallShell::getInstance()).jobs->removeFinishedJobs();
			(SmallShell::getInstance()).jobs->addJob(p, this);
			if(is_timed)
			{
				
			   (SmallShell::getInstance()).addTimedJob(p , name_with_timeout , duration);	
			}
		}
	}
	else
	{
		pid_t p = fork();
		if(p == 0)
		{
			setpgrp();
			char* cmd[4];
			std::string str1 = "bash";
			cmd[0] = const_cast<char*> (str1.c_str());
			std::string str2 = "-c";
			cmd[1] = const_cast<char*> (str2.c_str());
			cmd[2] = const_cast<char*> (cmd_line);
			cmd[3] = nullptr;
			execv("/bin/bash", cmd);
		}
		else
		{
			if(is_timed)
			{
			   (SmallShell::getInstance()).addTimedJob(p , name_with_timeout , duration);	
			}
			
			(SmallShell::getInstance()).foreground_pid = p;
			(SmallShell::getInstance()).foreground_command = this;
		    waitpid(p, nullptr, WUNTRACED);
			//(SmallShell::getInstance()).removeTimedJob(p);
			(SmallShell::getInstance()).foreground_pid = -1;
			(SmallShell::getInstance()).foreground_command = nullptr;
		}
	}
}

/*...............................SPECIAL COMMANDS.......................*/
void RedirectionCommand::execute()
{
	PipeRedirectionSign sign = _isPipeRedirectionCommand(cmd_line);
	const char* first_half = (char*) malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	const char* second_half = (char*) malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	_assemblePipeRedirectionCommand(cmd_line, first_half, second_half, sign);
	if(_trim(string(second_half)) == "")
	{
		cerr <<"smash error: syntax error near unexpected token `newline'"<<endl;
		free((char*)first_half);
		free((char*)second_half);
		return;
	}
	pid_t p = fork();
	if(p == 0)
	{
		setpgrp();
		close(1);
		if(sign == overriding_redirection){
			if(open(_trim(string(second_half)).c_str(), O_CREAT|O_RDWR|O_TRUNC, 0655) == -1){
				perror("smash error: open failed");
				exit(0);
			}
		}
		else
			if(open(_trim(string(second_half)).c_str(), O_CREAT|O_RDWR|O_APPEND, 0655) == -1){
				perror("smash error: open failed");
				exit(0);
			}
		if(_trim(string(first_half)) != "")
			(SmallShell::getInstance()).executeCommand(first_half);
		free((char*)first_half);
		free((char*)second_half);
		exit(0);
	}
	else
	{
		waitpid(p, nullptr, WUNTRACED);
		free((char*)first_half);
		free((char*)second_half);
	}
}

void PipeCommand::execute()
{
	PipeRedirectionSign sign = _isPipeRedirectionCommand(cmd_line);
	const char* first_half = (char*) malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	const char* second_half = (char*) malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	_assemblePipeRedirectionCommand(cmd_line, first_half, second_half, sign);
	if(_trim(string(first_half)) == "")
	{
		cerr <<"smash error: syntax error near unexpected token `|'"<<endl;
		free((char*)first_half);
		free((char*)second_half);
		return;
	}
	if(_trim(string(second_half)) == "")
	{
		cerr <<"smash error: syntax error near unexpected token `newline'"<<endl;
		free((char*)first_half);
		free((char*)second_half);
		return;
	}
	int fd[2];
	pipe(fd);
	pid_t p1 = fork();
	if(p1 == 0)
	{
		setpgrp();
		dup2(fd[1],sign - 1);
		close(fd[0]);
		close(fd[1]);
		(SmallShell::getInstance()).executeCommand(first_half);
		free((char*)first_half);
		free((char*)second_half);
		exit(0);
	}
	waitpid(p1, nullptr, WUNTRACED);
	pid_t p2 = fork();
	if(p2 == 0)
	{
		setpgrp();
		dup2(fd[0],0);
		close(fd[0]);
		close(fd[1]);
		(SmallShell::getInstance()).executeCommand(second_half);
		free((char*)first_half);
		free((char*)second_half);
		exit(0);
	}
	else
	{
		close(fd[0]);
		close(fd[1]);
		waitpid(p2, nullptr, WUNTRACED);
		free((char*)first_half);
		free((char*)second_half);
	}
}

void TailCommand::execute(){
	
	if(number_of_arguments != 2 && number_of_arguments != 3)
	{
		cerr << "smash error: tail: invalid arguments" << endl;
		return;
	}
	unsigned int lines_to_read = 0;
	int fd ;
	
	if(number_of_arguments == 2 )
	{
		lines_to_read = 10;
		fd = open(arguments[1] , O_RDONLY);	
		if(fd == -1)
		{
			perror("smash error: open failed");
			return;
		}
	}
	else 
	{	
		if(arguments[1][0] != '-')
		{
			cerr << "smash error: tail: invalid arguments" << endl;
			return;
		}
		_removeSignalNumber(arguments[1]);
		for(unsigned int i=0; i<strlen(arguments[1]); i++)
		{
			if(!isdigit(arguments[1][i]))
			{
				cerr << "smash error: tail: invalid arguments" << endl;
				return;
			}
		}
		lines_to_read = atoi(arguments[1]);
		fd = open(arguments[2] , O_RDONLY);	
		if(fd == -1)
		{
			perror("smash error: open failed");
			return;
		}	
	}
	
	size_t all_bytes = 0;
	unsigned int all_lines = 0;
	unsigned int count_lines = 0;
	size_t count_bytes = 0;
	char c;
	char prev_c;
	
	lseek(fd , 0 , SEEK_SET);
	
	if(read(fd, &c, 1) == -1)
	{
	   perror("smash error: read failed");
	   if(close(fd)==-1)
	       perror("smash error: close failed");
	   return;
	}
	
	lseek(fd , 0 , SEEK_SET);
	
	while(read(fd, &c, 1) == 1)
	{
		prev_c = c;
		all_bytes++;
		if(c == '\n')
		   all_lines++;  
	}

	if(prev_c != '\n')
		   all_lines++; 
	
	size_t bytes_to_read = all_bytes;
	lseek(fd , 0 , SEEK_SET);
	
	if(all_lines > lines_to_read)
	{
	    while(read(fd, &c, 1) == 1)
	    {		
		   count_bytes++;
		   if(c == '\n') 
		   {
		       if(++count_lines == all_lines - lines_to_read )
		          break;
	       }
	    }
	   bytes_to_read  = all_bytes - count_bytes ;  
    }
    
    lseek(fd , count_bytes , SEEK_SET);    
    char* buffer = (char*)malloc(bytes_to_read * sizeof(char));
    
    unsigned int req = bytes_to_read;
    int res =0;
    
    while(req>0)
    {
		res = read(fd , buffer + (bytes_to_read - req) , req) ;
		
        if(res == -1 )
         {
   	     	perror("smash error: read failed");
	        if(close(fd)==-1)
	            perror("smash error: close failed");
   	        return;	
   	     }
   	     req -= res; 	
    }
    req = bytes_to_read;
    res=0;
    
    while(req > 0){
	    res = write( 1 , buffer + (bytes_to_read - req) , req);	
        if(res == -1)
        {
			perror("smash error: write failed");   		
	        if(close(fd)==-1)
	            perror("smash error: close failed");			
			return;
		}
		req -= res;	
	}
		
	if(close(fd)==-1)
	    perror("smash error: close failed");
	    	
    
}
	
void TouchCommand::execute(){
	if(number_of_arguments != 3)
	{
		cerr << "smash error: touch: invalid arguments" << endl;
		return;
	}
	
	string temp_string;
	vector<string> time;
	std::istringstream iss(string(arguments[2]).c_str());
	while(std::getline(iss,temp_string, ':'))
		time.push_back(temp_string);
	struct tm date;
	date.tm_sec = atoi(time[0].c_str());
	date.tm_min = atoi(time[1].c_str());
	date.tm_hour = atoi(time[2].c_str());
	date.tm_mday = atoi(time[3].c_str());
	date.tm_mon = atoi(time[4].c_str()) - 1;
	date.tm_year = atoi(time[5].c_str()) - 1900;
	
	time_t ac_mod_time = mktime(&date);
	if(ac_mod_time == -1){
		perror("smash error: mktime failed");
		return;
	}
	
	struct utimbuf timebuf;
	timebuf.actime = ac_mod_time;
	timebuf.modtime = ac_mod_time;
	
	if(utime(arguments[1] , &timebuf) != 0 )
	   perror("smash error: utime failed");
	   	
}	
	
void TimeoutCommand::execute(){
	if(number_of_arguments <= 2){
		cerr << "smash error: timeout: invalid arguments" << endl;
		return;
	}
	for(unsigned int i=0; i<strlen(arguments[1]); i++)
	{
		if(!isdigit(arguments[1][i])){
			cerr << "smash error: timeout: invalid arguments" << endl;
			return;
		}
	}
    const char* cmd_to_time = (char*)malloc(sizeof(char)*MAX_COMMAND_LENGTH);
    string temp_num(arguments[1]);
    string temp_line(cmd_line);
    size_t pos = temp_line.find(temp_num);
    strncpy(const_cast<char*>(cmd_to_time) , cmd_line + pos + strlen(arguments[1]) , strlen(cmd_line) - strlen(arguments[1]) - pos);
    Command* cmd = (SmallShell::getInstance()).CreateCommand(cmd_to_time);
    cmd->is_timed = true;
    cmd->name_with_timeout = (char*)malloc(sizeof(char*)*(MAX_COMMAND_LENGTH+1));
    strcpy(cmd->name_with_timeout , cmd_line);
    cmd->duration = atoi(arguments[1]);
    cmd->execute();
    delete  cmd;
    free((char*)cmd_to_time);
	
}

/*...............................JOBS FUNCTIONS.........................*/
JobEntry::JobEntry(pid_t pid, int job_id, const char* cmd_line, bool isStopped)
{
	this->pid = pid;
	this->job_id = job_id;
	this->isStopped = isStopped;
	this->cmd_line = (char*)malloc((strlen(cmd_line)+1)* sizeof(char*));
    strcpy(const_cast<char*>(this->cmd_line), cmd_line);
	time = std::time(nullptr);
}

JobEntry::~JobEntry()
{
	free((char*)cmd_line);
}

bool JobEntry::operator==(const JobEntry& job) const
{
	return(job_id == job.job_id);
}

bool JobEntry::operator!=(const JobEntry& job) const
{
	return(job_id != job.job_id);
}

bool jobCompare(JobEntry* job1, JobEntry* job2)//Function Added
{
    return (job1->job_id < job2->job_id);
}

void JobsList::addJob(pid_t pid, Command* cmd, bool isStopped)//Some changes in function
{
    if(cmd == nullptr)
        return;
    if(cmd->job_id == -1)
    {
        if(jobs.size() == 0)
            cmd->job_id = 1;
        else
            cmd->job_id = (*jobs.back()).job_id + 1;
    }
    if(cmd->is_timed)
       jobs.push_back(new JobEntry(pid, cmd->job_id, cmd->name_with_timeout, isStopped));
    else
       jobs.push_back(new JobEntry(pid, cmd->job_id, cmd->cmd_line, isStopped));    
    std::sort(jobs.begin(), jobs.end(), jobCompare);
}

void JobsList::printJobsList()
{
	for(auto it = jobs.begin(); it != jobs.end(); ++it)
	{
		std::cout << "[" << (*it)->job_id << "] " << (*it)->cmd_line << " : " << (*it)->pid << " " << difftime(std::time(nullptr), (*it)->time) << " secs";
		if((*it)->isStopped)
			std::cout << " (stopped)" << std::endl;
		else
			std::cout << std::endl;
	}
}

void JobsList::killAllJobs()
{
	for (auto it = jobs.begin(); it != jobs.end(); ++it)
	{
		kill((*it)->pid, SIGKILL);
	}
	jobs.clear();
}

void JobsList::removeFinishedJobs()
{
	for (auto it = jobs.begin(); it != jobs.end();)
	{
		if(waitpid((*it)->pid, nullptr, WNOHANG) == (*it)->pid)
		{
			delete *it;
			jobs.erase(it);
		}
		else
			++it;
	}
}

JobEntry* JobsList::getJobById(int jobId)
{
	for (auto it = jobs.begin(); it != jobs.end();++it)
	{
		if((*it)->job_id == jobId)
		{
			return (*it);
		}
	}
	return nullptr;
}

JobEntry* JobsList::getJobByPid(int Pid)
{
	for (auto it = jobs.begin(); it != jobs.end();++it)
	{
		if((*it)->pid == Pid)
		{
			return (*it);
		}
	}
	return nullptr;
}

void JobsList::removeJobById(int jobId)
{
	for (auto it = jobs.begin(); it != jobs.end();++it)
	{
		if((*it)->job_id == jobId)
		{
			delete *it;
			jobs.erase(it);
			break;
		}
	}
}

JobEntry* JobsList::getLastJob(int* lastJobId)
{
	if(jobs.size() == 0)
		return nullptr;
	if(lastJobId != nullptr)
		*lastJobId = (*jobs.back()).job_id;
	return (jobs.back());
}

JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
	for (auto it = jobs.end(); it != jobs.begin();)
	{
		--it;
		if((*it)->isStopped)
		{
			if(jobId != nullptr)
				*jobId = (*it)->job_id;
			return (*it);
		}
	}
	return nullptr;
}


/*..............................SMASH FUNCTIONS.....................*/
SmallShell::SmallShell(): my_pid(getpid()), jobs(new JobsList()), foreground_pid(-1), name("smash"), previous_cwd(nullptr), foreground_command(nullptr){}

SmallShell::~SmallShell() {
	delete(jobs);
	//delete(timed_jobs);
	if(previous_cwd != nullptr)
		free(previous_cwd);
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if(firstWord.compare("timeout") == 0)
     return new TimeoutCommand(cmd_line);

  if(_isPipeRedirectionCommand(cmd_line) == overriding_redirection || _isPipeRedirectionCommand(cmd_line) == appending_redirection) {
	return new RedirectionCommand(cmd_line);
  }
  else if(_isPipeRedirectionCommand(cmd_line) != none){
	return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("tail") == 0 || firstWord.compare("tail&") == 0 ) {
    return new TailCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0 ) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("touch") == 0 || firstWord.compare("touch&") == 0 ) {
    return new TouchCommand(cmd_line);
  }
  else if (firstWord.compare("showpid" ) == 0 || firstWord.compare("showpid&") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("chprompt")==0 || firstWord.compare("chprompt&") == 0) {
      return new ChpromptCommand(cmd_line);
  }
  else if(firstWord.compare("cd")==0 || firstWord.compare("cd&") == 0) {
      return new ChangeDirCommand(cmd_line);
  }
  else if(firstWord.compare("jobs")==0 || firstWord.compare("jobs&") == 0) {
      return new JobsCommand(cmd_line);
  }
  else if(firstWord.compare("kill")==0 || firstWord.compare("kill&") == 0){
      return new KillCommand(cmd_line);
  }
  else if(firstWord.compare("fg")==0 || firstWord.compare("fg&") == 0){
      return new ForegroundCommand(cmd_line);
  }
  else if(firstWord.compare("bg")==0 || firstWord.compare("bg&") == 0){
      return new BackgroundCommand(cmd_line);
  }
  else if(firstWord.compare("quit")==0 || firstWord.compare("quit&") == 0){
      return new QuitCommand(cmd_line);
  }

  else {
    return new ExternalCommand(cmd_line);
  }

}

void SmallShell::executeCommand(const char *cmd_line) {

		Command* cmd = CreateCommand(cmd_line);
		cmd->execute();
		delete cmd;
}

pid_t SmallShell::min_time(int* min){ 
    		
	pid_t pid_s =-1;
	if(timed_jobs.size()==0){
	   return pid_s;
	   }
	*min = (*timed_jobs.begin())->duration - difftime(std::time(nullptr), (*timed_jobs.begin())->timestamp); 
	for(auto it = timed_jobs.begin(); it != timed_jobs.end();++it){
		if(((*it)->duration - difftime(std::time(nullptr),(*it)->timestamp)) <= *min ){
		    *min = (*it)->duration - difftime(std::time(nullptr),(*it)->timestamp);
		    pid_s = (*it)->pid; 
		}	
	}
	return pid_s;
	
}

void SmallShell::removeTimedJob(pid_t pid){
	
    if(timed_jobs.size()==0)	
       return;
    for(auto it = timed_jobs.begin(); it != timed_jobs.end();++it)
	{
		if((*it)->pid == pid)
		{
			delete *it;
			timed_jobs.erase(it);
			break;
		}
	}
}

void SmallShell::addTimedJob(pid_t pid , char* cmd_line , int duration){
	
	 if(timed_jobs.size() == 0){
	    alarm(duration);
	 }
	 else{
	    int min ;
	    min_time(&min);
	    if(min > duration)
	       alarm(duration);	 
	}  
	timed_job* to_add =new timed_job(pid , cmd_line , duration);  
    timed_jobs.push_back(to_add);	
   
    
} 	

timed_job* SmallShell::findByPid(pid_t zft)
{
	    
	for(auto it = timed_jobs.begin(); it != timed_jobs.end();++it)
	{
		if((*it)->pid == zft)
		{
			return (*it);
		}
	}
	
	return nullptr;
}
