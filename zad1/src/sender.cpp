#include <boost/program_options.hpp>
#include <iostream>
#include <cstdint>
#include <regex>

using namespace std;

namespace po = boost::program_options;

namespace {
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    const u16 DEFAULT_DATA_PORT = 20000 + 429592 % 10000;
    const u64 DEFAULT_PSIZE = 512;
    const std::string DEFAULT_NAME = "Nienazwany Nadajnik";

    struct address {
        std::string combined;
        std::string ip;
    };

    struct Args {
        struct address dest_addr;
        u16 data_port{};
        u64 psize{};
        std::string name;
    };

    void parse_address(struct address *addr) {
        static std::regex addr_regex("^(.*)$");
        std::smatch matches;
        std::regex_search(addr->combined, matches, addr_regex);

        addr->ip = matches[1];
    }

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

        parse_address(&program_args.dest_addr);

        return program_args;
    }

    class UdpSocket {

    };

    class Sender {
    private:
        UdpSocket
    public:
        Sender(struct Args &args) {

        }

        void read_and_send() {

        }
    };
}

int main(int argc, char **argv) {
    struct Args program_args = parse_args(argc, argv);

    Sender sender{program_args};
    sender.read_and_send();

    return 0;
}