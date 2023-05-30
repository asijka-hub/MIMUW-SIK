//
// Created by Andrzej on 22.05.2023.
//

#ifndef SIKRADIO_CONNECTION_H
#define SIKRADIO_CONNECTION_H

//#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <cstdint>
#include <arpa/inet.h>
#include <csignal>


struct sockaddr_in get_address(const char *host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result;
    if (getaddrinfo(host, NULL, &hints, &address_result) != 0)
        throw std::runtime_error("get address failed\n");

    struct sockaddr_in address;
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr =
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr; // IP address
    address.sin_port = htons(port);

    freeaddrinfo(address_result);

    return address;
}

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
        errno = 0;
        auto address_length = (socklen_t) sizeof(sender_addr);
        int flags = 0;
        ssize_t err = sendto(socket_fd, msg, length, flags,
                                     (struct sockaddr *) &sender_addr, address_length);

        if (err < 0 && (errno == ENETDOWN || errno == ENETUNREACH)) {
            return;
        } else if (err < 0) {
            throw std::runtime_error("send reply failed");
        }
    }

    void multicast_message(char* msg, size_t n) {
        errno = 0;
        auto err = sendto(socket_fd, msg, n, 0, reinterpret_cast<struct sockaddr*>(&multicast_addr),
                          sizeof(multicast_addr));

        if (err < 0 && (errno == ENETDOWN || errno == ENETUNREACH)) {
            return;
        } else if (err < 0) {
            throw std::runtime_error("Failed to multicast message");
        }
    }

    i64 recv_message(std::vector<char>& buffer) {
        errno = 0;

        auto address_length = (socklen_t) sizeof(sender_addr);
        int flags = 0; // we do not request anything special


        // TODO buffer size na datagram size
        ssize_t len = recvfrom(socket_fd, buffer.data(), buffer.capacity(), flags,
                               (struct sockaddr *) &sender_addr, &address_length);
        cout << "read len:" << len << "\n";

        if (len < 0 && (errno == ENETDOWN || errno == ENETUNREACH)) {
            return -1;
        } else if (len < 0) {
            throw std::runtime_error("send reply failed");
        }

        return len;
    }
};

#endif //SIKRADIO_CONNECTION_H
