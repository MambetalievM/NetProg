#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>

class TcpEchoClient {
private:
    int socket_fd;
    std::string target_ip;
    int target_port;
    std::string payload;

    void show_usage() const {
        std::cout << "Клиент (TCP)\n";
        std::cout << "Использование: ./echo_client \n";
        std::cout << "Опции:\n";
        std::cout << "  -h, --help     Справка\n";
        std::cout << "  -a, --address  IP сервера (по умолчанию 172.16.40.1)\n";
        std::cout << "  -p, --port     Порт сервера (по умолчанию 7)\n";
        std::cout << "  -r, --run      Запуск с параметрами по умолчанию\n";
    }

public:
    TcpEchoClient() : socket_fd(-1), target_ip("172.16.40.1"), target_port(7) {}

    ~TcpEchoClient() {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }

    void process_arguments(int argc, char** argv) {
        static struct option long_opts[] = {
            {"help", no_argument, nullptr, 'h'},
            {"address", required_argument, nullptr, 'a'},
            {"port", required_argument, nullptr, 'p'},
            {"run", no_argument, nullptr, 'r'},
            {nullptr, 0, nullptr, 0}
        };

        int current_opt;
        while ((current_opt = getopt_long(argc, argv, "ha:p:r", long_opts, nullptr)) != -1) {
            switch (current_opt) {
                case 'h':
                    show_usage();
                    exit(0);
                case 'a':
                    target_ip = optarg;
                    break;
                case 'p':
                    target_port = std::atoi(optarg);
                    break;
                case 'r':
                    target_ip = "172.16.40.1";
                    target_port = 7;
                    break;
                default:
                    show_usage();
                    exit(1);
            }
        }

        if (optind < argc) {
            payload = argv[optind];
            for (int i = optind + 1; i < argc; ++i) {
                payload += " ";
                payload += argv[i];
            }
        } else {
            std::cout << "Напишите сообщение: ";
            std::getline(std::cin, payload);
            if (payload.empty()) {
                throw std::invalid_argument("Вы не ввели сообщение.");
            }
        }
    }

    void execute() {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Не удалось создать сокет.");
        }

        sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(target_port);

        if (inet_pton(AF_INET, target_ip.c_str(), &server_addr.sin_addr) <= 0) {
            throw std::invalid_argument("Неверный IP-адрес.");
        }

        if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            throw std::runtime_error("Не удалось установить TCP соединение.");
        }

        std::cout << "Отправка данных на " << target_ip << ":" << target_port << "...\n";
        
        if (send(socket_fd, payload.c_str(), payload.length(), 0) < 0) {
            throw std::runtime_error("Сбой при отправке данных.");
        }

        std::vector<char> rx_buffer(1024, 0);
        ssize_t bytes = recv(socket_fd, rx_buffer.data(), rx_buffer.size() - 1, 0);

        if (bytes > 0) {
            std::cout << "Ответ сервера: " << rx_buffer.data() << "\n";
        } else {
            std::cout << "Сервер разорвал соединение или не ответил.\n";
        }
    }
};

int main(int argc, char** argv) {
    try {
        TcpEchoClient client;
        client.process_arguments(argc, argv);
        client.execute();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
