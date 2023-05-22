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
#include "../include/concurrent_queue.h"

namespace {
    using namespace std;

    class Exception : public std::exception
    {
        std::string _msg;
    public:
        explicit Exception(std::string  msg) : _msg(std::move(msg)){}

        [[nodiscard]] const char* what() const noexcept override
        {
            return _msg.c_str();
        }
    };

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

    void listener_thread(atomic<bool>& active, shared_ptr<ConcurrentQueue<char>> queue, u16 ctrl_port, const struct address& addr) {
        cout << "hello from thread\n";

        UdpSocket listening_socket{addr, ctrl_port};

        while (active) {

        }
    }

    class Sender {
    private:
        UdpSocket _socket;
        std::time_t _session_id{};
        u64 _psize{};
        u64 _rtime{};
        u64 _first_byte_enum{};
        atomic<bool> _active{true};
        shared_ptr<ConcurrentQueue<char>> _retransmission_queue;

        u64 get_current_first_byte_enum() {
            u64 res = _first_byte_enum;
            res = __builtin_bswap64(res);
            _first_byte_enum += _psize;

            return res;
        }

        void finish() {
            _active = false;
        }

    public:
        Sender() = delete;

        explicit Sender(struct SenderArgs &args) :
                _socket(args.mcast_addr, args.data_port), _psize(args.psize), _rtime(args.rtime) {
            _session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            // starting listener thread
            _retransmission_queue = make_shared<ConcurrentQueue<char>>(args.fsize);
            thread listener{listener_thread,
                            std::ref(_active), _retransmission_queue, args.ctrl_port, args.mcast_addr   };
            listener.detach();
        }

//        void read_and_send() {
//            vector<char> package(_psize + 2 * 8); // extra space for (u64) session_id and (u64) first_byte_enum
//
//            u64 session_id = __builtin_bswap64(_session_id); // host order to network order swap
//            memcpy(package.data(), &session_id, 8);
//
//            size_t n;
//            u64 first_byte_enum{};
//            while ((n = std::fread(package.data() + 2 * 8, 1, _psize, stdin))) {
//                first_byte_enum = get_current_first_byte_enum();
//                memcpy(package.data() + 8, &first_byte_enum, 8);
//
//                _socket.send_message(package.data(), _psize);
//
//                std::fill(package.begin() +  8, package.end(), 0);
//            }
//
//            if (n == _psize)
//                _socket.send_message(package.data(), _psize);
//
//            // if readed data is less than psize we drop the last packet
//
//            finish();
//        }

        void read_and_send() {
            vector<char> package{};

            size_t n;
            size_t first_byte_enum;

            auto start = std::chrono::high_resolution_clock ::now();

            while ((n = std::fread(package.data() + 2 * 8, 1, _psize, stdin))) {
                auto curr = std::chrono::high_resolution_clock ::now();

                if (std::chrono::duration_cast<std::chrono::milliseconds>(curr - start).count() > _rtime) {
                    start = curr;
                    //  retransmitujemy
                    auto count = _retransmission_queue->how_much_to_retransmit();


                } else {

                    first_byte_enum = get_current_first_byte_enum();
                    memcpy(package.data() + 8, &first_byte_enum, 8);

                    _socket.send_message(package.data(), _psize);

                    std::fill(package.begin() + 8, package.end(), 0);
                }
            }

            if (n == _psize)
                _socket.send_message(package.data(), _psize);

//            // if readed data is less than psize we drop the last packet

            finish();

        }

    };
}

int main(int argc, char **argv) {
    struct SenderArgs program_args = parse_sender_args(argc, argv);

    Sender sender{program_args};
    sender.read_and_send();

    return 0;
}