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

    bool Find(std::string path, T *&result)
    {
        auto it = index_by_path.find(path);
        if (it != index_by_path.end()) {
            size_t index = it->second;
            assert(index < entries.size());
            result = &entries[index];
            return true;
        }
        std::cerr << "WARN: Catalog.Find() did not find a match" << std::endl;
        assert(!"Catalog.Find() did not find a match");
        return false;
    }

    T *operator[](std::string path)
    {
        T* result = nullptr;
        Find(path, result);
        return result;
    }

private:
    virtual bool onLoad(T &entry, std::string path, int flags) = 0;

    std::vector<T> entries;
    std::unordered_map<std::string, size_t> index_by_path;
};