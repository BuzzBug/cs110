#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <algorithm>
#include <string.h>

using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

imdb::~imdb() {
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
  int numActors = *(int *)actorFile;
  int *offset_base_start = (int *)((char *)actorFile + sizeof(int));
  int *offset_base_end = (int *)((char *)actorFile + sizeof(int) + numActors * sizeof(int));
  int *lower = std::lower_bound(offset_base_start, offset_base_end, player, [this](int elem, std::string v){char *p = (char *)actorFile + elem; int name_length = strlen(p); const string str(p,name_length); return str < v;});  
  if (lower == offset_base_end)
  {
    return false;
  }
  bool found;
  char *actor_start = (char *)actorFile + *lower;
  int name_length = strlen(actor_start);
  const string str(actor_start, name_length);
  if (actor_start != player)
  {
    found = false;
  }
  else
  {
    found = true;
    int alotted_length;
    if (name_length % 2 == 0)
    {
      alotted_length = name_length + 2;
    }
    else
    {
      alotted_length = name_length + 1;
    }
    void *numMoviesaddr = (char *)actor_start + alotted_length;
    short numMovies = *(short *)numMoviesaddr;
    int sum_alotted_length = alotted_length + sizeof(short);
    void *movieOffsetaddr = (char *)numMoviesaddr + sizeof(short);
    if (sum_alotted_length % 4 != 0)
    {
      movieOffsetaddr = (char *)movieOffsetaddr + 2;
    } 
    for (short j = 0; j < numMovies; j++)
    {
      int movie_offset_j = *(int *)((char *)movieOffsetaddr + (j * sizeof(int)));
      char *movie_start = (char *)movieFile + movie_offset_j;
      int movie_name_length = strlen(movie_start);
      const string str_m(movie_start, movie_name_length);
      int moviename_alotted_length = movie_name_length + 1;
      void *year_addr = movie_start + moviename_alotted_length;
      int movie_year = *(char *)year_addr;
      movie_year += 1900;
      struct film filmstruct;
      filmstruct.title = str_m;
      filmstruct.year = movie_year;
      films.push_back(filmstruct);
    } 
  }
  return found;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 

  int numMovies = *(int *)movieFile;
  int *offset_base_start = (int *)((char *)movieFile + sizeof(int));
  int *offset_base_end = (int *)((char *)movieFile + sizeof(int) + numMovies * sizeof(int));
  
  int *lower = std::lower_bound(offset_base_start, offset_base_end, movie, [this] (int elem, struct film fval){char *ms = (char *)movieFile + elem; 
  int name_length = strlen(ms); const string str(ms, name_length); void *year_addr = ms + name_length + 1; int movie_year = *(char *)year_addr; 
  movie_year += 1900; struct film film_iter; film_iter.title = str;
  film_iter.year = movie_year; return film_iter < fval;});                                                                                                  
  if (lower == offset_base_end)
  {
    return false;
  } 
  char *ml = (char *)movieFile + *lower;
  int name_length = strlen(ml);
  const string strl(ml, name_length);
  void *year_addr = ml + name_length + 1;
  int movie_year = *(char *)year_addr;
  movie_year += 1900;

  bool found;
  
  if ((strl == movie.title) && (movie_year == movie.year))
  {
    found = true;
    int moviename_alotted_length = name_length + 1;
    int sum_alotted_length = moviename_alotted_length + 1;
      
    // if this length is odd, add extra \0
    if (sum_alotted_length % 2 != 0)
    {
      //numActorsaddr = (char *)numActorsaddr + 1;
      sum_alotted_length += 1;
    }
    //sum_alotted_length contains length of movie name and year
    void *numActorsaddr = ml + sum_alotted_length;
      
    short numActors = *(short *)numActorsaddr;
      
    //add the size of the number of Actors to sum_alotted_length
    sum_alotted_length += sizeof(short);

    //pad with two additional bytes of zeros if not a multiple of four
    if (sum_alotted_length % 4 != 0)
    {
      //add with 4 - l % 4 
      sum_alotted_length += 2;
    }

    void *actorOffsetaddr = ml + sum_alotted_length;
    for (short j = 0; j < numActors; j++)
    {
      int actor_offset_j = *(int *)((char *)actorOffsetaddr + (j * sizeof(int)));
      char *actor_start = (char *)actorFile + actor_offset_j;
      int actor_name_length = strlen(actor_start);
      const string str_a(actor_start, actor_name_length);
      players.push_back(str_a);
    }
    
  }
  else
  {
    found = false;
  }
  return found;
  
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
