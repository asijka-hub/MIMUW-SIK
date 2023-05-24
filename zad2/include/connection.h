//
// Created by Andrzej on 22.05.2023.
//

#ifndef SIKRADIO_CONNECTION_H
#define SIKRADIO_CONNECTION_H

//class UdpSocket {
//private:
//    int socket_fd{};
//    struct sockaddr_in sender_addr{};
//
//
//    void create_socket(const struct address& addr, u16 port) {
//        socket_fd = open_socket();
//
////        _receiver_addr = get_address(addr.combined.c_str(), port);
//
//        bind_socket(socket_fd, port);
//    }
//
//public:
//    UdpSocket() = delete;
//
//    UdpSocket(const struct address& addr, u16 port) {
//        create_socket(addr, port);
//    }
//
//    ~UdpSocket() {
//        close(socket_fd);
//    }
//
//    void send_reply(const char* msg, size_t length) const {
//        auto address_length = (socklen_t) sizeof(sender_addr);
//        int flags = 0;
//        ssize_t sent_length = sendto(socket_fd, msg, length, flags,
//                                     (struct sockaddr *) &sender_addr, address_length);
//        ENSURE(sent_length == (ssize_t) length);
//    }
//
//    size_t recv_message(std::vector<char>& buffer) {
//        auto address_length = (socklen_t) sizeof(sender_addr);
//        int flags = 0; // we do not request anything special
//        errno = 0;
//        ssize_t len = recvfrom(socket_fd, buffer.data(), buffer.capacity(), flags,
//                               (struct sockaddr *) &sender_addr, &address_length);
//        cout << "read len:" << len << "\n";
//        if (len < 0) {
//            PRINT_ERRNO();
//        }
//        return (size_t) len;
//    }
//};

inline static struct sockaddr_in get_address(const char *host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result;
    CHECK(getaddrinfo(host, NULL, &hints, &address_result));

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
            throw std::runtime_error("Failed to create socket");
        }

        // set multicast address


        // Join the multicast group
        ip_mreq multicastRequest{};
        multicastRequest.imr_multiaddr.s_addr = inet_addr(_multi_addr.combined.c_str());
        multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0) {
            throw std::runtime_error("Failed to join multicast group");
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

    void send_reply(const char* msg, size_t length) const {
        auto address_length = (socklen_t) sizeof(sender_addr);
        int flags = 0;
        ssize_t sent_length = sendto(socket_fd, msg, length, flags,
                                     (struct sockaddr *) &sender_addr, address_length);
        ENSURE(sent_length == (ssize_t) length);
    }

    void multicast_message(std::vector<char>& buffer) {

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
