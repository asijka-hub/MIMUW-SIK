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

int open_udp_socket() {
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw std::runtime_error("Failed to create socket\n");

    return fd;
}

int open_tcp_socket() {
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("Failed to create socket\n");

    return fd;
}

void bind_socket_port(int socket_fd, uint16_t port) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *) &server_address,
                     (socklen_t) sizeof(server_address)) != 0)
        throw std::runtime_error("binding socket failed\n");
}

int accept_connection(int socket_fd, struct sockaddr_in *client_address) {
    socklen_t client_address_length = (socklen_t) sizeof(*client_address);

    int client_fd = accept(socket_fd, (struct sockaddr *) client_address, &client_address_length);
    if (client_fd < 0) {
        throw std::runtime_error("accepting connection failed\n");
    }

    return client_fd;
}

inline static size_t receive_message(int socket_fd, void *buffer, size_t max_length, int flags) {
    errno = 0;
    ssize_t received_length = recv(socket_fd, buffer, max_length, flags);
    if (received_length < 0) {
        throw std::runtime_error("receiving message failed\n");
    }
    return (size_t) received_length;
}

class UdpSocket {
private:
    int socket_fd{};
    struct sockaddr_in sender_addr{};
    struct sockaddr_in dst_addr{};

public:
    UdpSocket() {
        socket_fd = open_udp_socket();
    }

    ~UdpSocket() {
        close(socket_fd);
    }

    void join_multicast(const struct address& _multi_addr, u16 _port) {
        // set multicast address
        dst_addr = get_address(_multi_addr.combined.c_str(), _port);

        // Join the multicast group
        ip_mreq multicastRequest{};
        multicastRequest.imr_multiaddr.s_addr = inet_addr(_multi_addr.combined.c_str());
        multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0) {
            throw std::runtime_error("Failed to join multicast group\n");
        }
    }

    void set_broadcast() const {
        int broadcastEnable = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
            throw std::runtime_error("Failed to set broadcast option\n");
        }
    }

    void set_destination_addr(const struct address& _dst_addr, u16 _port) {
        dst_addr = get_address(_dst_addr.combined.c_str(), _port);
    }

    void bind_socket(u16 port) const {
        bind_socket_port(socket_fd, port);
    }

    void bind_to_random() const {
        bind_socket_port(socket_fd, 0);
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

    void send_message(char* msg, size_t n) {
        errno = 0;
        auto err = sendto(socket_fd, msg, n, 0, reinterpret_cast<struct sockaddr*>(&dst_addr),
                          sizeof(dst_addr));

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
//        cout << "read len:" << len << "\n";

        if (len < 0 && (errno == ENETDOWN || errno == ENETUNREACH)) {
            return -1;
        } else if (len < 0) {
            throw std::runtime_error("receive message failed");
        }

        return len;
    }
};

class TcpSocket {
private:
    int socket_fd{};
    sockaddr_in addr;

public:
    TcpSocket(u16 port) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            throw std::runtime_error("Failed creating socket");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }
    }



    ~TcpSocket() {
        close(socket_fd);
    }
};

#endif //SIKRADIO_CONNECTION_H
