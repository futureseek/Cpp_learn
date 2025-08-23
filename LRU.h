//
// Created by 12793 on 2025/8/21.
//

#ifndef CLION_TEST_LRU_H
#define CLION_TEST_LRU_H
#include <iostream>
#include <unordered_map>
#include <utility>


struct ListNode {
    int key;
    int value;
    ListNode* prev;
    ListNode* next;

    ListNode(int k,int v):key(k),value(v),prev(nullptr),next(nullptr){}
};

class DoublyLinkedList{
public:
    ListNode* head;
    ListNode* tail;

    DoublyLinkedList(){
        head = new ListNode(0,0);
        tail = new ListNode(0,0);
        head->next = tail;
        tail->prev = head;
    }

    ~DoublyLinkedList(){
        clear();
        delete head;
        delete tail;
    }

    // 清空链表
    void clear()
    {
        ListNode* now = head->next;
        while(now != tail){
            ListNode* nextNode = now->next;
            delete now;
            now = nextNode;
        }
        head->next = tail;
        tail->prev = head;
    }

    void push_front(ListNode* node)
    {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

    void erase(ListNode* node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    void move_to_front(ListNode* node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        push_front(node);
    }

    ListNode* pop_back() {
        ListNode *node = tail->prev;
        if (node == head) return nullptr;
        // 断开连接但是不删除节点
        node->prev->next = tail;
        tail->prev = node->prev;
        node->prev = nullptr;
        node->next = nullptr;
        return node;
    }
};

class LRUCache{
public:
    LRUCache(int capacity):_capacity(capacity){}

    int get(int key){
        auto it=_cache.find(key);
        if(it == _cache.end())
        {
            return -1;
        }
        _list.move_to_front(it->second);
        return it->second->value;
    }

    void put(int key,int value)
    {
        auto it=_cache.find(key);
        if(it != _cache.end()){
            it->second->value=value;
            _list.move_to_front(it->second);
        }
        else{
            if(_cache.size()==_capacity)
            {
                ListNode* node = _list.pop_back();
                if(node!= nullptr){
                    _cache.erase(node->key);
                    delete node;
                }

            }
            ListNode* newNode = new ListNode(key,value);
            _list.push_front(newNode);
            _cache[key] = newNode;
        }
    }

private:
    int _capacity;
    DoublyLinkedList _list;
    std::unordered_map<int,ListNode*> _cache;
};



#endif //CLION_TEST_LRU_H
