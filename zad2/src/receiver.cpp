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
        cout << "LOOKUP thread started\n";

        string message{"ZERO_SEVEN_COME_IN\n"};

        while(active) {
            socket.multicast_message(message.data(), message.length());
            cout << "lookup thread: SEND MESSAGE\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    optional<tuple<string, u16, string>> get_reply(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return {};

        std::string input{buffer.data()};

        cout << "received input: " << input << endl;

        std::regex regex(R"(BOREWICZ_HERE ((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})) (\d{1,5}) (.+))");
        std::smatch match;

        if (std::regex_match(input, match, regex)) {
            std::string ip_address = match[1];

            uint16_t port;
            if (!parse_to_uint_T<u16>(match[6], port))
                return {};

            std::string name = match[7];

            std::cout << "IP Address: " << ip_address << std::endl;
            std::cout << "Port: " << port << std::endl;
            std::cout << "Message: " << name << std::endl;

            auto tuple = make_tuple(ip_address, port, name);

            return tuple;

        } else {
            std::cout << "Invalid input format." << std::endl;
            return {};
        }
    }

    void reply_thread(atomic<bool>& active, UdpSocket& socket, GuiHandler& guiHandler) {
        cout << "REPLY thread started\n";

        socket.bind_to_random();

        vector<char> buffer(1000);
        size_t read_len{};

        while(active) {
            buffer.clear();
            read_len = socket.recv_message(buffer);

            auto reply = get_reply(read_len, buffer);

            if (reply.has_value()) {
                cout << "GOT REPLY\n";
                continue;
            }

        }
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

            thread lookup{lookup_thread, std::ref(active), std::ref(lookup_reply_socket)};
            lookup.detach();

            thread reply{reply_thread, std::ref(active), std::ref(lookup_reply_socket), std::ref(guiHandler)};
            reply.detach();
        }

        void start() {
            cout<<"started receiver\n";
            while(1) {};
        }

    };
}

int main(int argc, char **argv) {
    struct ReceiverArgs program_args = parse_receiver_args(argc, argv);
    try {
        Receiver receiver{program_args};
        receiver.start();
    } catch (const std::exception& ex) {
        // Handle the exception thrown from the thread in the main thread
        std::cout << "Exception caught in main thread: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}