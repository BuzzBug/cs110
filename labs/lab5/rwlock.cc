class rwlock {
 public:
  rwlock(): numReaders(0), writeState(Ready) {}
  void acquireAsReader();
  void acquireAsWriter();
  void release();

 private:
   int numReaders;
   enum { Ready, Pending, Writing } writeState;
   mutex readLock, stateLock;
   condition_variable_any readCond, stateCond;
};

void rwlock::acquireAsReader() {
  lock_guard<mutex> lgs(stateLock);
  stateCond.wait(stateLock, [this]{ return writeState == Ready; });
  lock_guard<mutex> lgr(readLock);
  numReaders++;
}

void rwlock::acquireAsWriter() {
  stateLock.lock();
  stateCond.wait(stateLock, [this]{ return writeState == Ready; });
  writeState = Pending;
  stateLock.unlock();
  lock_guard<mutex> lgr(readLock);
  readCond.wait(readLock, [this]{ return numReaders == 0; });
  writeState = Writing;
}

void rwlock::release() {
  stateLock.lock();
  if (writeState == Writing) {
    writeState = Ready;
    stateLock.unlock();
    stateCond.notify_all();
    return;
  }

  stateLock.unlock();
  lock_guard<mutex> lgr(readLock);
  numReaders--;
  if (numReaders == 0) readCond.notify_one();
}

//flag bool reports if thread is trying to upgrade, in other words, if acquireAsReader has been called
//Consider the following scenario: numReaders is initially 0, all 10 acquireAsReader so that numReaders is now 10, all 10 threads
//try to upgrade at the same time, by calling acquireAsWriter2.  Let's say one thread, call it ThreadOne, ThreadOne gets past the first
//if statement increments numUpgraders to 1.  It passes writeState == Ready test
//because acquireAsReader requires that writeState == Ready in order to proceed past the wait call.  Thus, ThreadOne can proceed
//past the stateCond.wait call.  ThreadOne then makes writeState in "Pending" state and releases the stateLock.  ThreadOne next
//takes ownership of the readLock, so it can check the condition numReaders == numUpgraders.  Because numReaders is 10 and numUpgraders
//is 1, the test does not pass and ThreadOne releases the readLock mutex and waits and blocks.
//Another thread let's call it ThreadTwo can now take ownership of the stateLock. It will increment numUpgraders to 2 and it will signal the
//the readCond at the wait.  ThreadOne which is waiting at readCond.wait(readLock...) will wake up, take ownership of the ReadLock, check the condition, see that it has
//not yet been satisfied, release readLock, and go back to sleep.  ThreadTwo will get stuck at stateCond.wait(stateLock...) because WriteState had already
//been set to Pending by ThreadOne.  ThreadThree to ThreadTen will proceed as ThreadTwo and eventually get stuck at stateCond.wait(stateLock)
//but by this point, numUpgraders has already been incremented to 10, equal to numReaders.  Now since numReaders == numUpgraders test passes and ThreadOne
//passes decrements numUpgraders to 9 and numReaders to 9, to mark that one reader has already been fully upgraded.  It sets writeState to be writing.  now
//ThreadOne calls release and resets writeSTate to Ready and notifies all threads, ThreadTwo to ThreadTen that are blocked at stateCond.wait(stateLock..)
//Now ThreadTwo to ThreadTen will compete for processor time. Let's say that ThreadTwo wins, it wil set writeState to be pending, stopping
//threads, ThreadThree to ThreadTen from proceeeding.  It will sail past the readCond.wait(readLock...) call because 9 == 9.  It will decrements
//numUpgraders to 8 and numReaders to 8 and set writeState to Writing.  Once ThreadTwo releases, writeState will be reset to ready, allowing another threads
//blocked at stateCond.wait() to proceed.
static int numUpgraders = 0;

void rwlock::acquireAsWriter2(bool flag) {
  stateLock.lock();
  if(flag)
  {
    numUpgraders++;
    readCond.notify_all();
  }
  stateCond.wait(stateLock, [this]{ return writeState == Ready; });
  writeState = Pending;
  stateLock.unlock();
  lock_guard<mutex> lgr(readLock);
  readCond.wait(readLock, [this]{ return numReaders == numUpgraders; });
  if (flag)
  {
    numUpgraders--;
    numReaders--;
  }
  writeState = Writing;
}
