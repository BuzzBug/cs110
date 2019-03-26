/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */
#include <set> 
#include "cache.h"
#include <sstream>
#include "proxy-exception.h"
#include "request-handler.h"
#include "response.h"
#include "ostreamlock.h"
#include "request.h"

#include <socket++/sockstream.h> // for sockbuf, iosockstream
using namespace std;
//client issues a request to the proxy process, (socket descriptor, IP
//address of the machine you're browser is running on)

HTTPRequestHandler::HTTPRequestHandler(): RequestUsesProxy(false), blacklist(), cache(){
  try
  {
    blacklist.addToBlacklist("blocked-domains.txt");
  } catch (HTTPProxyException &hpe)
  {
    cerr << oslock << "Failure to add blacklist" << endl << osunlock;
    return;
  }
}

void HTTPRequestHandler::SetRequestUsesProxy(bool desiredProxyUse, const std::string& server, unsigned short port)
{
  RequestUsesProxy = desiredProxyUse;
  proxyServer = server;
  proxyPortNumber = port;
}

void HTTPRequestHandler::printRequestToConsole(HTTPRequest& request)
{
  cout << oslock << request.getMethod() << " " << request.getURL() << endl << osunlock;
}

bool HTTPRequestHandler::CheckServerAllowed(HTTPRequest& request)
{
  return blacklist.serverIsAllowed(request.getServer());
}

bool HTTPRequestHandler::HasCycle(string& ip_chain)
{
  set<string> ipSet;
  istringstream iss(ip_chain);
  bool HasCycle = false;
  string token;
  while(getline(iss, token, ','))
  {
    if(ipSet.find(token) != ipSet.end()){HasCycle = true; break;}
    else {ipSet.insert(token);}
  }
  return HasCycle;
}

bool HTTPRequestHandler::CheckForCycles(HTTPRequest& request)
{
  HTTPHeader header = request.getRequestHeader();
  string keychain("x-forwarded-for");
  if (!request.containsName(keychain)) {
    return false;
  }
  string ip_chain = header.getValueAsString(keychain);
  return HasCycle(ip_chain); 
}

void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {
  //first = clientfd, second = clientIPaddress
  HTTPRequest request;
  sockbuf sb(connection.first);
  iosockstream ss(&sb);
  HTTPResponse response; 
  try
  {
    request.ingestRequestLine(ss);
  } catch (HTTPBadRequestException &hbre)
  {
    PublishFailure(400, ss, response);
    return; 
  }
  request.ingestHeader(ss, connection.second);
  request.ingestPayload(ss);
  if (RequestUsesProxy)
  {
    request.setRequestProxy();
  }
  printRequestToConsole(request);
  //forward request to server to establish a connection with server
  //running on specified hostname and port
  if (!CheckServerAllowed(request))
  {
    PublishFailure(403, ss, response);
    return;
  }
  if (CheckForCycles(request)){
    cout << "have cycle!" << endl;
    PublishFailure(504, ss, response);
    return;
  }
  
  std::mutex& m = cache.GetHashedMutex(request);
  m.lock();
  //cacheLock.lock(); 
  bool UseCache = cache.containsCacheEntry(request, response);
  
  if(UseCache) { 
    cout << "uses cache!" << endl;
    ss << response; 
    ss.flush(); 
    cout << oslock << "Finished flushing response to client" << endl << osunlock;
    return;
  }
  int clientSocket;
  if(RequestUsesProxy)
  {
    cout << "created clientSocket for proxyserver " << endl;
    clientSocket = createClientSocket(proxyServer, proxyPortNumber);
  }
  else
  {
    clientSocket = createClientSocket(request.getServer(), request.getPort());
  }  
  //proxy is now a client, forward the request onto a server or the next proxy
  sockbuf sb_proxyasclient(clientSocket);
  iosockstream ss_proxyasclient(&sb_proxyasclient);
  ss_proxyasclient << request;
  ss_proxyasclient.flush();

  response.ingestResponseHeader(ss_proxyasclient);
  response.ingestPayload(ss_proxyasclient);
  
  if (cache.shouldCache(request, response)) {cout << "cached!";
   cache.cacheEntry(request, response); }
  
  ss << response;
  ss.flush();
  //cacheLock.unlock();
  m.unlock();
}

void HTTPRequestHandler::PublishFailure(int code, iosockstream &ss, HTTPResponse& response)
{
  response.setResponseCode(code);
  response.setProtocol("HTTP/1.1");
  string payload = response.retrieveStatusMessage(code);
  response.setPayload(payload);
  ss << response;
  ss.flush();
}

// the following two methods needs to be completed 
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {
  cache.clear();
}

void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
  cache.setMaxAge(maxAge);
}
