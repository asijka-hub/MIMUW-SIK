//
// Created by Andrzej on 22.05.2023.
//

#ifndef SIKRADIO_CONNECTION_H
#define SIKRADIO_CONNECTION_H

class UdpSocket {
private:
    int _socket_fd{};
    struct sockaddr_in _receiver_addr{};

    void create_socket(const struct address& addr, u16 port) {
        _socket_fd = open_socket();

        _receiver_addr = get_address(addr.combined.c_str(), port);

        connect_socket(_socket_fd, &_receiver_addr); // TODO usunac connect
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

    size_t recv_message(std::vector<char>& buffer) {
        return receive_message(_socket_fd, (void*)buffer.data(), buffer.size(), 0);
    }
};

#endif //SIKRADIO_CONNECTION_H
