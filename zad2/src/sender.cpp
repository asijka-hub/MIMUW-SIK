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
#include <optional>
#include <charconv>
#include "../include/common.h"
#include "../include/parsing_helper.h"
#include "../include/concurrent_structs.h"
#include "../include/connection.h"

namespace {
    using namespace std;
    using std::optional;

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

    bool got_lookup(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return false;

        if (strcmp(buffer.data(), "ZERO_SEVEN_COME_IN") == 0) {
            cout << "we got lookup\n";
            return true;
        }
        return false;
    }

    bool parseToUInt64(const std::string& str, uint64_t& value) {
        auto result = std::from_chars(str.data(), str.data() + str.size(), value);
        return result.ec == std::errc{} && result.ptr == str.data() + str.size();
    }


    // TODO moze dodatkowy scisly  check na znaki asci od 32 do 127
    optional<vector<u64>> get_ints_from_rexmit(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return {};

        std::string input{buffer.data()};

        cout << "received input: " << input << endl;

        std::regex pattern(R"(LOUDER_PLEASE \[((?:\d+(?:,)?)+)\])");
        std::smatch matches;

        if (std::regex_search(input, matches, pattern)) {
            cout << "message ok\n";
            std::string integersStr = matches[1].str();
            std::vector<u64> integers;

            std::istringstream iss(integersStr);
            std::string token;
            while (std::getline(iss, token, ',')) {
                uint64_t value;
                if (parseToUInt64(token, value)) {
                    integers.push_back(value);
                } else {
                    cout << "message look like rexmit but int parsing failed\n";
                    return {};
                }
            }

            return integers;
        } else {
            std::cout << "Invalid input format." << std::endl;
            return {};
        }
    }

    void listener_thread(atomic<bool>& active, shared_ptr<ConcurrentQueue<char>> queue,
                         shared_ptr<ConcurrentSet> set, struct SenderArgs args) {
        cout << "hello from thread\n";

        UdpSocket listening_socket{args.mcast_addr, args.ctrl_port};

        cout << "mcast addr: \n" << args.mcast_addr.combined.c_str() << endl;
        cout << "ctrl port: \n" << args.ctrl_port << endl;

        vector<char> buffer(1000);
        size_t read_len{};

        while (active) {
            buffer.clear();
            read_len = listening_socket.recv_message(buffer);
            if (got_lookup(read_len, buffer)) {
                printf("received: %s \n", buffer.data());
            }

            auto ints = get_ints_from_rexmit(read_len, buffer);

            if (ints.has_value()) {
                printf("received: %s \n", buffer.data());

                for (auto e : ints.value())
                    cout << e << " ";
                cout << endl;
            }
        }
    }

    void repeater_thread(atomic<bool>& active, shared_ptr<ConcurrentQueue<char>> queue,
                         shared_ptr<ConcurrentSet> set, struct SenderArgs args) {
        cout << "hello from REPEATER thread\n";

        std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));

        while (active) {
            // retransmite


            std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));
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
        shared_ptr<ConcurrentSet> _first_byte_num_set;

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

            _retransmission_queue = make_shared<ConcurrentQueue<char>>(args.fsize);
            _first_byte_num_set = make_shared<ConcurrentSet>();
            // starting listener thread

            thread listener{listener_thread,
                            std::ref(_active), _retransmission_queue, _first_byte_num_set, ref(args)};
            listener.detach();

            // starting repeater thread
            thread repeater{repeater_thread, std::ref(_active), _retransmission_queue, _first_byte_num_set, ref(args)};
            repeater.detach();
        }

        void read_and_send() {
            vector<char> package(_psize + 2 * 8); // extra space for (u64) session_id and (u64) first_byte_enum

            u64 session_id = __builtin_bswap64(_session_id); // host order to network order swap
            memcpy(package.data(), &session_id, 8);

            size_t n;
            u64 first_byte_enum{};
            while ((n = std::fread(package.data() + 2 * 8, 1, _psize, stdin))) {
                first_byte_enum = get_current_first_byte_enum();
                memcpy(package.data() + 8, &first_byte_enum, 8);

                _socket.send_message(package.data(), _psize);

                std::fill(package.begin() +  8, package.end(), 0);
            }

            if (n == _psize)
                _socket.send_message(package.data(), _psize);

            // if readed data is less than psize we drop the last packet

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