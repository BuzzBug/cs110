/**
 * File: time-server-streams.cc
 * ----------------------------
 * Provided a sequential version of a server which
 * provides the simplest service possible--reporting
 * the time back to the client.  This first version
 * relies on raw socket descriptors and the write
 * system call to respond to the connecting client.
 */

#include <iostream>                // for cout, cett, endl
#include <ctime>                   // for time, gmtime, strftim
#include <sys/socket.h>            // for socket, bind, accept, listen, etc.
#include <climits>                 // for USHRT_MAX
#include "socket++/sockstream.h"   // for sockbuf, iosockstream
#include "server-socket.h"
using namespace std;

static const short kDefaultPort = 12345;
static const int kWrongArgumentCount = 1;
static const int kServerStartFailure = 2;
static void publishTime(int clientSocket) {
  time_t rawtime;
  time(&rawtime);
  struct tm *ptm = gmtime(&rawtime);
  char timeString[128]; // more than big enough
  /* size_t len = */ strftime(timeString, sizeof(timeString), "%c", ptm);
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  ss << timeString << endl;
} // sockbuf destructor closes clientSocket

int main(int argc, char *argv[]) {
  if (argc > 1) {
    cerr << "Usage: " << argv[0] << endl;
    return kWrongArgumentCount;
  }
  
  int serverSocket = createServerSocket(kDefaultPort);
  if (serverSocket == kServerSocketFailure) {
    cerr << "Error: Could not start server on port " << kDefaultPort << "." << endl;
    cerr << "Aborting... " << endl;
    return kServerStartFailure;
  }
  
  cout << "Server listening on port " << kDefaultPort << "." << endl;
  while (true) {
    int clientSocket = accept(serverSocket, NULL, NULL);
    publishTime(clientSocket);
  }

  return 0;
}
