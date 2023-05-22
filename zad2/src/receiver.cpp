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

using namespace std;

namespace po = boost::program_options;

namespace {
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


    using std::cout;
    using std::cerr;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using i64 = std::int64_t;

    const u32 index_number = 429592;
    const u16 DEFAULT_DATA_PORT = static_cast<u16>(20000 + index_number % 10000);
    const u16 DEFAULT_CTRL_PORT = static_cast<u16>(30000 + index_number % 10000);
    const u64 DEFAULT_PSIZE = 512;
    const u64 DEFAULT_FSIZE = 128 * 1024;
    const u64 DEFAULT_RTIME = 250;
    const std::string DEFAULT_NAME = "Nienazwany Nadajnik";

    void check_positive(auto number) {
        if (number < 0)
            throw po::validation_error(po::validation_error::invalid_option_value);
    }

    void check_port(int port) {
        if (port < 0 || port > 65535)
            throw po::validation_error(po::validation_error::invalid_option_value);
    }

    struct address {
        std::string combined;
    };

    struct Args {
        struct address mcast_addr;
        u16 data_port{};
        u16 ctrl_port{};
        u64 psize{};
        u64 fsize{};
        u64 rtime{};
        std::string name;
    };

    struct Args parse_args(int argc, char **argv) {
        struct Args program_args;
        po::options_description desc("Sender options");

        int data_port, ctrl_port;
        i64 psize, fsize, rtime;

        desc.add_options()
                ("help,h", "produce help message")
                ("a,a", po::value<std::string>(&program_args.mcast_addr.combined)->required(), "broad cast address")
                ("P,P", po::value<int>(&data_port)->default_value(DEFAULT_DATA_PORT)->notifier(&check_port),
                 "port used for DATA transfer [0-65535]")
                ("C,C", po::value<int>(&ctrl_port)->default_value(DEFAULT_CTRL_PORT)->notifier(&check_port),
                 "port used for CONTROL data transfer [0-65535]")
                ("p,p", po::value<i64>(&psize)->default_value(DEFAULT_PSIZE)->notifier(&check_positive<i64>), "size of audio_data > 0")
                ("f,f", po::value<i64>(&fsize)->default_value(DEFAULT_FSIZE)->notifier(&check_positive<i64>), "size of fifo queue > 0")
                ("R,R", po::value<i64>(&rtime)->default_value(DEFAULT_RTIME)->notifier(&check_positive<i64>), "retransmission time > 0")
                ("n,n", po::value<std::string>(&program_args.name)->default_value(DEFAULT_NAME), "server address");

        try {
            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);
            if (vm.count("help")) {
                std::cout << desc << std::endl;
                exit(0);
            }

            po::notify(vm);
        }
        catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        program_args.data_port = static_cast<u16>(data_port);
        program_args.ctrl_port = static_cast<u16>(ctrl_port);
        program_args.psize = static_cast<u64>(psize);
        program_args.fsize = static_cast<u64>(fsize);
        program_args.rtime = static_cast<u64>(rtime);

        return program_args;
    }

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
        u64 _psize{};
        u64 _rtime{};
        u64 _first_byte_enum{};
        atomic<bool> _active{true};

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
        Receiver() = delete;

        explicit Receiver(struct Args &args) :
                _socket(args.mcast_addr, args.data_port), _psize(args.psize), _rtime(args.rtime) {
            _session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

        }


        void start() {
            cout<<"started receiver\n";
        }

    };
}

int main(int argc, char **argv) {
    struct Args program_args = parse_args(argc, argv);

    Receiver receiver{program_args};
    receiver.start();

    return 0;
}