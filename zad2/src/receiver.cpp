// author Andrzej Sijka as429592
// MIMUW 2023

#include <boost/program_options.hpp>
#include <iostream>
#include <cstdint>
#include <regex>
#include <vector>
#include <array>
#include <cstdio>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <queue>
#include <thread>
#include <optional>
#include <mutex>
#include "../include/common.h"
#include "../include/parsing_helper.h"
#include "../include/connection.h"
#include "../include/gui_handler.h"
#include "../include/utils.h"

namespace {
    using namespace std;

    void lookup_thread(atomic<bool>& active, UdpSocket& socket) {
        string message{"ZERO_SEVEN_COME_IN\n"};

        while(active) {
            socket.multicast_message(message.data(), message.length());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    optional<tuple<string, u16, string>> get_reply(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return {};

        std::string input{buffer.data()};

        std::regex regex(R"(BOREWICZ_HERE \[((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))\] \[(\d{1,5})\] \[(.+)\])");
        std::smatch match;

        if (std::regex_match(input, match, regex)) {
            std::string ip_address = match[1];

            uint16_t port;
            if (!parse_to_uint_T<u16>(match[6], port))
                return {};

            std::string name = match[7];

            auto tuple = make_tuple(ip_address, port, name);
            return tuple;
        } else {
            return {};
        }
    }

    void reply_thread(atomic<bool>& active, UdpSocket& socket, GuiHandler& guiHandler) {
        vector<char> buffer(1000);

        while(active) {
            buffer.clear();
            auto read_len = socket.recv_message(buffer);

            if (read_len < 0)
                continue;

            auto reply = get_reply(read_len, buffer);
            auto rec_time = chrono::system_clock::to_time_t(chrono::system_clock::now());

            if (reply.has_value()) {
                guiHandler.add_station(reply.value(), rec_time);
                continue;
            }

        }
    }

    void ui_thread([[maybe_unused]] atomic<bool>& active,[[maybe_unused]] GuiHandler& guiHandler) {
    }

    class Receiver {
    private:
        std::time_t session_id{};
        u64 bsize{};
        u64 rtime{};
        u64 first_byte_enum{};
        atomic<bool> active{true};
        UdpSocket lookup_reply_socket;
        GuiHandler guiHandler;

        void finish() {
            active = false;
        }

    public:
        Receiver() = delete;

        explicit Receiver(struct ReceiverArgs &args) : bsize(args.bsize)
                , lookup_reply_socket(args.dsc_addr, args.ctrl_port) {
            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            lookup_reply_socket.bind_to_random();

            thread lookup{lookup_thread, std::ref(active), std::ref(lookup_reply_socket)};
            lookup.detach();

            thread reply{reply_thread, std::ref(active), std::ref(lookup_reply_socket), std::ref(guiHandler)};
            reply.detach();

            thread ui{ui_thread, std::ref(active), std::ref(guiHandler)};
            ui.detach();
        }

        void start() {
            cout<<"started receiver\n";
            while(1) {};
        }

    };
}

int main(int argc, char **argv) {
    try {
        struct ReceiverArgs program_args = parse_receiver_args(argc, argv);
        Receiver receiver{program_args};
        receiver.start();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}