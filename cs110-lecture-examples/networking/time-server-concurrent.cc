/**
 * File: time-server-concurrent.cc
 * -------------------------------
 * Provided a concurrent version of the sequential
 * version served up in time-server-streams.cc
 */

#include <iostream>                // for cout, cett, endl
#include <ctime>                   // for time, gmtime, strftim
#include <sys/socket.h>            // for accept,  etc.
#include "socket++/sockstream.h"   // for sockbuf, iosockstream
#include "server-socket.h"
#include "thread-pool.h"
using namespace std;

static const unsigned short kDefaultPort = 12345;
static const int kWrongArgumentCount = 1;
static const int kServerStartFailure = 2;

static void publishTime(int clientSocket) {
  time_t rawtime;
  time(&rawtime);
  struct tm tm;
  gmtime_r(&rawtime, &tm);
  char timeString[128]; // more than big enough
  /* size_t len = */ strftime(timeString, sizeof(timeString), "%c", &tm);
  sockbuf sb(clientSocket); // destructor closes socket
  iosockstream ss(&sb);
  ss << timeString << endl;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    cerr << "Usage: " << argv[0] << endl;
    return kWrongArgumentCount;
  }
  
  int serverSocket = createServerSocket(kDefaultPort);
  if (serverSocket == kServerSocketFailure) {
    cerr << "Error: Could not start time server to listen to port " << kDefaultPort << "." << endl;
    cerr << "Aborting... " << endl;
    return kServerStartFailure;
  }
  
  cout << "Server listening on port " << kDefaultPort << "." << endl;  
  ThreadPool pool(4);
  while (true) {
    int clientSocket = accept(serverSocket, NULL, NULL);
    pool.schedule([clientSocket] { publishTime(clientSocket); });
  }
  return 0;
}
