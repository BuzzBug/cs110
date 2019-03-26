/**
 * File: mapreduce-reducer.cc
 * --------------------------
 * Presents the implementation of the MapReduceReducer class,
 * which is charged with the responsibility of collating all of the
 * intermediate files for a given hash number, sorting that collation,
 * grouping the sorted collation by key, and then pressing that result
 * through the reducer executable.
 *
 * See the documentation in mapreduce-reducer.h for more information.
 */
#include <mutex>
#include "mapreduce-reducer.h"
#include "mr-names.h"
#include "string-utils.h"
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <glob.h>
#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>

using namespace std;
std::mutex reducer_lock;

MapReduceReducer::MapReduceReducer(const string& serverHost, unsigned short serverPort,
                                   const string& cwd, const string& executable, const string& outputPath) : 
  MapReduceWorker(serverHost, serverPort, cwd, executable, outputPath) {}

inline std::vector<std::string> glob(const std::string& pat)
{
  using namespace std;
  glob_t glob_result;
  glob(pat.c_str(), GLOB_TILDE, NULL, &glob_result);
  vector<string> ret;
  for(unsigned int i = 0; i < glob_result.gl_pathc; ++i)
  {
    ret.push_back(string(glob_result.gl_pathv[i]));
  } 
  globfree(&glob_result);
  return ret;
}

/*
** reduce
*---------------------
* examine all files in a directory and see if they match the supplied
* regex.
* 1) collate the collection of intermediate files storing keys with the
* same hash code, take all the files that have the same hashed ending (12
* different files with the same "suffix"), combine them into one file
* 2) sort that collation, sort those 32 files
* 3) group that sorted collation by key
* 4) invoke the reducer executable on that sorted collation of
* key/vector-of-value pairs to produce final output files
*/

void MapReduceReducer::reduce() const {
  while(true)
  {
    // example pattern:
    // /afs/.ir.stanford.edu/users/c/c/cchen9/Desktop/classes/CS110W2016/assign6/files/intermediate/00000
    string pattern;
    if (!requestInput(pattern)) break;
    alertServerOfProgress("About to process \"" + pattern + "\".");
    // 1) access the files/intermediate directory and create a #.grouped
    // file where you will collate all the 000#.mapped files' contents
    // 2) loop through all files in directory to check if the file
    // stripped of "mapped" ends with the right extension
    // 3)
    string hashcode = pattern.substr(pattern.length() - 5);
    string intermediatePath = pattern.substr(0, pattern.length() - 5 - 1);
    string regex = intermediatePath + "/*." + hashcode + ".mapped";
    vector<string> found = glob(regex);    
    /* 
    // print out the vector found above
    cout << "hashcode is " << hashcode << " text = "; 
    std::ostream_iterator<string> out_it(std::cout,", ");
    std::copy(found.begin(), found.end(), out_it);
    cout << endl;
    */
    
    string aggregatename = pattern + ".grouped";
    std::ofstream aggregate(aggregatename);
    for (size_t i = 0; i < found.size(); i++)
    {
      std::ifstream if_a(found[i], std::ios_base::binary);
      aggregate << if_a.rdbuf();
    }
    
    aggregate.close();
    //sort the .grouped files sort -o 00007.sorted_grouped 00007.grouped
    string aggregate_sorted_name = pattern + ".sorted_grouped";
    string sortcommand = "sort < " + aggregatename + " > " + aggregate_sorted_name;
    //cout << "sortcommand: " << sortcommand << endl;
    //string sortcommand = "sort -o " + aggregate_sorted_name + " " + aggregatename; 
    system(sortcommand.c_str()); 
    // call group-by-key; python group-by-key.py <
    // files/intermediate/00001.sorted_grouped
    // python group-by-key.py < files/intermediate/00000.sorted_grouped >
    // output.txt
    string outputPath = "/afs/.ir.stanford.edu/users/c/c/cchen9/Desktop/classes/CS110S2017/assign8v2/files/output/";
    string path2grpbykey = "/afs/.ir.stanford.edu/users/c/c/cchen9/Desktop/classes/CS110S2017/assign8v2/group-by-key.py";
    string groupcommand = "python " + path2grpbykey + " < " + aggregate_sorted_name + " > " + outputPath + hashcode + ".coalesced";
    //cout << "groupCommand: " << groupcommand << endl;
    system(groupcommand.c_str());

    string coalesced = outputPath + hashcode + ".coalesced";
    string output = outputPath + hashcode + ".output";
    bool success = processInput(coalesced, output);
    if (!success)
    {
      remove(coalesced.c_str());
      notifyServer(pattern, success);
      continue;
    }
    //remove the coalesced file
    remove(coalesced.c_str());
    notifyServer(pattern, success);
  }
}
//implement this
//request input from server, input is 
//collate
//sort 
