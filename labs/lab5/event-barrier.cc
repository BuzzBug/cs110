/**
 * File: event-barrier.cc
 * ----------------------
 * Implements the EventBarrier class.
 */

#include "event-barrier.h"
using namespace std;

EventBarrier::EventBarrier() {
  barrier_lifted = false;
  numConsumersWaiting = 0;
  numConsumersPassed = 0;
}

void EventBarrier::wait() {
  numConsumersWaitingM.lock();
  numConsumersWaiting++;
  numConsumersWaitingM.unlock();
  lock_guard<mutex> lg(barrier_liftedM);
  cv.wait(barrier_liftedM, [this]{return barrier_lifted;});
}
void EventBarrier::lift() {
  barrier_liftedM.lock();
  barrier_lifted = true;
  barrier_liftedM.unlock();
  cv.notify_all();
  lock_guard<mutex> lg(numConsumersPassedM);
  cv2.wait(numConsumersPassedM, [this]{return (numConsumersPassed == numConsumersWaiting);});
}

void EventBarrier::past() {
  numConsumersPassedM.lock();
  numConsumersPassed++;
  numConsumersPassedM.unlock();
  cv2.notify_one();
}
