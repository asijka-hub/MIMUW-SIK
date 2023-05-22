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
        UdpSocket _socket;
        std::time_t _session_id{};
        u64 _bsize{};
        u64 _rtime{};
        u64 _first_byte_enum{};
        atomic<bool> _active{true};

        void finish() {
            _active = false;
        }

    public:
        Receiver() = delete;

        explicit Receiver(struct ReceiverArgs &args) :
                _socket(args.dsc_addr, args.ctrl_port), _bsize(args.bsize) {
            _session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

        }

        void start() {
            cout<<"started receiver\n";
        }

    };
}

int main(int argc, char **argv) {
    struct ReceiverArgs program_args = parse_receiver_args(argc, argv);

    Receiver receiver{program_args};
    receiver.start();

    return 0;
}