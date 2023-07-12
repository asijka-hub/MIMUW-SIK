//
// Created by andrzej on 5/22/23.
//

#ifndef SIKRADIO_CONCURRENT_STRUCTS_H
#define SIKRADIO_CONCURRENT_STRUCTS_H

#include <cstdlib>
#include <vector>
#include <mutex>
#include <exception>

class ConcurrentQueue {
    using u64 = std::uint64_t;
private:
    std::vector<char> queue;
    std::mutex mut;
    u64 fsize;
    u64 psize;
    u64 last; // last element
    u64 number_of_last;
    u64 size;
public:
    explicit ConcurrentQueue(u64 _fsize, u64 _psize) : fsize(_fsize), psize(_psize) {
        last = 0;
        number_of_last = 0;
        queue.resize(_fsize, 0);
    }

    void push(char* buffer) {
        std::unique_lock<std::mutex> lock(mut);

        memccpy(queue.data() + last, buffer, 1, psize); //TODO

        last = (last + psize) % fsize;
        number_of_last += psize;
    }

    std::vector<char> get(u64 which) {
        std::unique_lock<std::mutex> lock(mut);

        auto how_many_back = (number_of_last - which);
        auto from_where = (last - how_many_back) % fsize;

        std::vector<char> res{queue.begin() + from_where, queue.begin() + from_where + psize};

        return res;
    }
};

class ConcurrentSet {
private:
    std::set<u64> _data;
    std::mutex _mut;
public:
    ConcurrentSet() = default;

    void add(u64 number) {
        std::unique_lock<std::mutex> lock(_mut);

        _data.insert(number);
    }

    std::vector<u64> get_all_reset_set() {
        std::unique_lock<std::mutex> lock(_mut);

        std::vector<u64> res{_data.begin(), _data.end()};

        _data.clear();

        return res;
    }

};

#endif //SIKRADIO_CONCURRENT_STRUCTS_H
