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

namespace {
    using namespace std;

    class UdpSocket {
    private:
        int _socket_fd{};
        struct sockaddr_in _receiver_addr{};

        void create_socket(const struct address& addr, u16 port) {
            _socket_fd = open_socket();

            _receiver_addr = get_address(addr.combined.c_str(), port);

            connect_socket(_socket_fd, &_receiver_addr);
        }

    public:
        UdpSocket() = delete;

        UdpSocket(const struct address& addr, u16 port) {
            create_socket(addr, port);
        }

        ~UdpSocket() {
            close(_socket_fd);
        }

        void send_message(const char* msg, size_t length) const {
            errno = 0;
            ssize_t sent_length = send(_socket_fd, msg, length, 0);
            if (sent_length < 0) {
                PRINT_ERRNO();
            }
            ENSURE(sent_length == (ssize_t) length);
        }
    };

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