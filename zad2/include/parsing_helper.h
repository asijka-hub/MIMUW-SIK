//
// Created by andrzej on 5/22/23.
//

#ifndef SIKRADIO_PARSING_HELPER_H
#define SIKRADIO_PARSING_HELPER_H

#include <boost/program_options.hpp>
#include <iostream>
#include <unistd.h>
#include <cstdio>

using std::cout;
using std::cerr;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i64 = int64_t;

const u32 index_number = 429592;
const u16 DEFAULT_DATA_PORT = static_cast<u16>(20000 + index_number % 10000);
const std::string DEFAULT_DISCOVER_ADDR = "255.255.255.255";
const u16 DEFAULT_CTRL_PORT = static_cast<u16>(30000 + index_number % 10000);
const u16 DEFAULT_UI_PORT = static_cast<u16>(10000 + index_number % 10000);
const u64 DEFAULT_PSIZE = 512;
const u64 DEFAULT_BSIZE = 65536;
const u64 DEFAULT_FSIZE = 128 * 1024;
const u64 DEFAULT_RTIME = 250;
const std::string DEFAULT_NAME = "Nienazwany Nadajnik";

namespace po = boost::program_options;


void check_positive(auto number) {
    if (number <= 0)
        throw po::validation_error(po::validation_error::invalid_option_value);
}

void check_port(int port) {
    if (port < 0 || port > 65535)
        throw po::validation_error(po::validation_error::invalid_option_value);
}

struct address {
    std::string combined;
};

struct SenderArgs {
    struct address mcast_addr;
    u16 data_port{};
    u16 ctrl_port{};
    u64 psize{};
    u64 fsize{};
    u64 rtime{};
    std::string name;
};

struct SenderArgs parse_sender_args(int argc, char **argv) {
    struct SenderArgs program_args;
    po::options_description desc("Sender options");

    int data_port, ctrl_port;
    i64 psize, fsize, rtime;

    desc.add_options()
            ("help,h", "produce help message")
            ("a,a", po::value<std::string>(&program_args.mcast_addr.combined)->required(), "broadcast address")
            ("P,P", po::value<int>(&data_port)->default_value(DEFAULT_DATA_PORT)->notifier(&check_port),
             "port used for DATA transfer [0-65535]")
            ("C,C", po::value<int>(&ctrl_port)->default_value(DEFAULT_CTRL_PORT)->notifier(&check_port),
             "port used for CONTROL data transfer [0-65535]")
            ("p,p", po::value<i64>(&psize)->default_value(DEFAULT_PSIZE)->notifier(&check_positive<i64>), "size of audio_data > 0")
            ("f,f", po::value<i64>(&fsize)->default_value(DEFAULT_FSIZE)->notifier(&check_positive<i64>), "size of fifo queue > 0")
            ("R,R", po::value<i64>(&rtime)->default_value(DEFAULT_RTIME)->notifier(&check_positive<i64>), "retransmission time > 0")
            ("n,n", po::value<std::string>(&program_args.name)->default_value(DEFAULT_NAME), "name");

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(0);
        }

        po::notify(vm);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }


    if (program_args.name.empty())
        throw std::runtime_error("name empty\n");

    if (program_args.name == "\"\"")
        throw std::runtime_error("not accepted name\n");

    program_args.data_port = static_cast<u16>(data_port);
    program_args.ctrl_port = static_cast<u16>(ctrl_port);
    program_args.psize = static_cast<u64>(psize);
    program_args.fsize = static_cast<u64>(fsize);
    program_args.rtime = static_cast<u64>(rtime);

    return program_args;
}

struct ReceiverArgs {
    struct address dsc_addr;
    u16 ctrl_port{};
    u16 ui_port{};
    u64 bsize{};
    u64 rtime{};
    std::string name;
};

struct ReceiverArgs parse_receiver_args(int argc, char **argv) {
    struct ReceiverArgs program_args;
    po::options_description desc("Sender options");

    int ui_port, ctrl_port;
    i64 bsize, rtime;

    desc.add_options()
            ("help,h", "produce help message")
            ("d,d", po::value<std::string>(&program_args.dsc_addr.combined)->default_value(DEFAULT_DISCOVER_ADDR), "broad cast address")
            ("C,C", po::value<int>(&ctrl_port)->default_value(DEFAULT_CTRL_PORT)->notifier(&check_port),
             "port used for CONTROL data transfer [0-65535]")
            ("U,U", po::value<int>(&ui_port)->default_value(DEFAULT_UI_PORT)->notifier(&check_port),
             "port used for CONTROL data transfer [0-65535]")
            ("b,b", po::value<i64>(&bsize)->default_value(DEFAULT_BSIZE)->notifier(&check_positive<i64>), "size of audio_data > 0")
            ("R,R", po::value<i64>(&rtime)->default_value(DEFAULT_RTIME)->notifier(&check_positive<i64>), "retransmission time > 0")
            ("n,n", po::value<std::string>(&program_args.name), "name");


    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(0);
        }

        po::notify(vm);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    program_args.ctrl_port = static_cast<u16>(ctrl_port);
    program_args.ui_port = static_cast<u16>(ui_port);
    program_args.bsize = static_cast<u64>(bsize);
    program_args.rtime = static_cast<u64>(rtime);

    return program_args;
}

#endif //SIKRADIO_PARSING_HELPER_H
