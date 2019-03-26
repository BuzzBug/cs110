#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"
#include "exit-utils.h"
#include <errno.h>
#include <stdio.h>

using namespace std;

static const int kWaitFailed = 3;

struct worker {
  worker() {}
  worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
  subprocess_t sp;
  bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
// restore static keyword once you start using these, commented out to suppress compiler warning
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig) {
  pid_t pid;
  while(true)
  {
    pid = waitpid(-1, NULL, WNOHANG | WUNTRACED);
    if (pid <= 0) break;
    for (size_t i = 0; i < kNumCPUs; i++)
    {
      if (workers[i].sp.pid == pid)
      {
        workers[i].available = true;
        numWorkersAvailable++;
        break;
      }
    }
  }
  exitUnless(pid == 0 || errno == ECHILD, kWaitFailed, stderr, "waitpid failed within markWorkerAsAvailable sighandler.\n");
}

// restore static keyword once you start using it, commented out to suppress compiler warning
static const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers() {
  cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through " << kNumCPUs - 1 << "." << endl;
  for (size_t i = 0; i < kNumCPUs; i++) {
    workers[i] = worker(const_cast<char **>(kWorkerArguments));
    cpu_set_t cpusetp;
    CPU_ZERO(&cpusetp);
    CPU_SET(i, &cpusetp);
    sched_setaffinity(workers[i].sp.pid, sizeof(cpu_set_t), &cpusetp); 
    cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
  }
}

// restore static keyword once you start using it, commented out to suppress compiler warning
static size_t getAvailableWorker() {
  sigset_t additions, existingmask;
  sigemptyset(&additions);
  sigaddset(&additions, SIGCHLD);
  sigprocmask(SIG_BLOCK, &additions, &existingmask);
  while(numWorkersAvailable == 0)
  {
    sigsuspend(&existingmask);
  }
  size_t worker_available;
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    if (workers[i].available)
    {
       worker_available = i;
       workers[i].available = false;
       numWorkersAvailable--;
       break;
    }
  }
  sigprocmask(SIG_UNBLOCK, &additions, NULL); 
  return worker_available;
}

static void broadcastNumbersToWorkers() {
  while (true) {
    string line;
    getline(cin, line);
    if (cin.fail()) break;
    size_t endpos;
    long long num = stoll(line, &endpos);
    cout << "number: " << num << endl;
    if (endpos != line.size()) break;
    size_t index_available = getAvailableWorker();
    dprintf(workers[index_available].sp.supplyfd, "%lld\n", num);
    kill(workers[index_available].sp.pid, SIGCONT);
  }
}

static void waitForAllWorkers()
{
  for (size_t i = 0; i < kNumCPUs; i++)
  {
    getAvailableWorker();
  }

  for (size_t child = 0; child < kNumCPUs; child++)
  {
    close(workers[child].sp.supplyfd);
    kill(workers[child].sp.pid, SIGCONT);
  }

  for (size_t i = 0; i < kNumCPUs; i++)
  {
    waitpid(-1, NULL, WUNTRACED); 
  }
}

static void closeAllWorkers() {
  signal(SIGCHLD, SIG_DFL);
}

int main(int argc, char *argv[]) {
  signal(SIGCHLD, markWorkersAsAvailable);
  spawnAllWorkers();
  broadcastNumbersToWorkers();
  waitForAllWorkers();
  closeAllWorkers();
  return 0;
}
