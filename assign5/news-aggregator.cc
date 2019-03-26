/**
 * File: news-aggregator.cc
 * --------------------------------
 * Presents the implementation of the NewsAggregator class.
 */

#include "news-aggregator.h"
#include <iostream>
#include <iomanip>
#include <getopt.h>
#include <libxml/parser.h>
#include <libxml/catalog.h>
// you will almost certainly need to add more system header includes

// I'm not giving away too much detail here by leaking the #includes below,
// which contribute to the official CS110 staff solution.
#include "rss-feed.h"
#include "rss-feed-list.h"
#include "html-document.h"
#include "html-document-exception.h"
#include "rss-feed-exception.h"
#include "rss-feed-list-exception.h"
#include "utils.h"
#include "ostreamlock.h"
#include "string-utils.h"
#include "utils.h"
#include "article.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include "ostreamlock.h"
#include "semaphore.h"
#include "thread-utils.h"
#include <map>
#include <set>
#define MAXARTICLEDOWNLOAD 18
#define MAXCHILDTHREAD 5
#define MAXSERVERCONNECTIONS 8

using namespace std;

/**
 * Factory Method: createNewsAggregator
 * ------------------------------------
 * Factory method that spends most of its energy parsing the argument vector
 * to decide what rss feed list to process and whether to print lots of
 * of logging information as it does so.
 */
static const string kDefaultRSSFeedListURL = "small-feed.xml";
NewsAggregator *NewsAggregator::createNewsAggregator(int argc, char *argv[]) {
  struct option options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"url", required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0},
  };
  
  string rssFeedListURI = kDefaultRSSFeedListURL;
  bool verbose = false;
  while (true) {
    int ch = getopt_long(argc, argv, "vqu:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'v':
      verbose = true;
      break;
    case 'q':
      verbose = false;
      break;
    case 'u':
      rssFeedListURI = optarg;
      break;
    default:
      NewsAggregatorLog::printUsage("Unrecognized flag.", argv[0]);
    }
  }
  
  argc -= optind;
  if (argc > 0) NewsAggregatorLog::printUsage("Too many arguments.", argv[0]);
  return new NewsAggregator(rssFeedListURI, verbose);
}

/**
 * Method: buildIndex
 * ------------------
 * Initalizex the XML parser, processes all feeds, and then
 * cleans up the parser.  The lion's share of the work is passed
 * on to processAllFeeds, which you will need to implement.
 */
void NewsAggregator::buildIndex() {
  if (built) return;
  built = true; // optimistically assume it'll all work out
  xmlInitParser();
  xmlInitializeCatalog();
  processAllFeeds();
  xmlCatalogCleanup();
  xmlCleanupParser();
}

/**
 * Method: queryIndex
 * ------------------
 * Interacts with the user via a custom command line, allowing
 * the user to surface all of the news articles that contains a particular
 * search term.
 */
void NewsAggregator::queryIndex() const {
  static const size_t kMaxMatchesToShow = 15;
  while (true) {
    cout << "Enter a search term [or just hit <enter> to quit]: ";
    string response;
    getline(cin, response);
    response = trim(response);
    if (response.empty()) break;
    const vector<pair<Article, int> >& matches = index.getMatchingArticles(response);
    if (matches.empty()) {
      cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
    } else {
      cout << "That term appears in " << matches.size() << " article"
           << (matches.size() == 1 ? "" : "s") << ".  ";
      if (matches.size() > kMaxMatchesToShow)
        cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
      else if (matches.size() > 1)
        cout << "Here they are:" << endl;
      else
        cout << "Here it is:" << endl;
      size_t count = 0;
      for (const pair<Article, int>& match: matches) {
        if (count == kMaxMatchesToShow) break;
        count++;
        string title = match.first.title;
        if (shouldTruncate(title)) title = truncate(title);
        string url = match.first.url;
        if (shouldTruncate(url)) url = truncate(url);
        string times = match.second == 1 ? "time" : "times";
        cout << "  " << setw(2) << setfill(' ') << count << ".) "
             << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
        cout << "       \"" << url << "\"" << endl;
      }
    }
  }
}

/**
 * Private Constructor: NewsAggregator
 * -----------------------------------
 * Self-explanatory.  You may need to add a few lines of code to
 * initialize any additional fields you add to the private section
 * of the class definition.
 */
NewsAggregator::NewsAggregator(const string& rssFeedListURI, bool verbose): 
  log(verbose), rssFeedListURI(rssFeedListURI), built(false), TotalArticleDwnldSemaphore(MAXARTICLEDOWNLOAD),
  NewsFeedSemaphore(MAXCHILDTHREAD) {}

/**
 * Private Method: ServerSemaphoreWait()
 */

void NewsAggregator::ServerSemaphoreWait(const string& article_url_server)
{
  ServerSemaphoreMapLock.lock();
  unique_ptr<semaphore>& articleurlsemaphore = ServerSemaphore[article_url_server];
  if (articleurlsemaphore == nullptr) articleurlsemaphore.reset(new semaphore(MAXSERVERCONNECTIONS));
  ServerSemaphoreMapLock.unlock();
  //if you don't release the lock, the thread will be sleeping and holding
  //the lock other threads cannot acquire the lock
  articleurlsemaphore->wait();

}

/**
 * Private Method: Downloaded
 */

bool NewsAggregator::Downloaded(const string& url)
{
  lock_guard<mutex> lg(ArticleLock);
  bool URLIsDownloaded = (DownloadedURLs.find(url) != DownloadedURLs.end());
  return URLIsDownloaded;
}

/**
 * Private Method: UpdateArticleDownloads
 */

void NewsAggregator::UpdateArticleDownloads(const string& url)
{
  lock_guard<mutex> lg(ArticleLock);
  DownloadedURLs.insert(url);
}

/**
 * Private Method: DownloadArticle
 */

void NewsAggregator::DownloadArticle(const Article& article)
{
  TotalArticleDwnldSemaphore.signal(on_thread_exit);
  ServerSemaphoreMapLock.lock();
  auto found = ServerSemaphore.find(getURLServer(article.url));
  ServerSemaphoreMapLock.unlock();
  if (found == ServerSemaphore.end()){
    cout << "Error: feedUrl not found in map!" << endl;
    exit(-1);
  }
  unique_ptr<semaphore>& feedurlsemaphore = found->second;
  feedurlsemaphore->signal(on_thread_exit);
  HTMLDocument htmldocument(article.url);
  try {
    htmldocument.parse();
  } catch (const HTMLDocumentException &hde) {
    log.noteSingleArticleDownloadFailure(article);
    return;
  }
  
  UpdateArticleDownloads(article.url);
  vector<string> words = htmldocument.getTokens();
  sort(words.begin(), words.end());
  //Key: (title of article, url of article)
  map<pair<string, string>, pair<vector<string>, string>>::iterator MapTitleIt;
  pair<string, string> key(article.title, getURLServer(article.url));     
  MapTitleLock.lock();
  MapTitleIt = MapTitle.find(key);
  if (MapTitleIt == MapTitle.end())
  {
    pair<vector<string>, string> value(words, article.url);
    MapTitle[key] = value;
    MapTitleLock.unlock();
  }
  else
  {
    pair<vector<string>, string> currvalue = MapTitle[key];
    MapTitleLock.unlock();
    vector<string> currtokens = currvalue.first;
    string newurl = currvalue.second;
    if(article.url < currvalue.second) newurl = article.url;
    vector<string> mergedlist;
    set_intersection(currtokens.cbegin(), currtokens.cend(),words.cbegin(), words.cend(), back_inserter(mergedlist));
    sort(mergedlist.begin(), mergedlist.end());
    pair<vector<string>, string> value(mergedlist, newurl);
    MapTitleLock.lock();
    MapTitle[key] = value;
    MapTitleLock.unlock();
  }
}
/**
 * Private Method: processFeed
 */
void NewsAggregator::processFeed(const string& feedurl)
{
  NewsFeedSemaphore.signal(on_thread_exit);
  RSSFeed feed(feedurl);
  try{
    feed.parse();
  } catch (const RSSFeedException& rfe){
    log.noteSingleFeedDownloadFailure(feedurl);
    return;
  }
  const vector<Article>& articles = feed.getArticles();
  vector<thread> ArticleThreads;
  
  for (const auto& article: articles)
  {
    if(Downloaded(article.url)) continue;
    ServerSemaphoreWait(getURLServer(article.url)); //map article url server to correct semaphore counter
    TotalArticleDwnldSemaphore.wait(); //increase the count of dwnloaded articles
    ArticleThreads.push_back(thread([this, article]{this->DownloadArticle(article);}));
  }
  for (thread& t: ArticleThreads) t.join();
}

/**
 * Private Method: processAllFeeds
 * -------------------------------
 * Downloads and parses the encapsulated RSSFeedList, which itself
 * leads to RSSFeeds, which themsleves lead to HTMLDocuemnts, which
 * can be collectively parsed for their tokens to build a huge RSSIndex.
 * 
 * The vast majority of your Assignment 5 work has you implement this
 * method using multithreading while respecting the imposed constraints
 * outlined in the spec.
 */

void NewsAggregator::processAllFeeds() {
  RSSFeedList feedlist(rssFeedListURI);
  try{
    feedlist.parse();
  } catch (const RSSFeedListException& rfle){
    log.noteFullRSSFeedListDownloadFailureAndExit(rssFeedListURI);
    return;
  }
  const auto& feeds = feedlist.getFeeds();
  vector<thread> FeedURLThreads;
  for (const auto& feed_entry: feeds)
  {
    const string& feed_url = feed_entry.first;
    NewsFeedSemaphore.wait();
    FeedURLThreads.push_back(thread([this,feed_url]{this->processFeed(feed_url);}));
    //FeedURLThreads.push_back(thread(&NewsAggregator::processFeed,this,feed_url));
  }
  for (thread& t: FeedURLThreads) t.join();

  for (const auto& entry: MapTitle)
  {
    pair<string, string> key = entry.first;
    pair<vector<string>, string> value = entry.second;
    Article a;
    //value: vector of words, url
    //key: title, url
    a.url = value.second;
    vector<string> words = value.first;
    a.title = key.first;
    index.add(a, words);
  }
}
