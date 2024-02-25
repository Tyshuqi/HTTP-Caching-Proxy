#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include "HTTPResponseParser.h"

std::tm parseDate(const std::string& dateStr);
int parseMaxAge(const std::string& cacheControlStr);
int parseSMaxAge(const std::string& cacheControlStr);
std::string formatDate(std::time_t time);
std::string calculateExpiration(const std::string& dateStr, const std::string& cacheControlStr);
bool compareCurrAndExpires(const std::string& expirationTime);
bool isNotExpired(const std::string& dateStr, const std::string& cacheControlStr);
