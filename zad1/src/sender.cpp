#include <boost/program_options.hpp>
#include <iostream>
#include <cstdint>
#include <regex>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
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

    const u16 DEFAULT_DATA_PORT = 20000 + 429592 % 10000;
    const u64 DEFAULT_PSIZE = 512;
    const std::string DEFAULT_NAME = "Nienazwany Nadajnik";

    struct address {
        std::string combined;
//        std::string ip;
    };

    struct Args {
        struct address dest_addr;
        u16 data_port{};
        u64 psize{};
        std::string name;
    };

//    void parse_address(struct address *addr) {
//        static std::regex addr_regex("^(.*)$");
//        std::smatch matches;
//        std::regex_search(addr->combined, matches, addr_regex);
//
//        addr->ip = matches[1];
//    }

    struct Args parse_args(int argc, char **argv) {
        struct Args program_args;
        po::options_description desc("Sender options");

        desc.add_options()
                ("help,h", "produce help message")
                ("dest_addr,a", po::value<std::string>(&program_args.dest_addr.combined)->required(), "sender address")
                ("data_port,P", po::value<u16>(&program_args.data_port)->default_value(DEFAULT_DATA_PORT),
                 "port used for data transfer")
                ("psize,p", po::value<u64>(&program_args.psize)->default_value(DEFAULT_PSIZE), "size of audio_data")
                ("nazwa,n", po::value<std::string>(&program_args.name)->default_value(DEFAULT_NAME), "server address");

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

//        parse_address(&program_args.dest_addr);

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

        void send_message(const char* msg, size_t length) {
            errno = 0;
            ssize_t sent_length = send(_socket_fd, msg, length, 0);
            if (sent_length < 0) {
                PRINT_ERRNO();
            }
            ENSURE(sent_length == (ssize_t) length);
        }
    };

    class Sender {
    private:
        UdpSocket _socket;
        std::time_t _curr_time{};
    public:
        Sender() = delete;

        explicit Sender(struct Args &args) : _socket(args.dest_addr, args.data_port) {
            _curr_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        }

        void read_and_send() {
            cout<<"wysylanie wiadomosci\n";
            _socket.send_message("sraka",5);
        }
    };
}

int main(int argc, char **argv) {
    struct Args program_args = parse_args(argc, argv);

    Sender sender{program_args};
    sender.read_and_send();


    return 0;
}