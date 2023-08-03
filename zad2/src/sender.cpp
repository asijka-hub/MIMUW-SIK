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
//#include "../include/common.h"
#include "../include/parsing_helper.h"
#include "../include/concurrent_structs.h"
#include "../include/connection.h"
#include "../include/utils.h"


namespace {
    using namespace std;
    using std::optional;

    string borewicz_message;

    bool got_lookup(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return false;

        if (strcmp(buffer.data(), "ZERO_SEVEN_COME_IN\n") == 0) {
            return true;
        }
        return false;
    }

    optional<vector<u64>> get_numbers_from_rexmit(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return {};

        std::string input{buffer.data()};

        std::regex pattern(R"(LOUDER_PLEASE \[((?:\d+(?:,)?)+)\])");
        std::smatch matches;

        if (std::regex_search(input, matches, pattern)) {
            std::string integersStr = matches[1].str();
            std::vector<u64> integers;

            std::istringstream iss(integersStr);
            std::string token;
            while (std::getline(iss, token, ',')) {
                uint64_t value;
                if (parse_to_uint_T<u64>(token, value)) {
                    integers.push_back(value);
                } else {
                    return {};
                }
            }

            cout << "recieved correct rexmit\n";

            return integers;
        } else {
            return {};
        }
    }

    string get_borewicz_mess(struct SenderArgs& args) {
        std::ostringstream oss;
        string mcast_addr{args.mcast_addr.combined};
        oss << "BOREWICZ_HERE " << mcast_addr << " " << args.data_port << " " << args.name << "\n";
        std::string formatted_str = oss.str();

        return formatted_str;
    }

    void listener_thread(atomic<bool>& active,
                         shared_ptr<ConcurrentSet>& retransmission_set, struct SenderArgs& args) {

        UdpSocket listening_socket{};
        listening_socket.bind_socket(args.ctrl_port);
        listening_socket.set_broadcast();

        vector<char> buffer(1000);

        while (active) {
            buffer.clear();
            auto read_len = listening_socket.recv_message(buffer);

            if (read_len < 0)
                continue;

            if (got_lookup(read_len, buffer)) {
                cout << "received LOOKUP -> sending message:" << borewicz_message;

                listening_socket.send_reply(borewicz_message.c_str(), borewicz_message.length());

                continue;
            }

            auto packet_numbers = get_numbers_from_rexmit(read_len, buffer);

            if (packet_numbers.has_value()) {
                for (auto e : packet_numbers.value())
                    retransmission_set->add(e);
            }
        }
    }

    void repeater_thread(atomic<bool>& active, shared_ptr<ConcurrentQueue>& queue,
                         shared_ptr<ConcurrentSet>& set, struct SenderArgs& args, UdpSocket& socket, std::time_t session_id) {
        std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));

        vector<char> buffer(args.psize + 2 * 8);

        *((u64*)buffer.data()) = htobe64(session_id);

        while (active) {
            auto packet_numbers = set->get_all_reset_set();

            for (auto number : packet_numbers) {

                auto packet = queue->get(number);
                auto packet_number = htobe64(number);

                memcpy(buffer.data() + 8, &packet_number, 8);
                memcpy(buffer.data() + 2 * 8, packet.data(), args.psize);

                socket.send_message(packet.data(), packet.size());
            }

            std::this_thread::sleep_for(std::chrono::microseconds(args.rtime));
        }
    }

    class Sender {
    private:
        UdpSocket sending_socket;
        std::time_t session_id{};
        u64 psize{};
        u64 first_byte_enum{};
        atomic<bool> active{true};
        shared_ptr<ConcurrentQueue> retransmission_queue;
        shared_ptr<ConcurrentSet> first_byte_num_set;

        u64 get_current_first_byte_enum() {
            u64 res = first_byte_enum; //TODO
            res = htobe64(res);
            first_byte_enum += psize;

            return res;
        }

        void finish() {
            active = false;
        }

    public:
        Sender() = delete;

        explicit Sender(struct SenderArgs &args) :
                sending_socket(), psize(args.psize) {
            sending_socket.join_multicast(args.mcast_addr, args.data_port);

            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            retransmission_queue = make_shared<ConcurrentQueue>(args.fsize, args.psize);
            first_byte_num_set = make_shared<ConcurrentSet>();

            // starting listener thread
            thread listener{listener_thread,
                            std::ref(active), std::ref(first_byte_num_set), ref(args)};
            listener.detach();

            // starting repeater thread
            thread repeater{repeater_thread,
                            std::ref(active), std::ref(retransmission_queue),
                            std::ref(first_byte_num_set), ref(args), ref(sending_socket), session_id};
            repeater.detach();

            borewicz_message = get_borewicz_mess(args);
        }

        void read_and_send() {
            cout<<"started SENDER\n";

            vector<char> buffer(psize + 2 * 8);

            *((u64*)buffer.data()) = htobe64(session_id);

            while (std::fread(buffer.data() + 2 * 8 , 1, psize, stdin) == psize) {
                retransmission_queue->push(buffer.data() + 2 * 8);

                u64 first_byte = get_current_first_byte_enum();
                memcpy(buffer.data() + 8, &first_byte, 8);

                sending_socket.send_message(buffer.data(), buffer.size());
            }

            // if readed data is less than psize we drop the last packet
            finish();
        }

    };
}

int main(int argc, char **argv) {
    try {
        struct SenderArgs program_args = parse_sender_args(argc, argv);
        Sender sender{program_args};
        sender.read_and_send();
    } catch (const std::exception& e) {
        cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}