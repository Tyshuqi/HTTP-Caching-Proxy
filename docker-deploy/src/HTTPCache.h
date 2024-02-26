#ifndef HTTP_CACHE_H
#define HTTP_CACHE_H

#include <unordered_map>
#include <string>
#include <chrono>
#include <mutex>

class CacheEntry {
public:
    std::string data;
    std::chrono::system_clock::time_point expiry;
    bool mustRevalidate = false;
    // Consider if you need maxStale as a duration or specific behavior
    //std::chrono::seconds maxStale;
};

class HTTPCache {
private:
    std::unordered_map<std::string, CacheEntry> cache;
    std::mutex cacheMutex;

public:
    // Checks if the cache contains a valid entry for the given key
    bool isValid(const std::string& key, int clientMaxStale) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cache.find(key);
        // 1.Not in cache
        if (it == cache.end()) {
            return false; // Not in cache
        }
        // 2.In cache & still fresh(not exceed expiry time)
        auto now = std::chrono::system_clock::now();
        if (it->second.expiry > now) {
            return true; // Still fresh
        }

        // Check for expired but within maxStale limit if mustRevalidate is false
        // if (!it->second.mustRevalidate) {
        //     auto expiredDuration = now - it->second.expiry;
        //     if (expiredDuration <= it->second.maxStale) {
        //         return true; // Expired but within maxStale limit
        //     }
        // }
        // 3. In cache & not fresh & client request can accept stale response within clientMaxStale duration
        auto expiredDuration = now - it->second.expiry;
        if (!it->second.mustRevalidate && clientMaxStale >= expiredDuration.count()) {
        return true;
    }
        
        return false; // Otherwise expired
    }

    // Retrieves a cache entry
    std::string get(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cache[key].data; // Assuming the caller checks isValid first
    }

    // Adds or updates a cache entry
    void put(const std::string& key, const CacheEntry& entry) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        cache[key] = entry;
    }
};

#endif