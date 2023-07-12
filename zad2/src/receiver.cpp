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
#include <optional>
#include <mutex>
//#include "../include/common.h"
#include "../include/parsing_helper.h"
#include "../include/connection.h"
#include "../include/gui_handler.h"
#include "../include/utils.h"

namespace {
    using namespace std;

    void lookup_thread(atomic<bool>& active, UdpSocket& socket) {
        string message{"ZERO_SEVEN_COME_IN\n"};

        while(active) {
            socket.send_message(message.data(), message.length());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    optional<tuple<string, u16, string>> get_reply(size_t read_len, vector<char>& buffer) {
        if (read_len <= 0)
            return {};

        std::string input{buffer.data()};

        std::regex regex(R"(BOREWICZ_HERE ((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})) (\d{1,5}) (.+)\n)");
        std::smatch match;

        if (std::regex_match(input, match, regex)) {
            std::string ip_address = match[1];

            uint16_t port;
            if (!parse_to_uint_T<u16>(match[6], port))
                return {};

            std::string name = match[7];

            auto tuple = make_tuple(ip_address, port, name);
            return tuple;
        } else {
            return {};
        }
    }

    void reply_thread(atomic<bool>& active, UdpSocket& socket, GuiHandler& guiHandler) {
        vector<char> buffer(1000);

        while(active) {
            buffer.clear();
            auto read_len = socket.recv_message(buffer);

            if (read_len < 0)
                continue;

            auto reply = get_reply(read_len, buffer);
            auto rec_time = chrono::system_clock::to_time_t(chrono::system_clock::now());

            if (reply.has_value()) {
                cout << "received BOREWICZ\n";

                guiHandler.add_station(reply.value(), rec_time);
                continue;
            }

        }
    }

    char gui_mess[] = "------------------------------------------------------------------------\n"
                      "\n"
                      " SIK Radio\n"
                      "\n"
                      "------------------------------------------------------------------------\n"
                      "\n"
                      "PR1\n"
                      "\n"
                      "Radio \"357\"\n"
                      "\n"
                      " > Radio \"Disco Pruszkow\"\n"
                      "\n"
                      "------------------------------------------------------------------------\n";

    char setting_right_mode[] = {static_cast<char>(255), static_cast<char>(253), 34};

    void ui_thread([[maybe_unused]] atomic<bool>& active,[[maybe_unused]] GuiHandler& guiHandler, u16 ui_port) {
        int socket_fd = open_tcp_socket();
        bind_socket_port(socket_fd, ui_port);

        int queue_length = 5;

        int err = listen(socket_fd, queue_length);

        if (err != 0) {
            cout << "err: " << err << endl;
            throw std::runtime_error("listening failed\n");
        }

        const int buffer_size = 1000000;
        char buffer[buffer_size];

        bool f = false;

        for (;;) {
            memset(buffer, 0, buffer_size);
            struct sockaddr_in client_address;
            int client_fd = accept_connection(socket_fd, &client_address);
            char *client_ip = inet_ntoa(client_address.sin_addr);
            // We don't need to free this,
            // it is a pointer to a static buffer.

            uint16_t client_port = ntohs(client_address.sin_port);
            printf("Accepted connection from %s:%d\n", client_ip, client_port);

            // Reading needs to be done in a loop, because:
            // 1. the client may send a message that is larger than the buffer
            // 2. a single read() call may not read the entire message, even if it fits in the buffer
            // 3. in general, there is no rule that for each client's write(), there will be a corresponding read()
            size_t read_length;
            do {
                if (!f) {
                    send_message(client_fd, setting_right_mode, sizeof(setting_right_mode), 0);
                    f = true;
                }

                int flags = 0;
                read_length = receive_message(client_fd, buffer, buffer_size, flags);
                if (read_length > 0) {
                    printf("Received %zd bytes from client %s:%u: \n", read_length, client_ip, client_port,
                           (int) read_length); // note: we specify the length of the printed string

                  send_message(client_fd, gui_mess, sizeof(gui_mess), flags);
//                printf("Sent %zd bytes to client %s:%u\n", read_length, client_ip, client_port);
                }
            } while (read_length > 0);

            printf("Closing connection\n");
            if (close(client_fd) != 0)

                throw std::runtime_error("closing connection failed\n");
        }
    }

    class Receiver {
    private:
        std::time_t session_id{};
        u64 bsize{};
        u64 rtime{};
        u64 first_byte_enum{};
        atomic<bool> active{true};
        UdpSocket lookup_reply_socket;
        GuiHandler guiHandler;

        void finish() {
            active = false;
        }

    public:
        Receiver() = delete;

        explicit Receiver(struct ReceiverArgs &args) : bsize(args.bsize)
                , lookup_reply_socket() {
            session_id = chrono::system_clock::to_time_t(chrono::system_clock::now());

            lookup_reply_socket.bind_to_random();
            lookup_reply_socket.set_broadcast();
            lookup_reply_socket.set_destination_addr(args.dsc_addr, args.ctrl_port);

            thread lookup{lookup_thread, std::ref(active), std::ref(lookup_reply_socket)};
            lookup.detach();

            thread reply{reply_thread, std::ref(active), std::ref(lookup_reply_socket), std::ref(guiHandler)};
            reply.detach();

            thread ui{ui_thread, std::ref(active), std::ref(guiHandler), args.ui_port};
            ui.detach();
        }

        void start() {
            cout<<"started RECEIVER\n";
            while(1) {};
        }

    };
}

int main(int argc, char **argv) {
    try {
        struct ReceiverArgs program_args = parse_receiver_args(argc, argv);
        Receiver receiver{program_args};
        receiver.start();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}