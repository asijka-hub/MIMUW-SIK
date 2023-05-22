//
// Created by andrzej on 5/22/23.
//

#ifndef SIKRADIO_CONCURRENT_QUEUE_H
#define SIKRADIO_CONCURRENT_QUEUE_H

#include <cstdlib>
#include <vector>
#include <mutex>
#include <exception>

template<class T>
class ConcurrentQueue {
    using u64 = std::uint64_t;
private:
    std::vector<T> _queue;
    std::mutex _mut;
    u64 _fsize;
public:
    explicit ConcurrentQueue(u64 fsize) : _fsize(fsize) {
    }

    void push(const std::vector<T>& e) {
        std::unique_lock<std::mutex> lock(_mut);

        u64 next_size = _queue.size() + e.size();

        if (next_size < _fsize) {

            _queue.insert(_queue.end(), e.begin(), e.end());
            return;
        }

        u64 elems_to_remove = next_size - _fsize;

        _queue.erase(_queue.begin(), _queue.begin() + elems_to_remove); //TODO check
        _queue.insert(_queue.end(), e.begin(), e.end());
    }

    [[nodiscard]] std::vector<T> pop(u64 count) {
        std::unique_lock<std::mutex> lock(_mut);

        if (_queue.size() < count)
            std::cout<<"ERROR FROM QUEUE\n";

        std::vector<T> data(_queue.begin(), _queue.begin() + count);
        _queue.erase(_queue.begin(), _queue.begin() + count);
        return data;

    }

    [[nodiscard]] T pop() {
        std::unique_lock<std::mutex> lock(_mut);

        auto res = _queue[_queue.size() - 1];
        _queue.pop_back();

        return res;
    }

    [[nodiscard]] u64 how_much_to_retransmit() {
        std::unique_lock<std::mutex> lock(_mut);
        return _queue.size();
    }

};

#endif //SIKRADIO_CONCURRENT_QUEUE_H
