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

    class Receiver {
    private:
        UdpSocket socket_fd;
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

        explicit Receiver(struct ReceiverArgs &args) :
                socket_fd(args.dsc_addr, args.ctrl_port), bsize(args.bsize) {
            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

        }

        void start() {
            cout<<"started receiver\n";
        }

    };
}

int main(int argc, char **argv) {
    struct ReceiverArgs program_args = parse_receiver_args(argc, argv);
    try {
        Receiver receiver{program_args};
        receiver.start();
    } catch (const std::exception& e) {
        cerr << "exception occurred\n";
        return 1;
    }
    return 0;
}