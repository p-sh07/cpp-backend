#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;
static const size_t max_udp_buffer_size = 65500;
void StartServer(uint16_t port) {

    try {
        Player player(ma_format_u8, 1);

        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
        std::cout << "Server started, waiting for message..." << std::endl;

        //Message recieving loop:
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, max_udp_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto msg_size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            if(msg_size > 0) {
                std::cout << "Message of Size[" << msg_size << "] Received, Starting playback..." << std::endl;

                player.PlayBuffer(recv_buf.data(), msg_size, 1.5s);
                std::cout << "Playback finished." << std::endl;
            }
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartClient(uint16_t port) {
    try {
        net::io_context io_context;
        Recorder recorder(ma_format_u8, 1);

        udp::socket socket(io_context, udp::v4());
        boost::system::error_code ec;

        //Message sending loop
        for(;;) {
            std::cout << "Enter Server ip and press Enter to start Recording..."s << std::endl;

            std::string server_ip;
            std::cin >> server_ip;

            if(server_ip == "lh"s) {
                server_ip = "127.0.0.1"s;
            }

            auto rec_result = recorder.Record(max_udp_buffer_size, 1.5s);

            std::cout << "Recording done, Sending..."s << std::endl;

            auto endpoint = udp::endpoint(net::ip::make_address(server_ip, ec), port);

            socket.send_to(net::buffer(rec_result.data), endpoint);

            std::cout << "Package Sent"s << std::endl;
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: "sv << argv[0] << " <[client] || [server]> <udp port>"sv << std::endl;
        return 1;
    }

    if(argv[1] == "client"s) {
        StartClient(std::stoi(argv[2]));
    } else if(argv[1] == "server"s) {
        StartServer(std::stoi(argv[2]));
    } else {
        std::cerr << "Error: incorrect second argument passed to function,\n recognises only: \'server\' or \'client\'\n"s;
        return 2;
    }

    // std::string str;
    //
    //     std::cout << "Press Enter to record message..." << std::endl;
    //     std::getline(std::cin, str);
    //
    //     auto rec_result = recorder.Record(65000, 1.5s);
    //     std::cout << "Recording done" << std::endl;
    //
    //     player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
    //     std::cout << "Playing done" << std::endl;
    // }

    return 0;
}
