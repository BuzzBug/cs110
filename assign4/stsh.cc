/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */
#include "stsh-parse-utils.h"
#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>  // for fork
#include <signal.h>  // for kill
#include <sys/wait.h>
#include "assert.h"
#include <string>
using namespace std;

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it

static void CheckJobNo(size_t jobno, bool fg)
{
  if(!joblist.containsJob(jobno))
  {
    string title;
    if(fg)
    {
      title = "fg ";
    }
    else
    {
      title = "bg ";
    }
    string second(to_string(jobno));
    string endmesg(": No such job.");
    string errormessage = title + second + endmesg;
    throw STSHException(errormessage);
  }  
}

static void FgBuiltin(struct command com)
{
  string usage("Usage: fg <jobid>.");
  size_t jobno = parseNumber(com.tokens[0], usage);
  CheckJobNo(jobno, true);  
  if(!joblist.containsJob(jobno)) throw STSHException("invalid jobno");
  STSHJob& job = joblist.getJob(jobno);
  pid_t gpid = job.getGroupID();
  kill(-gpid, SIGCONT);
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);  
  */
  job.setState(kForeground);
  STSHJobState state_const = kForeground;
  assert(job.getState() == state_const);
  //sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void BgBuiltin(struct command com)
{
  string usage("Usage: bg <jobid>.");
  size_t jobno = parseNumber(com.tokens[0], usage);
  CheckJobNo(jobno, false);
  if(!joblist.containsJob(jobno)) throw STSHException("invalid jobno");
  STSHJob& job = joblist.getJob(jobno);
  pid_t gpid = job.getGroupID();
  kill(-gpid, SIGCONT);
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  */
  job.setState(kBackground);
  STSHJobState state_const = kBackground;
  assert(job.getState() == state_const);
  //sigprocmask(SIG_UNBLOCK, &additions, &existingmask);
}

static void SlayBuiltin(struct command com)
{
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  */
  size_t size;
  for (size = 0; com.tokens[size] != NULL; size++);
  if (size == 1)
  {
    string usage("Invalid input to slay.");
    size_t pid = parseNumber(com.tokens[0], usage);
    STSHJob& job = joblist.getJobWithProcess(pid);
    if(job.containsProcess(pid))
    {
      kill(pid, SIGKILL);
    }
    else
    {
      throw STSHException("Job does not contain pid.");
    } 
  }
  if (size == 2)
  {
    string usage("Invalid input to slay.");
    size_t jobno = parseNumber(com.tokens[0], usage);
    if(!joblist.containsJob(jobno)) throw STSHException("Invalid job number.");
    STSHJob& job = joblist.getJob(jobno);
    vector<STSHProcess>& processvec = job.getProcesses();
    size_t index = parseNumber(com.tokens[1], usage);
    pid_t pid = processvec[index].getID();
    kill(pid, SIGKILL);
  }
  //sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void HaltBuiltin(struct command com)
{
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  */
  string usage("Invalid input to halt.");
  size_t size;
  for (size = 0; com.tokens[size] != NULL; size++);
  if (size == 1)
  {
    size_t pid = parseNumber(com.tokens[0], usage);
    STSHJob& job = joblist.getJobWithProcess(pid);
    if(job.containsProcess(pid))
    {
      STSHProcess& process = job.getProcess(pid);
      if(process.getState() == kStopped) return;
      kill(pid, SIGSTOP);
    }
    else
    {
      throw STSHException("Job does not contain pid.");
    }
  }
  if (size == 2)
  {
    size_t jobno = parseNumber(com.tokens[0], usage);
    if(!joblist.containsJob(jobno)) throw STSHException("Invalid job number.");
    STSHJob& job = joblist.getJob(jobno);
    vector<STSHProcess>& processvec = job.getProcesses();
    size_t index = parseNumber(com.tokens[1], usage);
    if (processvec[index].getState() == kStopped) return;
    pid_t pid = processvec[index].getID();
    kill(pid, SIGSTOP);  
  }
  //sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void ContBuiltin(struct command com)
{
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  */
  string usage("Invalid input to cont.");
  size_t size;
  for (size = 0; com.tokens[size] != NULL; size++);
  if (size == 1)
  {
    size_t pid = parseNumber(com.tokens[0], usage);
    STSHJob& job = joblist.getJobWithProcess(pid);
    if(job.containsProcess(pid))
    {
      STSHProcess& process = job.getProcess(pid);
      if (process.getState() == kRunning) return;
      kill(pid, SIGCONT);
    }
    else
    {
      throw STSHException("Job does not contain pid.");
    }
  }
  if (size == 2)
  {
    size_t jobno = parseNumber(com.tokens[0], usage);
    if(!joblist.containsJob(jobno)) throw STSHException("Invalid job number.");
    STSHJob& job = joblist.getJob(jobno);
    vector<STSHProcess>& processvec = job.getProcesses();
    size_t index = parseNumber(com.tokens[1], usage);
    if (processvec[index].getState() == kRunning) return;
    pid_t pid = processvec[index].getID();
    kill(pid, SIGCONT);
  }
  //sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void JobBuiltin(void)
{
  /*
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  */
  cout << joblist; 
  //sigprocmask(SIG_UNBLOCK, &additions, NULL);
}
/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins)/sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline& pipeline) {
  const string& command = pipeline.commands[0].command;
  auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
  if (iter == kSupportedBuiltins + kNumSupportedBuiltins) return false;
  size_t index = iter - kSupportedBuiltins;

  switch (index) {
  case 0:
  case 1: exit(0);
  case 2: FgBuiltin(pipeline.commands[0]); break;
  case 3: BgBuiltin(pipeline.commands[0]); break;
  case 4: SlayBuiltin(pipeline.commands[0]); break;
  case 5: HaltBuiltin(pipeline.commands[0]); break;
  case 6: ContBuiltin(pipeline.commands[0]); break;
  case 7: JobBuiltin(); break;
  default: throw STSHException("Internal Error: Builtin command not supported."); // or not implemented yet
  }
  
  return true;
}

static void SigIntHandler(int sig)
{
  if(joblist.hasForegroundJob())
  {
    STSHJob& fgjob = joblist.getForegroundJob();
    kill(-fgjob.getGroupID(), SIGINT);
  }
}

static void SigTstpHandler(int sig)
{
  if(joblist.hasForegroundJob())
  {
    STSHJob& fgjob = joblist.getForegroundJob();
    kill(-fgjob.getGroupID(), SIGTSTP);
  }
}

static void SigChildHandler(int sig)
{
  pid_t pid;
  while(true)
  {
    int status;
    pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (pid <= 0) break;
    if (WIFEXITED(status) | WIFSIGNALED(status))
    {
      STSHJob& job = joblist.getJobWithProcess(pid);
      assert(job.containsProcess(pid));
      STSHProcess& process = job.getProcess(pid);
      process.setState(kTerminated);
      joblist.synchronize(job);
    }
    if (WIFSTOPPED(status))
    {
      STSHJob& job = joblist.getJobWithProcess(pid);
      assert(job.containsProcess(pid));
      STSHProcess& process = job.getProcess(pid);
      process.setState(kStopped);
      joblist.synchronize(job);
    }
    if (WIFCONTINUED(status))
    {
      STSHJob& job = joblist.getJobWithProcess(pid);
      assert(job.containsProcess(pid));
      STSHProcess& process = job.getProcess(pid);
      process.setState(kRunning);
      joblist.synchronize(job);
    } 
  }
}
/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD, 
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and 
 * ignores two others.
 */
static void installSignalHandlers() {
  installSignalHandler(SIGQUIT, [](int sig) { exit(0); });
  installSignalHandler(SIGTTIN, SIG_IGN);
  installSignalHandler(SIGTTOU, SIG_IGN);
  installSignalHandler(SIGCHLD, SigChildHandler);
  installSignalHandler(SIGINT, SigIntHandler);
  installSignalHandler(SIGTSTP, SigTstpHandler);
}

static size_t getNumTokens(const pipeline& p, size_t index)
{
  size_t count = 0;
  while(p.commands[index].tokens[count] != NULL && count < kMaxArguments)
  {
    count++;
  }
  return count;
}

static int handleInput(const pipeline& p)
{
  int inputfd = -1;
  if(!p.input.empty())
  {
    inputfd = open(p.input.c_str(), O_RDONLY);
    dup2(inputfd, STDIN_FILENO);
    close(inputfd);
  }
  return inputfd;
}

static int handleOutput(const pipeline& p)
{
  int outputfd = -1;
  if(!p.output.empty())
  {
    outputfd = open(p.output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(outputfd, STDOUT_FILENO);
    close(outputfd);
  }
  return outputfd;
}

static void printBackgroundInfo(STSHJob& job)
{
  vector<STSHProcess>& processes = job.getProcesses();
  int jobno = job.getNum();
  cout << "[" << jobno << "] ";
  for (const STSHProcess& process: processes)
  {
    cout << process.getID() << " ";
  }
  cout << endl;
}

static void tcsetWrapper(const pipeline& p)
{
  if (!p.background)
  {
    bool transferred = tcsetpgrp(STDIN_FILENO, getpgid(0)) == 0;
    if (!transferred && errno != ENOTTY) throw STSHException("Unexpected trouble calling tcsetpgrp");
  }
}
static void createJobSingle(const pipeline& p)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    setpgid(getpid(), getpid());
    size_t index = 0;
    size_t count = getNumTokens(p, index);
    size_t argc = count + 1; //+1 for NULL terminator
    char *argv[argc];
    argv[0] = (char *)p.commands[index].command;
    memcpy(argv + 1, p.commands[index].tokens, sizeof(char *) * argc); //also pasting in NULL terminator    
    tcsetWrapper(p);
    handleInput(p);
    handleOutput(p);
    execvp(argv[0], argv);
    string errormessage = string(argv[0]);
    errormessage += ": Command not found.";
    throw STSHException(errormessage);
  }
  setpgid(pid, pid);
  STSHJobState state = (p.background ? kBackground : kForeground);
  STSHJob& job = joblist.addJob(state);
  STSHProcess process(pid, p.commands[0]);
  job.addProcess(process);
  if (state == kBackground) printBackgroundInfo(job);
}

static void createJobMultiple(const pipeline& p)
{
  int fds[2];
  pipe(fds);
  pid_t pid1 = fork();
  if (pid1 == 0)
  {
    setpgid(getpid(), getpid());
    close(fds[0]);
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);
    size_t index = 0;
    size_t count = getNumTokens(p, index);
    size_t argc = count + 1;
    char *argv[argc];
    argv[0] = (char *)p.commands[index].command;
    memcpy(argv + 1, p.commands[index].tokens, sizeof(char *) * argc);
    tcsetWrapper(p);
    handleInput(p);
    execvp(argv[0], argv);
    string errormessage = string(argv[0]);
    errormessage += ": not found";
    throw STSHException(errormessage);
  } 
  setpgid(pid1, pid1);
  close(fds[1]);
  STSHJobState state = (p.background ? kBackground : kForeground);
  STSHJob& job = joblist.addJob(state);
  STSHProcess process(pid1, p.commands[0]);
  job.addProcess(process);
  int lastfd = fds[0];
  for (size_t i = 1; i < p.commands.size() - 1; i++)
  {
    int fdsB[2];
    pipe(fdsB);
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
      setpgid(getpid(), pid1);
      dup2(lastfd, STDIN_FILENO);
      close(lastfd);
      close(fdsB[0]);
      dup2(fdsB[1], STDOUT_FILENO);
      close(fdsB[1]);
      size_t count = getNumTokens(p, i);
      size_t argc = count + 1;
      char *argv[argc];
      argv[0] = (char *)p.commands[i].command;
      memcpy(argv + 1, p.commands[i].tokens, sizeof(char *) * argc);
      tcsetWrapper(p);
      execvp(argv[0], argv);
      throw STSHException("Execvp failed\n");
    }
    setpgid(pid2, pid1);
    STSHProcess process2(pid2, p.commands[i]);
    job.addProcess(process2);
    close(lastfd);
    close(fdsB[1]);
    lastfd = fdsB[0];
  } 
  size_t last = p.commands.size() - 1;
  pid_t pidlast = fork();
  if (pidlast == 0)
  {
    dup2(lastfd, STDIN_FILENO);
    close(lastfd);
    size_t count = getNumTokens(p, last);
    size_t argc = count + 1;
    char *argv[argc];
    argv[0] = (char *)p.commands[last].command;
    memcpy(argv + 1, p.commands[last].tokens, sizeof(char *) * argc);
    setpgid(getpid(), pid1);
    handleOutput(p);
    execvp(argv[0], argv);
    throw STSHException("execvp failed.\n");
  }
  setpgid(pidlast, pid1);
  STSHProcess processlast(pidlast, p.commands[last]);
  job.addProcess(processlast);
  close(lastfd);
  //close(fds[1]);
  if(p.background) printBackgroundInfo(job);
}
/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline& p) {
  ///* STSHJob& job = */ joblist.addJob(kForeground);
  if(p.commands.size() == 1) createJobSingle(p);
  else createJobMultiple(p);
}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).  
 */
int main(int argc, char *argv[]) {
  pid_t stshpid = getpid();
  installSignalHandlers();
  rlinit(argc, argv);
  while (true) {
    string line;
    if (!readline(line)) break;
    if (line.empty()) continue;
    sigset_t additions, existingmask;
    sigemptyset(&existingmask);
    sigemptyset(&additions);
    sigaddset(&additions, SIGCHLD);
    try {
      pipeline p(line);
      //bool builtin = handleBuiltin(p);
      //sigset_t additions, existingmask;
      //sigemptyset(&additions);
      //sigaddset(&additions, SIGCHLD);
      sigprocmask(SIG_BLOCK, &additions, &existingmask);
      bool builtin = handleBuiltin(p);
      if (!builtin) createJob(p);
      while(joblist.hasForegroundJob())
      {
        sigsuspend(&existingmask);
      }
      //when the parents see that the fg job is closed, they should call
      //tcsetpgrp() again on their own pid to reclaim terminal control.  
      bool transferred = tcsetpgrp(STDIN_FILENO, getpgid(0)) == 0;
      if (!transferred && errno != ENOTTY) throw STSHException("Unexpected trouble calling tcsetpgrp");
      sigprocmask(SIG_UNBLOCK, &additions, NULL);
    } catch (const STSHException& e) {
      sigprocmask(SIG_UNBLOCK, &additions, NULL);
      cerr << e.what() << endl;
      if (getpid() != stshpid) exit(0); // if exception is thrown from child process, kill it
    }
  }
  return 0;
}
