/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include "semaphore.h"
#include <queue>
#include <mutex>
#include <iostream>
#include <atomic>
#include "ostreamlock.h"
#include "thread-utils.h"
#include <condition_variable>
using namespace std;

class ThreadPool {
 public:

/**
 * Constructs a ThreadPool configured to spawn up to the specified
 * number of threads.
 */
  ThreadPool(size_t numThreads);

/**
 * Schedules the provided thunk (which is something that can
 * be invoked as a zero-argument function without a return value)
 * to be executed by one of the ThreadPool's threads as soon as
 * all previously scheduled thunks have been handled.
 */
  void schedule(const std::function<void(void)>& thunk);

/**
 * Blocks and waits until all previously scheduled thunks
 * have been executed in full.
 */
  void wait();

/**
 * Waits for all previously scheduled thunks to execute, and then
 * properly brings down the ThreadPool and any resources tapped
 * over the course of its lifetime.
 */
  ~ThreadPool();
  
 private:
  std::thread dt;                // dispatcher thread handle
  std::vector<std::thread> wts;  // worker thread handles
  semaphore numAvailableWorkersSemaphore; // for the workers to notify dispatcher that there is available worker that can receive more work
  size_t numOutstandingThunks;  //counter for number of thunks that have not finished executing
  vector<bool> availableWorkers; //for the dispatcher to check which worker is available
  vector<function<void(void)>> vecFnPointers; //vector for storing a thunk where the worker thread can find it
  vector<unique_ptr<semaphore>> vecDispatcherSigWorker; //vector for storing semaphores that the dispatcher can use to signal the appropriate worker
  bool isRunning;  //destructor communicates to thread (dispatchers and workers) that it can stop looping    
  std::queue<std::function<void(void)>> queueScheduledFunctions; //queue of scheduled functions
  mutex queueScheduledFunctionsLock; //mutex for protecting scheduled functions
  mutex numOutstandingLock;  //protect numOutstandingThunks
  semaphore numOutstandingThunksSemaphore; //for the scheduler to notify dispatcher that there is outstanding thunks, initialized to 0
  size_t numWorkers;
  condition_variable_any cv;
  mutex availableWorkersLock;
  void dispatcher();
  void worker(size_t workerID);
  int searchForAvailableWorker();
  void markWorkerAsAvailable(size_t workerID);
  std::function<void()> GetNextFnPtr();
/**
 * ThreadPools are the type of thing that shouldn't be cloneable, since it's
 * not clear what it means to clone a ThreadPool (should copies of all outstanding
 * functions to be executed be copied?).
 *
 * In order to prevent cloning, we remove the copy constructor and the
 * assignment operator.  By doing so, the compiler will ensure we never clone
 * a ThreadPool.
 */
  ThreadPool(const ThreadPool& original) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
