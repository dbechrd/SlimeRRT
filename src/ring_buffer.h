#pragma once
#include <vector>
#include <cassert>

template<typename T>
struct RingBuffer : protected std::vector<T> {
    RingBuffer(size_t size) {
        std::vector<T>::resize(size);
    }

    T &Alloc()
    {
        if (empty()) {
            resize(CHAT_MESSAGE_HISTORY);
        }
        size_t size = std::vector<T>::size();
        size_t idx = (first + count) % size;
        T* ptr = std::vector<T>::data() + idx;
        T &elem = *new(ptr) T{};
        if (count < size) {
            count++;
        } else {
            first = (first + 1) % size;
        }
        return elem;
    }

    void Clear()
    {
        std::vector<T>::clear();
        first = 0;
        count = 0;
    }

    size_t Count()
    {
        return count;
    }

    bool Empty()
    {
        return count == 0;
    }

    //void Resize(size_t size) {
    //    std::vector<T>::resize(size);
    //}

    size_t Size()
    {
        return std::vector<T>::size();
    }

    T &At(size_t index)
    {
        assert(index < count);
        size_t at = (first + index) % std::vector<T>::size();
        return std::vector<T>::at(at);
    }

protected:
    size_t first{ 0 };  // index of first element
    size_t count{ 0 };  // current # of elements
}; 