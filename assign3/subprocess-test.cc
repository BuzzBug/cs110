/**
 * File: subprocess-test.cc
 * ------------------------
 * Simple unit test framework in place to exercise functionality of the subprocess function.
 */

#include "subprocess.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <ext/stdio_filebuf.h>

using namespace __gnu_cxx; // __gnu_cxx::stdio_filebuf -> stdio_filebuf
using namespace std;

/**
 * File: publishWordsToChild
 * -------------------------
 * Algorithmically self-explanatory.  Relies on a g++ extension where iostreams can
 * be wrapped around file desriptors so that we can use operator<<, getline, endl, etc.
 */
const string kWords[] = {"put", "a", "ring", "on", "it"};
static void publishWordsToChild(int to) {
  stdio_filebuf<char> outbuf(to, std::ios::out);
  ostream os(&outbuf); // manufacture an ostream out of a write-oriented file descriptor so we can use C++ streams semantics (prettier!)
  for (const string& word: kWords) os << "testing supply: " << word << endl;
} // stdio_filebuf destroyed, destructor calls close on desciptor it owns
    
/**
 * File: ingestAndPublishWords
 * ---------------------------
 * Reads in everything from the provided file descriptor, which should be
 * the sorted content that the child process running /usr/bin/sort publishes to
 * its standard out.  Note that we one again rely on the same g++ extenstion that
 * allows us to wrap an iostream around a file descriptor so we have C++ stream semantics
 * available to us.
 */
static void ingestAndPublishWords(int from) {
  stdio_filebuf<char> inbuf(from, std::ios::in);
  istream is(&inbuf);
  while (true) {
    string word;
    getline(is, word);
    if (is.fail()) break;
    cout << "testing ingest from ingestandpublish function: " << word << endl;
  }
} // stdio_filebuf destroyed, destructor calls close on desciptor it owns

/**
 * Function: waitForChildProcess
 * -----------------------------
 * Halts execution until the process with the provided id exits.
 */
static void waitForChildProcess(pid_t pid) {
  if (waitpid(pid, NULL, 0) != pid) {
    throw SubprocessException("Encountered a problem while waiting for subprocess's process to finish.");
  }
}

/**
 * Function: main
 * --------------
 * Serves as the entry point for for the unit test.
 */
const string kSortExecutable = "/usr/bin/sort";
const string kDateExecutable = "date";
const string kEchoExecutable = "echo 12345";
int test1(void)
{
  //expect to print to console with message
  char *argv[] = {const_cast<char *>(kDateExecutable.c_str()), NULL};
  subprocess_t child = subprocess(argv, false, true);
  publishWordsToChild(child.supplyfd); 
  ingestAndPublishWords(child.ingestfd);
  waitForChildProcess(child.pid);
  return 0;
}

int test2(void)
{
  //expect child to print to console without message 
  char *argv[] = {const_cast<char *>(kSortExecutable.c_str()), NULL};
  subprocess_t child = subprocess(argv, true, false);
  publishWordsToChild(child.supplyfd);
  ingestAndPublishWords(child.ingestfd);
  waitForChildProcess(child.pid);
  return 0;
}

int test3(void)
{
  //expect child to print to console with message
  char *argv[] = {const_cast<char *>(kSortExecutable.c_str()), NULL};
  subprocess_t child = subprocess(argv, true, true);
  publishWordsToChild(child.supplyfd);
  ingestAndPublishWords(child.ingestfd);
  waitForChildProcess(child.pid);
  return 0;
}

int test4(void)
{ 
  //expect to print to console no message
  char *argv[] = {const_cast<char *>(kDateExecutable.c_str()), NULL};
  subprocess_t child = subprocess(argv, false, false);
  publishWordsToChild(child.supplyfd);
  ingestAndPublishWords(child.ingestfd);
  waitForChildProcess(child.pid);
  return 0;
}

//ingestChildOutput is true so should expect the ingestAndPublishWords
//function to print 12345 to the console with the "ingest" message, it
//should not go directly to console
int test4b(void)
{
  //char *argv1[] = {const_cast<char *>(kEchoExecutable.c_str()), NULL};
  char *argv1[] = {const_cast<char *>("echo"), const_cast<char *>("12345"), NULL};

  subprocess_t child = subprocess(argv1, false, true);
  publishWordsToChild(child.supplyfd);
  ingestAndPublishWords(child.ingestfd);
  waitForChildProcess(child.pid);
  return 0;
}

int test5(void)
{
  char *argv1[] = {const_cast<char *>("sleep"), const_cast<char *>("5"), NULL};
  subprocess_t child = subprocess(argv1, false, false);
  waitForChildProcess(child.pid);
  return 0;
}
int main(int argc, char *argv[]) {
  try {
    test1();
    test2();
    test3();
    test4();
    test4b();
    test5();
  } catch (const SubprocessException& se) {
    cerr << "Problem encountered while spawning second process to run \"" << kSortExecutable << "\"." << endl;
    cerr << "More details here: " << se.what() << endl;
    return 1;
  } catch (...) { // ... here means catch everything else
    cerr << "Unknown internal error." << endl;
    return 2;
  }
}
