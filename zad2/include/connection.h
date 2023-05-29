//
// Created by Andrzej on 22.05.2023.
//

#ifndef SIKRADIO_CONNECTION_H
#define SIKRADIO_CONNECTION_H

#include "common.h"

class UdpSocket {
private:
    int socket_fd{};
    u16 port;
    struct sockaddr_in sender_addr{};
    struct sockaddr_in multicast_addr{};


    void create_multicast_socket(const struct address& _multi_addr, u16 _port) {
        if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            throw std::runtime_error("Failed to create socket\n");
        }

        // set multicast address
        multicast_addr = get_address(_multi_addr.combined.c_str(), _port);

        // Join the multicast group
        ip_mreq multicastRequest{};
        multicastRequest.imr_multiaddr.s_addr = inet_addr(_multi_addr.combined.c_str());
        multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0) {
            throw std::runtime_error("Failed to join multicast group\n");
        }
    }

public:
    UdpSocket() = delete;

    UdpSocket(const struct address& _multi_addr, u16 _port) : port(_port) {
        create_multicast_socket(_multi_addr, _port);
    }

    ~UdpSocket() {
        close(socket_fd);
    }

    void bind_socket() const {
        // Bind the socket to the specified port
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }
    }

    void bind_to_random() const {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = 0;

        if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind socket to random port");
        }
    }

    void send_reply(const char* msg, size_t length) const {
        auto address_length = (socklen_t) sizeof(sender_addr);
        int flags = 0;
        ssize_t sent_length = sendto(socket_fd, msg, length, flags,
                                     (struct sockaddr *) &sender_addr, address_length);

        if (sent_length != (ssize_t) length)
            throw std::runtime_error("send reply failed");
    }

    void multicast_message(char* msg, size_t n) {
        if (sendto(socket_fd, msg, n, 0, reinterpret_cast<struct sockaddr*>(&multicast_addr),
                   sizeof(multicast_addr)) < 0) {
            throw std::runtime_error("Failed to multicast message");
        }
    }

    size_t recv_message(std::vector<char>& buffer) {
        auto address_length = (socklen_t) sizeof(sender_addr);
        int flags = 0; // we do not request anything special

        ssize_t len = recvfrom(socket_fd, buffer.data(), buffer.capacity(), flags,
                               (struct sockaddr *) &sender_addr, &address_length);
        cout << "read len:" << len << "\n";

        if (len < 0) {
            throw std::runtime_error("Failed to receive message");
        }

        return (size_t) len;
    }
};

#endif //SIKRADIO_CONNECTION_H
