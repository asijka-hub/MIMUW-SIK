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

        bind_socket(_socket_fd, port);
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
        struct sockaddr_in client_address{};
        auto address_length = (socklen_t) sizeof(client_address);
        int flags = 0; // we do not request anything special
        errno = 0;
        ssize_t len = recvfrom(_socket_fd, buffer.data(), buffer.capacity(), flags,
                               (struct sockaddr *) &client_address, &address_length);
        cout << "read len:" << len << "\n";
        if (len < 0) {
            PRINT_ERRNO();
        }
        return (size_t) len;
    }
};

#endif //SIKRADIO_CONNECTION_H
