//
// Created by Andrzej on 29.05.2023.
//

#ifndef SIKRADIO_GUI_HANDLER_H
#define SIKRADIO_GUI_HANDLER_H

#include <tuple>
#include <vector>
#include <atomic>
#include <set>
#include <map>

struct TupleComparator {
    bool operator()(const std::tuple<std::string, int, std::string>& lhs,
            const std::tuple<std::string, int, std::string>& rhs) const {
        return std::get<2>(lhs) < std::get<2>(rhs);
    }
};

class GuiHandler{
private:
    using key = std::tuple<std::string, u16, std::string>;
    using value = std::time_t;
    std::mutex mut;
    std::map<key, value, TupleComparator> stations;
    std::tuple<std::string, u16, std::string> active_station{};
    std::atomic<bool> is_active{false};

    void go_up_or_down(int number) {
        if (!is_active)
            return;

        auto it = stations.find(active_station);
        auto act_index = std::distance(stations.begin(), it);
        auto new_index = (act_index + number) % stations.size();
        auto diff = new_index - act_index;
        std::advance(it, diff);

        active_station = (it)->first;
    }

public:
    GuiHandler() = default;

    void add_station(const std::tuple<std::string, u16, std::string>& station, std::time_t time_added) {
        std::unique_lock<std::mutex> lock(mut);

        if (!stations.contains(station))
            stations[station] = time_added;
    }

    auto get_active() {
        std::unique_lock<std::mutex> lock(mut);

        return active_station;
    }

    void go_up() {
        std::unique_lock<std::mutex> lock(mut);

        go_up_or_down(1);
    }

    void go_down() {
        std::unique_lock<std::mutex> lock(mut);

        go_up_or_down(-1);
    }
};

#endif //SIKRADIO_GUI_HANDLER_H
