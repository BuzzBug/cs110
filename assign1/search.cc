#include <string>
#include <vector>
#include "imdb.h"
#include "imdb-utils.h"
#include "path.h"
#include <set>
#include <list>
#include <queue>
#include <iostream>

using namespace std;

static int getNumCostars(const imdb& db, const string& player)
{
  vector<film> credits;
  db.getCredits(player, credits);
  set<string> costars;
  for (int i = 0; i < (int) credits.size(); i++)
  {
    const film& movie = credits[i];
    vector<string> cast;
    db.getCast(movie, cast);
    for (int j = 0; j < (int)cast.size(); j++)
    {
      const string& costar = cast[j];
      if (costar != player)
      {
        costars.insert(costar);
      }
    }
  }
  return costars.size();
}
void BFS(const string& start_arg, const string& target_arg)
{
  imdb db(kIMDBDataDirectory);
  list<path> queue;
  bool reversed = false;
  int startarg_numCostars = getNumCostars(db, start_arg);
  int targetarg_numCostars = getNumCostars(db, target_arg);
  string start;
  string target;
  if (targetarg_numCostars < startarg_numCostars)
  {
    reversed = true;
    start = target_arg;
    target = start_arg;
  }
  else
  {
    start = start_arg;
    target = target_arg;
  }
  set<string> visitedActors;
  set<film> visitedFilms;
  path path0 {start};
  queue.push_back(path0);
  visitedActors.insert(start);
  while(!queue.empty())
  {
    path currpath(queue.front());
    queue.pop_front();
    const string& lastPlayer = currpath.getLastPlayer();
    
    int numHops = currpath.getLength();
    
    if (numHops == 7)
    {
      cerr << "No path could be found between these two people." << endl;
      return;
    }
    
    vector<film> credits;
    db.getCredits(lastPlayer, credits);
    for (auto& film_i: credits)
    {
      pair<std::set<film>::iterator,bool> ret;
      ret = visitedFilms.insert(film_i);
      if (ret.second == false)
      {
        continue;
      }
      vector<string> cast;
      db.getCast(film_i, cast);
      for (auto& actor_i: cast)
      {
        pair<std::set<string>::iterator, bool> ret2;
        ret2 = visitedActors.insert(actor_i);
        if (ret2.second == false)
        {
          continue;
        }
        path newpath(currpath);
        newpath.addConnection(film_i, actor_i);
        if (actor_i == target)
        {
          if(reversed){
            newpath.reverse();
          }
          cout << newpath;
          return;
        }   
         
        if (newpath.getLength() == 7)
        {
          continue;
        }
        queue.push_back(newpath);
      }
    }
  }
  cout << "No path could be found between these two actors" << endl;
}

int main(int argc, char *argv[]) {
  if (argc != 3)
  {
    cout << "Usage: slink/search_soln <source-actor> <target-actor>" << endl;
    return -1;
  }
  const string start(argv[1]);
  const string target(argv[2]);
  if (start == target)
  {
    cout << "Ensure that source and target are different!" << endl;
    return -1;
  }
  BFS(start, target);
  return 0;
}
