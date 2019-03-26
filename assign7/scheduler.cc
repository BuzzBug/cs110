/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include <utility>
using namespace std;

HTTPProxyScheduler::HTTPProxyScheduler():requestpool(num_threads){}

void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress) throw () {
  //requestHandler.serviceRequest(make_pair(clientfd, clientIPAddress));
  requestpool.schedule([clientfd, clientIPAddress,
  this]{requestHandler.serviceRequest(make_pair(clientfd,
  clientIPAddress));}); 
}

void HTTPProxyScheduler::setProxy(const std::string& server, unsigned short port){
  bool desiredProxyUse = true;
  requestHandler.SetRequestUsesProxy(desiredProxyUse, server, port);
}
