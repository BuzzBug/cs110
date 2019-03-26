/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _request_handler_
#define _request_handler_

#include <set>
#include <socket++/sockstream.h>
#include <utility>
#include <string>
#include "blacklist.h"
#include <mutex>
#include <string>
#include "cache.h"
#include <sstream>
#include "request.h"
#include "response.h"
#include <sstream>
#include "string.h"
#include "client-socket.h"

using namespace std;

class HTTPRequestHandler {
 public:
  HTTPRequestHandler();
  void serviceRequest(const std::pair<int, std::string>& connection) throw();
  void clearCache();
  void setCacheMaxAge(long maxAge);
  void SetRequestUsesProxy(bool desiredProxyUse, const std::string& proxyServer, unsigned short proxyPortNumber); 
 private:
  bool RequestUsesProxy;
  HTTPBlacklist blacklist;
  HTTPCache cache;
  mutex cacheLock;
  string proxyServer;
  unsigned short proxyPortNumber;
  void printRequestToConsole(HTTPRequest& request);
  bool CheckServerAllowed(HTTPRequest& request);
  bool HasCycle(string& ip_chain);
  bool CheckForCycles(HTTPRequest& request);
  void PublishFailure(int code, iosockstream &ss, HTTPResponse& response);
};

#endif
