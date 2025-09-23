#ifndef CPP_LEARN_TCPCONNECTIONMANAGER_H
#define CPP_LEARN_TCPCONNECTIONMANAGER_H


#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include <system_error>
#include <condition_variable>
#include <assert.h>

class TcpConnectionManager {
public:
    // 连接状态枚举
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING,
        ERROR
    };
    // 连接信息结构体
    struct ConnectionInfo {
        int fd; // 套接字描述符
        std::string remote_address; 
        uint16_t remote_port;
        ConnectionState state;
        time_t connection_time;
    };
    using ConnectionCallback = std::function<void(const ConnectionInfo&)>;
    using DataCallback = std::function<void(const ConnectionInfo&,const std::vector<uint8_t>&)>;
    using ErrorCallback = std::function<void(const ConnectionInfo&,const std::error_code&)>;
private:
    std::unordered_map<int,std::shared_ptr<ConnectionInfo>> connections_;
    ConnectionCallback connection_callback_;
    DataCallback data_callback_;
    ErrorCallback error_callback_;
    int listen_fd_ = -1;
    bool running_ = false;
public:
    TcpConnectionManager() = default;
    ~TcpConnectionManager(){
        stop();
    }

    void setConnectionCallback(ConnectionCallback cb){
        connection_callback_=cb;
    }
    void setDataCallback(DataCallback db){
        data_callback_ = db;
    }
    void setErrorCallback(ErrorCallback eb){
        error_callback_ = eb;
    }

    // 启动服务器
    std::error_code start(uint16_t port){
        /*
            创建监听套接字
            AF_INET: IPv4
            SOCK_STREAM: TCP
            0: 默认协议，这里默认选择TCP协议，显式指定： IPPROTO_TCP,IPPROTO_UDP
        */
        listen_fd_ = socket(AF_INET,SOCK_STREAM,0);
        if(listen_fd_ < 0){
            return std::error_code(errno,std::generic_category());
        }
        // 设置SO_REUSEADDR选项，允许重用处于处于 TIME_WAIT 状态的本地地址和端口
        int opt = 1;
        if(setsockopt(listen_fd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0){
            close(listen_fd_);
            return std::error_code(errno,std::generic_category());
        }
        // 绑定地址和端口
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有可用接口
        server_addr.sin_port = htons(port); // 转换为网络字节序
        if(bind(listen_fd_,(sockaddr*)&server_addr,sizeof(server_addr)) < 0){
            close(listen_fd_);
            return std::error_code(errno,std::generic_category());
        }
        // 开始监听
        // SOMAXCONN: 系统允许的最大连接数,linux系统下通常为128
        if(listen(listen_fd_,SOMAXCONN)<0){
            close(listen_fd_);
            return std::error_code(errno,std::generic_category());
        }
        running_ = true;
        std::thread([this](){
            this->acceptConnections();
        }).detach();
        return std::error_code(); // 成功返回默认构造的错误码，表示无错误
    }
    // 停止服务器
    void stop(){
        running_ = false;
        if(listen_fd_ != -1){
            close(listen_fd_);
            listen_fd_ = -1;
        }
        // 关闭所有连接
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for(const auto& pair : connections_){
            closeConnection(pair.first);
        }
        connections_.clear();
    }
    // 主动建立连接
    std::error_code connect(const std::string& host,uint16_t port,std::shared_ptr<ConnectionInfo>& conn_info){
        running_ = true;
        int fd =socket(AF_INET,SOCK_STREAM,0);
        if(fd<0){
            return std::error_code(errno,std::generic_category());
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        // 将点分十进制字符串转换为网络字节序的二进制形式
        if(inet_pton(AF_INET,host.c_str(),&server_addr.sin_addr)<=0){
            close(fd);
            return std::error_code(errno,std::generic_category());
        }
        auto info = std::make_shared<ConnectionInfo>();
        info->fd = fd;
        info->remote_address = host;
        info->remote_port = port;
        info->state = ConnectionState::CONNECTING;
        info->connection_time = time(nullptr);
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_[fd] = info;
        }
        // ::conect 避免与成员函数重名
        if(::connect(fd,(sockaddr*)&server_addr,sizeof(server_addr))<0){
            close(fd);
            return std::error_code(errno,std::generic_category());
        }
        info->state = ConnectionState::CONNECTED;
        if(connection_callback_){
            connection_callback_(*info);
        }
        std::thread([this,fd](){
            this->receiveData(fd);
        }).detach();
        conn_info = info;
        return std::error_code();
    }
    // 发送数据
    std::error_code sendData(int fd, const std::vector<uint8_t>& data) {
        size_t total_sent = 0;
        while (total_sent < data.size()) {
            ssize_t sent = ::send(fd,
                                data.data() + total_sent,
                                data.size() - total_sent,
                                MSG_NOSIGNAL);
            if (sent < 0) {
                if (errno == EINTR) {
                    continue; // 被信号打断，重试
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 对端接收缓冲区满了，可以选择 epoll 等待再写
                    // 简单实现：直接返回错误
                    return std::error_code(errno, std::generic_category());
            }
                return std::error_code(errno, std::generic_category());
            }
            total_sent += static_cast<size_t>(sent);
        }
        return {}; // success
    }
    void closeConnection(int fd) {
        if (fd < 0) return;

        // 半关闭写端，告诉对方不再发数据
        ::shutdown(fd, SHUT_WR);

        // 直接关闭，不阻塞读
        ::close(fd);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_.erase(fd);
        }

        // 通知回调
        if (connection_callback_) {
            TcpConnectionManager::ConnectionInfo info;
            info.fd = fd;
            info.state = TcpConnectionManager::ConnectionState::DISCONNECTED;
            connection_callback_(info);
        }
    }

    // 获取活动连接数
    size_t getConnectionCount() const{
        std::lock_guard<std::mutex> lock(connections_mutex_);
        return connections_.size();
    }
private:
    mutable std::mutex connections_mutex_;
    // 接受新连接
    void acceptConnections(){
        while(running_){
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            /*
                accept 函数的第二个参数 &client_addr：是一个输出参数，内核会将客户端的地址信息（IP、端口、地址族）填充到这个结构体中。
            */
            int client_fd = accept(listen_fd_,(struct sockaddr*)&client_addr,&client_len);
            if(client_fd < 0){
                if(running_){
                    std::error_code ec(errno,std::system_category());
                    handleError(-1,ec);
                }
                continue;
            }
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&client_addr.sin_addr,ip_str,INET_ADDRSTRLEN);
            auto info = std::make_shared<ConnectionInfo>();
            info->fd = client_fd;
            info->remote_address = ip_str;
            info->remote_port = ntohs(client_addr.sin_port);
            info->state = ConnectionState::CONNECTED;
            info->connection_time = time(nullptr);
            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                connections_[client_fd] = info;
            }
            if(connection_callback_){
                connection_callback_(*info);
            }
            std::thread([this,client_fd](){
                this->receiveData(client_fd);
            }).detach();
        }
    }
    // 接受数据
    void receiveData(int fd){
        std::shared_ptr<ConnectionInfo> conn;
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            auto it = connections_.find(fd);
            if(it == connections_.end()){
                return;
            }
            conn = it->second;
        }
        std::vector<uint8_t> buffer(4096);
        while(running_){
            ssize_t bytes_received = recv(fd,buffer.data(),buffer.size(),0);
            
            if(bytes_received > 0){
                std::vector<uint8_t> data(buffer.begin(),buffer.begin()+bytes_received);
                if(data_callback_){
                    data_callback_(*conn,data);
                }
            }
            else if(bytes_received == 0){
                closeConnection(fd);
                break;
            }
            else{
                if(errno != EAGAIN && errno!= EWOULDBLOCK){
                    std::error_code ec(errno,std::system_category());
                    handleError(fd,ec);
                    closeConnection(fd);
                    break;
                }
            }
        }
    }
    // 错误处理
    void handleError(int fd,const std::error_code& ec){
        if(error_callback_){
            if(fd == -1){
                std::lock_guard<std::mutex> lock(connections_mutex_);
                auto it = connections_.find(fd);
                if(it != connections_.end()){
                    it->second->state = ConnectionState::ERROR;
                    error_callback_(*(it->second),ec);
                }
            }
            else{
                ConnectionInfo info;
                info.state = ConnectionState::ERROR;
                error_callback_(info,ec);
            }
        }
    }
};

namespace TcpConnectionManager_Test {
    std::mutex mutex;
    std::condition_variable cv;
    void test() {
        TcpConnectionManager server;
        TcpConnectionManager client;
        
        // 同步工具
        bool server_ready = false;
        bool server_connected = false;
        bool client_connected = false;
        bool server_received = false;        // 服务器收到（仅服务器设置）
        bool client_received = false;        // 客户端收到回显（仅客户端设置）
        std::string client_echo_str;         // 保存客户端接收到的回显，便于断言

        // 1. 服务器初始化
        server.setConnectionCallback([&](const TcpConnectionManager::ConnectionInfo& conn) {
            std::lock_guard<std::mutex> lock(mutex);
            if (conn.state == TcpConnectionManager::ConnectionState::CONNECTED) {
                std::cout << "[Server] Connection from " 
                          << conn.remote_address << ":" << conn.remote_port
                          << " established\n";
                server_connected = true;
            } else if (conn.state == TcpConnectionManager::ConnectionState::DISCONNECTED) {
                std::cout << "[Server] Connection closed\n";
                server_connected = false;
            }
            cv.notify_all();
        });
        
        server.setDataCallback([&](const TcpConnectionManager::ConnectionInfo& conn, 
                                 const std::vector<uint8_t>& data) {
            std::lock_guard<std::mutex> lock(mutex);
            std::cout << "[Server] Received " << data.size() << " bytes\n";
            std::vector<uint8_t> echo(data.rbegin(), data.rend());
            std::string echo_str(echo.begin(), echo.end());
            std::cout << "[Server] Echo payload: " << echo_str << "\n";
            if (auto ec = server.sendData(conn.fd, echo)) {
                std::cout << "[Server] Failed to send echo: " << ec.message() << "\n";
            } else {
                std::cout << "[Server] Echo sent successfully\n";
            }

            server_received = true;
            cv.notify_all();
        });
        
        // 启动服务器
        if (auto ec = server.start(12345)) {
            std::cerr << "Server failed to start: " << ec.message() << "\n";
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            server_ready = true;
            std::cout << "[Server] Ready on port 12345\n";
            cv.notify_all();
        }
        
        // 2. 客户端初始化
        std::shared_ptr<TcpConnectionManager::ConnectionInfo> client_conn;
        client.setConnectionCallback([&](const TcpConnectionManager::ConnectionInfo& conn) {
            std::cout << "[Client] Connection state changed to: ";
            switch(conn.state) {
                case TcpConnectionManager::ConnectionState::CONNECTED: 
                    std::cout << "CONNECTED"; break;
                case TcpConnectionManager::ConnectionState::DISCONNECTING: 
                    std::cout << "DISCONNECTING"; break;
                case TcpConnectionManager::ConnectionState::DISCONNECTED: 
                    std::cout << "DISCONNECTED"; break;
                case TcpConnectionManager::ConnectionState::ERROR: 
                    std::cout << "ERROR"; break;
                default: 
                    std::cout << static_cast<int>(conn.state);
            }
            std::cout << "\n";
            std::lock_guard<std::mutex> lock(mutex);
            if (conn.state == TcpConnectionManager::ConnectionState::CONNECTED) {
                std::cout << "[Client] Connected to server\n";
                client_connected = true;
            } else {
                std::cout << "[Client] Connection state changed: " 
                          << static_cast<int>(conn.state) << "\n";
                client_connected = false;
            }
            cv.notify_all();
        });
        
        client.setDataCallback([&](const TcpConnectionManager::ConnectionInfo& conn, 
                                 const std::vector<uint8_t>& data) {
            std::lock_guard<std::mutex> lock(mutex);
            std::string received(data.begin(), data.end());
            std::cout << "[Client] Received echo: " << received << "\n";
            client_received = true;
            cv.notify_all();
        });
        
        // 等待服务器启动
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait_for(lock, std::chrono::seconds(3), [&]{ return server_ready; });
            if (!server_ready) {
                std::cerr << "Timeout waiting for server to start\n";
                return;
            }
        }

        // 客户端连接
        if (auto ec = client.connect("127.0.0.1", 12345, client_conn)) {
            std::cerr << "Client failed to connect: " << ec.message() << "\n";
            return;
        }
        
        // 等待连接建立
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!cv.wait_for(lock, std::chrono::seconds(3), [&]{ 
                return server_connected && client_connected; 
            })) {
                std::cerr << "Timeout waiting for connection to establish\n";
                return;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 确保稳定
        if (server.getConnectionCount() != 1) {
            std::cerr << "Connection count mismatch. Expected 1, got: " 
                      << server.getConnectionCount() << "\n";
            return;
        }
        
        // 3. 测试数据传输
        std::string test_message = "Hello World!";
        std::vector<uint8_t> test_data(test_message.begin(), test_message.end());
        
        if (auto ec = client.sendData(client_conn->fd, test_data)) {
            std::cerr << "Failed to send data: " << ec.message() << "\n";
            return;
        }
        std::cout << "[Client] Data sent\n";
        
        // 等待数据回传
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!cv.wait_for(lock, std::chrono::seconds(3), [&]{ return client_received; })) {
                std::cerr << "Timeout waiting for client to receive echo\n";
                return;
            }
        }
        
        // 4. 测试连接关闭
        try {
            client.closeConnection(client_conn->fd);
        } catch (const std::exception& e) {
            std::cerr << "Failed to close connection: " << e.what() << "\n";
            return;
        }
        
        // 等待连接关闭
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!cv.wait_for(lock, std::chrono::seconds(3), [&]{ 
                return !server_connected && !client_connected; 
            })) {
                std::cerr << "Timeout waiting for disconnection\n";
                return;
            }
        }
        
        if (server.getConnectionCount() != 0) {
            std::cerr << "Expected 0 connections after close, got: " 
                      << server.getConnectionCount() << "\n";
            return;
        }
        
        std::cout << "All TCP connection manager tests passed successfully!\n";
    }
};



#endif //CPP_LEARN_TCPCONNECTIONMANAGER_H