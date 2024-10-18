#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm> // remove

using namespace std;

#pragma comment(lib, "Ws2_32.lib") // 链接Winsock库，确保编译器在链接阶段包含Ws2_32.lib

#define PORT 8080        // 服务器将使用的端口号
#define BUFFER_SIZE 1024 // 缓冲区大小，用于接收和发送数据

vector<SOCKET> clients; // 保存所有已连接的客户端套接字
mutex client_mutex;     // 互斥锁，用于保护对客户端列表的访问

/**
 * @brief 广播消息给所有已连接的客户端
 * @param message 要广播的消息内容
 * @param sender_socket 发送消息的客户端套接字，消息不会回送给该客户端
 */
void broadcastMessage(const string &message, SOCKET sender_socket)
{
    lock_guard<mutex> lock(client_mutex); // 锁住互斥锁，以确保线程安全访问客户端列表
    for (SOCKET client_socket : clients)
    {
        if (client_socket != sender_socket)
        {                                                            // 确保消息不发送回发送者
            send(client_socket, message.c_str(), message.size(), 0); // 向客户端发送消息
        }
    }
}

/**
 * @brief 处理单个客户端的连接
 * @param client_socket 已连接的客户端套接字
 */
void handleClient(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE]; // 用于存储从客户端接收的数据
    while (true)
    {
        auto currentTime = std::chrono::system_clock::now();
        time_t timestamp = std::chrono::system_clock::to_time_t(currentTime);
        tm localTime;
        localtime_s(&localTime, &timestamp);
        char timeStr[50];

        memset(buffer, 0, BUFFER_SIZE);                                   // 将缓冲区清零，以准备接收新消息
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0); // 接收来自客户端的数据
        if (bytes_received <= 0)
        { // 如果接收失败或客户端断开连接
            string message = std::string("(") + timeStr + ")" + "\n" + "Server: Client" + std::to_string(client_socket) + " is disconnected.\n";
            cout << message;
            broadcastMessage(message, client_socket);
            closesocket(client_socket);                                                          // 关闭与该客户端的连接
            lock_guard<mutex> lock(client_mutex);                                                // 锁住互斥锁，以安全地操作客户端列表
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end()); // 从列表中移除断开的客户端
            break;                                                                               // 结束当前客户端处理线程
        }

        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTime); // 格式化时间输出

        string message = std::string("(") + timeStr + ")" + "\n" + "Client" + std::to_string(client_socket) + ": " + buffer + "\n";
        cout << message;
        broadcastMessage(message, client_socket); // 广播消息给其他客户端
    }
}

int main()
{
    WSADATA wsaData;
    // 初始化Winsock库，以便使用网络功能
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    { // 使用Winsock版本2.2
        cerr << "Failed to initialize Winsock." << endl;
        return -1;
    }

    // 创建一个套接字，用于监听客户端连接
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        cerr << "Failed to create socket." << endl;
        WSACleanup(); // 清理Winsock资源
        return -1;
    }

    // 服务器地址信息
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 可接收所有可用的网络接口
    server_addr.sin_port = htons(PORT);

    // 绑定套接字到指定的IP地址和端口
    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        cerr << "Binding failed." << endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    // 使服务器进入监听状态，准备接受连接请求
    if (listen(server_socket, 5) == SOCKET_ERROR)
    { // 最大等待队列长度为5
        cerr << "Error in listen." << endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    cout << "Server started on port " << PORT << "..." << endl;

    // 主循环：接受客户端连接并启动新的线程来处理每个客户端
    while (1)
    {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr); // 接受一个新的客户端连接
        if (client_socket == INVALID_SOCKET)
        {
            cerr << "Server accept failed." << endl;
            continue;
        }
        lock_guard<mutex> lock(client_mutex); // 锁住互斥锁，以线程安全地操作客户端列表
        clients.push_back(client_socket);     // 将新连接的客户端添加到客户端列表

        auto currentTime = std::chrono::system_clock::now();
        time_t timestamp = std::chrono::system_clock::to_time_t(currentTime);
        tm localTime;
        localtime_s(&localTime, &timestamp);
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTime); // 格式化时间输出

        string message = std::string("(") + timeStr + ")" + "\n" + "Server: Client" + std::to_string(client_socket) + " is connected.\n";
        cout << message;
        broadcastMessage(message, client_socket);
        thread(handleClient, client_socket).detach(); // 为新客户端启动一个线程，并与主线程分离
    }
    closesocket(server_socket); // 关闭服务器套接字
    WSACleanup();               // 清理Winsock资源
    cout << "Exited." << endl;
    return 0;
}
