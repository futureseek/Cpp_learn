//
// Created by 12793 on 2025/8/28.
//

#ifndef CPP_LEARN_FILESYSTEM_H
#define CPP_LEARN_FILESYSTEM_H

#include <iostream>
#include <string>
#include <map>

class FileSystem{
    /*
     * 模拟文件结构，没有实际创建文件
     */
public:
    struct Node{
        std::string name;
        bool is_dir;
        Node(const std::string& _name,bool dir):name(_name),is_dir(dir){}
        virtual ~Node() = default;
    };

    struct Directory : Node {
        std::map<std::string,std::shared_ptr<Node>> children;
        Directory(const std::string& name): Node(name,true){};
    };
    struct File:Node{
        File(const std::string& name): Node(name,false){};
    };
private:
    std::shared_ptr<Directory> root;
    std::shared_ptr<Directory> current;

    std::shared_ptr<Directory> findDir(const std::string& path){
        if(path.empty()) return current;

        auto it = current->children.find(path);
        if(it!=current->children.end() && it->second->is_dir){
            return std::static_pointer_cast<Directory>(it->second);
        }
        return nullptr;
    }
public:
    FileSystem(){
        root = std::make_shared<Directory>("root");
        current = root;
    }
    bool mkdir(const std::string& name){
        if(current->children.find(name)!=current->children.end()){
            std::cout<<"already have"<<name<<std::endl;
            return false;
        }
        current->children[name]=std::make_shared<Directory>(name);
        std::cout<<"Create "<<name<<" successs"<<std::endl;
        return true;
    }
    bool createFile(const std::string& name) {
        if (current->children.find(name) != current->children.end()) {
            std::cout << "File already exists: " << name << std::endl;
            return false;
        }

        current->children[name] = std::make_shared<File>(name);
        std::cout << "Created file: " << name << std::endl;
        return true;
    }

    bool move(const std::string& source,const std::string& dest){
        auto src_it = current->children.find(source);
        if (src_it == current->children.end()) {
            std::cout << "Source not found: " << source << std::endl;
            return false;
        }

        // 检查目标是否存在
        if (current->children.find(dest) != current->children.end()) {
            std::cout << "Destination already exists: " << dest << std::endl;
            return false;
        }

        // 执行移动（重命名）
        auto node = src_it->second;
        current->children.erase(src_it);
        current->children[dest] = node;

        std::cout << "Moved " << source << " to " << dest << std::endl;
        return true;
    }
    void ls() {
        std::cout << "Contents:" << std::endl;
        for (const auto& pair : current->children) {
            if (pair.second->is_dir) {
                std::cout << "[DIR]  " << pair.first << std::endl;
            } else {
                std::cout << "[FILE] " << pair.first << std::endl;
            }
        }
    }
    void printTree(std::shared_ptr<Directory> dir = nullptr, int depth = 0) {
        if (!dir) dir = root;

        // 缩进显示层次
        for (int i = 0; i < depth; i++) std::cout << "  ";

        if (dir == root) {
            std::cout << "/root" << std::endl;
        } else {
            std::cout << dir->name << "/" << std::endl;
        }

        // 递归显示子节点
        for (const auto& pair : dir->children) {
            if (pair.second->is_dir) {
                printTree(std::static_pointer_cast<Directory>(pair.second), depth + 1);
            } else {
                for (int i = 0; i <= depth; i++) std::cout << "  ";
                std::cout << pair.first << std::endl;
            }
        }
    }
};


namespace FileSystem_Test{
  void test(){
      FileSystem fs;
      std::cout << "=== Simple File System Demo ===" << std::endl;

      // 测试创建目录
      std::cout << "\n1. Creating directories:" << std::endl;
      fs.mkdir("documents");
      fs.mkdir("downloads");
      fs.mkdir("pictures");

      // 测试创建文件
      std::cout << "\n2. Creating files:" << std::endl;
      fs.createFile("readme.txt");
      fs.createFile("notes.doc");

      // 测试列出内容
      std::cout << "\n3. Listing contents:" << std::endl;
      fs.ls();

      // 测试移动/重命名
      std::cout << "\n4. Moving/renaming:" << std::endl;
      fs.move("documents", "mydocs");
      fs.move("readme.txt", "README.txt");

      // 最终结构展示
      std::cout << "\n5. Final file system structure:" << std::endl;
      fs.printTree();
  }
};

#endif //CPP_LEARN_FILESYSTEM_H
