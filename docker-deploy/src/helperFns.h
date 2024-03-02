#ifndef HELPER_FNS_H
#define HELPER_FNS_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <unordered_map>

extern std::unordered_map<std::string, std::string> cache;

bool isNotExpired(const std::string& rawResponse, const std::string& rawRequest);
std::string addIfNoneMatch(const std::string& request);
std::string addIfModifiedSince(const std::string& request);
std::string getFirstLine(std::string& msg);

void runProxy();
void * processRequest(void * user_);

// implementation
std::tm parseDate(const std::string& dateStr);
int parseMaxAge(const std::string& cacheControlStr);
int parseSMaxAge(const std::string& cacheControlStr);
std::string formatDate(std::time_t time);
std::string calculateExpiration(const std::string& dateStr, const std::string& cacheControlStr);
bool compareCurrAndExpires(const std::string& expirationTime);
bool strHasSubstr(const std::string& str, const std::string& subStr);

#endif