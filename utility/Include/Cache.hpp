#pragma once

#include <map>
#include "Misc.hpp"

namespace utility
{
    template <typename T>
    struct CacheEntry
    {
        public:
            T Item;
            unsigned int SerialNumber;

            CacheEntry()
            {
            }

            CacheEntry(T item, unsigned int serialNumber)
            {
                Item = item;
                SerialNumber = serialNumber;
            }
    };

    template <typename TKey, typename TValue>
    class Cache
    {
        private:
            unsigned int _cacheLength, _serialNumber;
            std::map<TKey, CacheEntry<TValue>> _map;

            void RemoveOldestCacheItem()
            {
                unsigned int minSerialNumber = 0xFFFFFFFF;
                
                std::map<TKey, CacheEntry<TValue>>::iterator it;
                TKey minKey;

                for (it = _map.begin(); it != _map.end(); ++it)
                {
                    CacheEntry<TValue> entry = it->second;
                    if (entry.SerialNumber < minSerialNumber)
                    {
                        minKey = it->first;
                        minSerialNumber = entry.SerialNumber;
                    }
                }

                _map.erase(minKey);
            }

        public:
            Cache<TKey, TValue>(unsigned int cacheLength)
            {
                _cacheLength = cacheLength;
                _serialNumber = 0;
            }

            bool Get(TKey key, TValue &value)
            {
                if (_map.find(key) == _map.end())
                {
                    value = 0;
                    return false;
                }

                CacheEntry<TValue> entry = _map[key];
                value = entry.Item;
                return true;
            }

            void Insert(TKey key, TValue value)
            {
                if (_map.find(key) != _map.end())
                    return;

                if (_map.size() >= _cacheLength)
                    RemoveOldestCacheItem();

                _map[key] = CacheEntry<TValue>(value, _serialNumber++);
            }
    };
}