/**
 * File: mapreduce-mapper.cc
 * -------------------------
 * Presents the implementation of the MapReduceMapper class,
 * which is charged with the responsibility of pressing through
 * a supplied input file through the provided executable and then
 * splaying that output into a large number of intermediate files
 * such that all keys that hash to the same value appear in the same
 * intermediate.
 */

#include "mapreduce-mapper.h"
#include "mr-names.h"
#include "string-utils.h"
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <iostream>
using namespace std;

MapReduceMapper::MapReduceMapper(const string& serverHost, unsigned short serverPort,
                                 const string& cwd, const string& executable,
                                 const string& outputPath, size_t numHashcodes) :
  MapReduceWorker(serverHost, serverPort, cwd, executable,
  outputPath),numhashcodes(numHashcodes) {}


void MapReduceMapper::map() const {
  while (true) {
    string name;
    if (!requestInput(name)) break;
    alertServerOfProgress("About to process \"" + name + "\".");
    string base = extractBase(name);
    string output = outputPath + "/" + changeExtension(base, "input", "mapped");
    
    bool success = processInput(name, output);
    if (!success)
    {
      notifyServer(name, success);
      continue;
    }
    
    vector<ofstream> outFiles;
    string stem = changeExtension(output, "mapped", "");
   
    /* open all output files at once */
    
    for (size_t i = 0; i < numhashcodes; i++)
    {
      string prodpath = stem + numberToString(i) + ".mapped";
      ofstream prodfile(prodpath);
      outFiles.push_back(move(prodfile));
    } 

    ifstream filehandle;
    filehandle.open(output.c_str());
    
    if(!filehandle.is_open())
    {
      cout << "unable to open input file: " << output << endl;
    }

    string line;
    while (getline(filehandle, line)) {
      istringstream iss(line);
      string key, value;
      iss >> key >> value;
      
      size_t hashValue = hash<string>()(key);
      size_t hashed = hashValue % numhashcodes;
      outFiles[hashed] << line << endl;
      outFiles[hashed].flush();
       
    } 
    filehandle.close();
    
    //close all the output files
    for (size_t i = 0; i < numhashcodes; ++i)
    {
      outFiles[i].close();
    }
    //remove the temporarily mapped file
    if(success)
    {
      remove(output.c_str());
    }
    notifyServer(name, success);
  }
  alertServerOfProgress("Server says no more input chunks, so shutting down.");
}
