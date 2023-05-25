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
#include <mutex>
#include "../include/common.h"
#include "../include/parsing_helper.h"
#include "../include/connection.h"

namespace {
    using namespace std;

    void lookup_thread(struct ReceiverArgs& args) {

            UdpSocket lookup_socket(args.dsc_addr, args.ctrl_port);

            string message{"ZERO_SEVEN_COME_IN\n"};

            for(;;) {
                lookup_socket.multicast_message(message.data(), message.length());
                cout << "lookup thread: SEND MESSAGE\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
    }

    class Receiver {
    private:
        std::time_t session_id{};
        u64 bsize{};
        u64 rtime{};
        u64 first_byte_enum{};
        atomic<bool> active{true};

        void finish() {
            active = false;
        }

    public:
        Receiver() = delete;

        explicit Receiver(struct ReceiverArgs &args) : bsize(args.bsize) {
            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            thread a{lookup_thread, std::ref(args)};
            a.detach();
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
    }
    return 0;
}