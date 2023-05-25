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

    //TODO ogarnac te end of line characters
    bool got_lookup(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return false;

        if (strcmp(buffer.data(), "ZERO_SEVEN_COME_IN\n") == 0) {
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
    // TODO warunki maja ostatecznie luznie
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

    void listener_thread(atomic<bool>& active,
                         shared_ptr<ConcurrentSet> set, struct SenderArgs& args) {
        cout << "LISTENER thread started\n";

        UdpSocket listening_socket{args.mcast_addr, args.ctrl_port};
        listening_socket.bind_socket();

        vector<char> buffer(1000);
        size_t read_len{};

        while (active) {
            buffer.clear();
            read_len = listening_socket.recv_message(buffer);
            printf("got something: %s\n", buffer.data());
            if (got_lookup(read_len, buffer)) {
                cout << "GOT LOOK UP\n";

                std::ostringstream oss;
                string mcast_addr{args.mcast_addr.combined};
                oss << "BOREWICZ_HERE [" << mcast_addr << "] [" << args.data_port << "] [" << args.name << "]";
                std::string formatted_str = oss.str();

                std::vector<char> char_vector(formatted_str.begin(), formatted_str.end());

                cout << "message: " << formatted_str << endl;

                listening_socket.send_reply(formatted_str.c_str(), formatted_str.length());

                continue;
            }

            auto ints = get_ints_from_rexmit(read_len, buffer);

            if (ints.has_value()) {
                cout << "REXMIT\n";

                for (auto e : ints.value())
                    set->add(e);
            }
        }
    }

    void repeater_thread(atomic<bool>& active, shared_ptr<ConcurrentQueue<char>> queue,
                         shared_ptr<ConcurrentSet> set, struct SenderArgs args) {
        cout << "REPEATER thread started\n";

        std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));

        while (active) {
            // TODO
            auto packet_numbers = set->get_all_reset_set();




            std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));
        }
    }

    class Sender {
    private:
        UdpSocket socket_fd;
        std::time_t session_id{};
        u64 psize{};
        u64 first_byte_enum{};
        atomic<bool> active{true};
        shared_ptr<ConcurrentQueue<char>> retransmission_queue;
        shared_ptr<ConcurrentSet> first_byte_num_set;

        u64 get_current_first_byte_enum() {
            u64 res = first_byte_enum;
            res = htole64(res);
            first_byte_enum += psize;

            return res;
        }

        void finish() {
            active = false;
        }

        // TODO zmienic moze na char[] i bez dodatkowej funkcji
        vector<char> get_message(vector<char>& buffer) {
            u64 id = htobe64(session_id);
            u64 first_byte = get_current_first_byte_enum();

            vector<char> res(psize + 2 * 8);

            memcpy(res.data(), &id, 8);
            memcpy(res.data() + 8, &first_byte, 8);
            memcpy(res.data() + 16, buffer.data(), psize);

            return res;
        }

    public:
        Sender() = delete;

        explicit Sender(struct SenderArgs &args) :
                socket_fd(args.mcast_addr, args.data_port), psize(args.psize) {
            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            retransmission_queue = make_shared<ConcurrentQueue<char>>(args.fsize);
            first_byte_num_set = make_shared<ConcurrentSet>();

            // starting listener thread
            thread listener{listener_thread,
                            std::ref(active), first_byte_num_set, ref(args)};
            listener.detach();

            // starting repeater thread
            thread repeater{repeater_thread, std::ref(active), retransmission_queue, first_byte_num_set, ref(args)};
            repeater.detach();
        }

        void read_and_send() {
            vector<char> buffer(psize);

            u64 n;
            while ((n = std::fread(buffer.data() , 1, psize, stdin))) {
                auto message = get_message(buffer);
                socket_fd.multicast_message(buffer.data(), buffer.size());
                cout << "i\n";
            }

            cout << "n: " << n << "\n";

            // TODO sprawdzic czy fread tak dziala
            if (n == psize) {
                cout << "fdsafsafddasfs\n";
                auto message = get_message(buffer);
                socket_fd.multicast_message(buffer.data(), buffer.size());

            }

            // if readed data is less than psize we drop the last packet
            finish();
        }

    };
}

int main(int argc, char **argv) {
    struct SenderArgs program_args = parse_sender_args(argc, argv);
    try {
        Sender sender{program_args};
        sender.read_and_send();
    } catch (const std::exception& e) {
        cerr << "exception occurred\n";
        return 1;
    }
    return 0;
}