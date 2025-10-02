# pragma once
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>  
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <mysql_driver.h>

class MysqlPool{
private:
    std::queue<sql::Connection*> pool_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::string host_,user_,password_,db_;
    int poolSize_;
    bool initialized_ = false;
    sql::mysql::MySQL_Driver *driver_ = nullptr;

    MysqlPool() = default;
    MysqlPool(const MysqlPool&) = delete;
    MysqlPool& operator=(const MysqlPool&) = delete;
    ~MysqlPool(){
        std::lock_guard<std::mutex> lock(mtx_);
        while(!pool_.empty()){
            auto conn = pool_.front();
            pool_.pop();
            if(conn && !conn->isClosed()){
                conn->close();
            }
            delete conn;
        }
    }
public:
    static MysqlPool& getInstance(){
        static MysqlPool instance;
        return instance;
    }
    // 初始化连接池
    void initialize(const std::string& host,const std::string& user,const std::string& password,const std::string& db,int poolSize){
        if(initialized_){
            throw std::runtime_error("Connection pool already initialized");
        }
        host_ = host;
        user_ = user;
        password_ = password;
        db_ = db;
        poolSize_ = poolSize;
        driver_ = sql::mysql::get_mysql_driver_instance();
        for(int i=0;i<poolSize_;++i){
            try{
                sql::Connection* rawConn = driver_->connect(host_,user_,password_);
                rawConn->setSchema(db_);
                pool_.push(rawConn);
            }catch(sql::SQLException &e){
                throw std::runtime_error("Failed to create connection: "+std::string(e.what()));
            }
        }
        initialized_ = true;
    }
    // 获取连接
    std::shared_ptr<sql::Connection> getConnection(){
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock,[this](){
            return !pool_.empty();
        });
        auto rawConn = pool_.front();
        pool_.pop();

        // 自动回池的 shared_ptr deleter
        auto sp = std::shared_ptr<sql::Connection>(rawConn, [this](sql::Connection* c){
            std::lock_guard<std::mutex> lock(this->mtx_);
            this->pool_.push(c);
            this->cv_.notify_one();
        });

        // 重连
        if(rawConn->isClosed()){
            try{
                rawConn->reconnect();
                rawConn->setSchema(db_);
            }catch(sql::SQLException &e){
                throw std::runtime_error("Failed to create connection: "+std::string(e.what()));
            }
        }
        return sp;
    }
};

namespace MysqlPool_Test {
    void test(){
        try {
            auto& pool = MysqlPool::getInstance();
            pool.initialize("tcp://127.0.0.1:3306", "root", "qwer0122", "book_manage", 3);

            {
                auto conn = pool.getConnection();
                std::unique_ptr<sql::Statement> stmt(conn->createStatement());
                std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SHOW TABLES;"));

                std::cout << "Tables in book_manage:" << std::endl;
                while(res->next()){
                    std::cout << " - " << res->getString(1) << std::endl;
                }
            }

            std::cout << "Test finished successfully." << std::endl;
        } catch (sql::SQLException &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
        } catch (std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}