#pragma once
#include <cassert>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

template<typename T>
class Catalog
{
public:
    Catalog() {}

    bool Load(std::string path, int flags = 0)
    {
        size_t index;
        auto it = index_by_path.find(path);
        if (it != index_by_path.end()) {
            index = it->second;
        } else {
            index = entries.size();
            entries.push_back(T());
            index_by_path[path] = index;
        }
        return onLoad(entries[index], path, flags);
    }

    bool Exists(std::string path)
    {
        return index_by_path.count(path) > 0;
    }

    T &Find(std::string path)
    {
        auto it = index_by_path.find(path);
        if (it != index_by_path.end()) {
            size_t index = it->second;
            assert(index < entries.size());
            return entries[index];
        } else {
            std::cerr << "WARN: Catalog.Find(\"" << path << "\") did not find a match. Attempting fallback..." << std::endl;
            return onMissing(path);
        }
    }

    T &operator[](std::string path)
    {
        return Find(path);
    }

private:
    virtual bool onLoad(T &entry, std::string path, int flags) = 0;
    virtual T &onMissing(std::string path) = 0;

    std::vector<T> entries;
    std::unordered_map<std::string, size_t> index_by_path;
};