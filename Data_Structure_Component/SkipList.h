//
// Created by 12793 on 2025/9/17.
//

#ifndef CPP_LEARN_SKIPLIST_H
#define CPP_LEARN_SKIPLIST_H

#include <vector>
#include <random>
#include <limits>

template<typename T>
class SkipList{
private:
    struct Node{
        T value;
        std::vector<Node*> forward;
        Node(const T&v,int level):value(v),forward(level, nullptr){}
    };
    int max_level;
    float probability; //层数增长概率
    Node* head;
    std::mt19937 rng;
public:
    SkipList(int max_lvl = 16,float p=0.5):max_level(max_lvl),probability(p),head(new Node(std::numeric_limits<T>::min(),max_level)),rng(std::random_device{}()){}
    ~SkipList(){
        Node* cur = head;
        while(cur){
            Node* next = cur->forward[0];
            delete cur;
            cur = next;
        }
    }
private:
    int random_level(){
        int level = 1;
        while(level<max_level&&(rng()%100)<probability*100){
            level++;
        }
        return level;
    }
public:
    bool find(const T& target){
        Node* cur = head;
        for(int level = max_level-1;level>=0;level--){
            while(cur->forward[level] && cur->forward[level]->value < target){
                cur = cur->forward[level];
            }
        }
        cur = cur->forward[0];
        return cur!= nullptr && cur->value==target;
    }

    void insert(const T& value){
        // 记录每层前驱节点
        std::vector<Node*> update(max_level, nullptr);
        Node* cur = head;
        for(int level = max_level-1;level >=0;level--){
            while(cur->forward[level] && cur->forward[level]->value < value){
                cur = cur->forward[level];
            }
            update[level]=cur;
        }
        int new_level = random_level();
        Node* new_node = new Node(value,new_level);
        // 逐层插入新节点
        for(int level = 0;level < new_level;level++){
            new_node->forward[level] = update[level]->forward[level];
            update[level]->forward[level]=new_node;
        }
    }

    bool erase(const T& value){
        std::vector<Node*> update(max_level, nullptr);
        Node* cur = head;
        for(int level = max_level-1;level >=0;level--){
            while(cur->forward[level] && cur->forward[level]->value < value){
                cur = cur->forward[level];
            }
            update[level]=cur;
        }
        cur = cur->forward[0];
        if(!cur||cur->value!=value){
            return false;
        }
        for(int level = 0;level<max_level;level++){
            if(update[level]->forward[level]!=cur){
                break;
            }
            update[level]->forward[level]=cur->forward[level];
        }
        delete cur;
        return true;
    }
};

namespace SkipList_Test{
  void test(){
      SkipList<int> skiplist;
      skiplist.insert(3);
      skiplist.insert(6);
      skiplist.insert(7);
      skiplist.insert(9);
      skiplist.insert(12);
      std::cout << "Search 6: " << skiplist.find(6) << '\n'; // 1 (true)
      std::cout << "Search 5: " << skiplist.find(5) << '\n'; // 0 (false)
      skiplist.erase(6);
      std::cout << "Search 6 (after erase): " << skiplist.find(6) << '\n'; // 0 (false)
  }
};

#endif //CPP_LEARN_SKIPLIST_H
