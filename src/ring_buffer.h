#pragma once
#include <array>

template<typename T, size_t BufferSize>
struct RingBuffer : protected std::array<T, BufferSize> {
    //RingBuffer(size_t size) {
    //    std::vector<T>::resize(size);
    //}

    T &Alloc()
    {
        size_t size = std::array<T, BufferSize>::size();
        size_t idx = (first + count) % size;
        T* ptr = std::array<T, BufferSize>::data() + idx;
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
        // NOTE: Destructors will not be called when clearing a RingBuffer!
        //std::array<T, BufferSize>::fill({});
        first = 0;
        count = 0;
    }

    size_t Count() const
    {
        return count;
    }

    bool Empty() const
    {
        return count == 0;
    }

    //void Resize(size_t size) {
    //    std::vector<T>::resize(size);
    //}

    size_t Size() const
    {
        return std::array<T, BufferSize>::size();
    }

    T &At(size_t index)
    {
        DLB_ASSERT(index < count);
        size_t at = (first + index) % std::array<T, BufferSize>::size();
        return std::array<T, BufferSize>::at(at);
    }

    T &Last()
    {
        DLB_ASSERT(count);
        size_t at = (first + count - 1) % std::array<T, BufferSize>::size();
        return std::array<T, BufferSize>::at(at);
    }

    const T &At(size_t index) const
    {
        DLB_ASSERT(index < count);
        size_t at = (first + index) % std::array<T, BufferSize>::size();
        return std::array<T, BufferSize>::at(at);
    }

    const T &Last() const
    {
        DLB_ASSERT(count);
        size_t at = (first + count - 1) % std::array<T, BufferSize>::size();
        return std::array<T, BufferSize>::at(at);
    }

protected:
    size_t first{ 0 };  // index of first element
    size_t count{ 0 };  // current # of elements
};
