/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), numAvailableWorkersSemaphore(numThreads), 
                                            numOutstandingThunks(0), availableWorkers(numThreads), 
                                            vecFnPointers(numThreads),
                                            vecDispatcherSigWorker(numThreads),
                                            isRunning(true),numOutstandingThunksSemaphore(0),
                                            numWorkers(numThreads) {
  for (size_t i = 0; i < numThreads; ++i)
  {
    vecDispatcherSigWorker[i].reset(new semaphore(0));
  }
  dt = thread([this](){
    dispatcher();
  });
  for (size_t workerID = 0; workerID < numThreads; workerID++)
  {
    availableWorkersLock.lock();
    availableWorkers[workerID] = true;
    availableWorkersLock.unlock();
    wts[workerID] = thread([this](size_t workerID){
      worker(workerID);
    }, workerID);
  }                                          
}

int ThreadPool::searchForAvailableWorker()
{
  lock_guard<mutex> lg(availableWorkersLock);
  for (size_t i = 0; i < numWorkers; ++i)
  {
    if(availableWorkers[i])
    {
      //availableWorkersLock.lock();
      availableWorkers[i] = false;
      //availableWorkersLock.unlock();
      return i;
    }
  }
  return -1;
}

function<void(void)> ThreadPool::GetNextFnPtr()
{
  lock_guard<mutex> lg(queueScheduledFunctionsLock);
  //if (queueScheduledFunctions.empty()) cout << "queue is empty!" << endl;
  function<void(void)> nextFn = queueScheduledFunctions.front();
  queueScheduledFunctions.pop();
  return nextFn;
}

void ThreadPool::dispatcher()
{
  while(true)
  {
    numOutstandingThunksSemaphore.wait();
    numAvailableWorkersSemaphore.wait();
    if(!isRunning) break;
    int availableWorkerIndex = searchForAvailableWorker();
    /*
    if(availableWorkerIndex == -1) 
    {
      cout << "no worker available!" << endl;
    }
    */
    function<void(void)> nextFn = GetNextFnPtr();
    vecFnPointers[availableWorkerIndex] = nextFn;
    vecDispatcherSigWorker[availableWorkerIndex]->signal();
  }
}

void ThreadPool::markWorkerAsAvailable(size_t workerID)
{
  availableWorkersLock.lock();
  availableWorkers[workerID] = true;
  availableWorkersLock.unlock();
  numOutstandingLock.lock();
  numOutstandingThunks--;
  if (numOutstandingThunks == 0) cv.notify_all();
  numOutstandingLock.unlock();
}

void ThreadPool::worker(size_t workerID)
{
  while(true)
  {
    vecDispatcherSigWorker[workerID]->wait();
    if(!isRunning) break;
    function<void(void)> thunk = vecFnPointers[workerID];
    thunk();
    markWorkerAsAvailable(workerID);
    numAvailableWorkersSemaphore.signal();
  }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
  queueScheduledFunctionsLock.lock();
  queueScheduledFunctions.push(thunk);
  queueScheduledFunctionsLock.unlock();
  numOutstandingLock.lock();
  numOutstandingThunks++;
  numOutstandingLock.unlock();
  numOutstandingThunksSemaphore.signal();
}

void ThreadPool::wait() {
  lock_guard<mutex> lg(numOutstandingLock);
  cv.wait(numOutstandingLock, [this]{return (numOutstandingThunks == 0);});
}

ThreadPool::~ThreadPool() {
  wait();
  isRunning = false;
  //get dispatcher to break
  numOutstandingThunksSemaphore.signal();
  numAvailableWorkersSemaphore.signal();
  //get workers to break
  for (size_t workerID = 0; workerID < numWorkers; workerID++)
  {
    vecDispatcherSigWorker[workerID]->signal();
  }
  dt.join();
  for (size_t i = 0; i < numWorkers; i++)
  {
    wts[i].join();
  }
  exit(0);
}
