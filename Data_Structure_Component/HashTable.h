//
// Created by 12793 on 2025/9/16.
//

#ifndef CPP_LEARN_HASHTABLE_H
#define CPP_LEARN_HASHTABLE_H

#include <vector>
#include <list>
#include <functional> // for std::hash
#include <string>

template <typename Key,typename Value,typename Hash = std::hash<Key>>
class HashTable{
private:
    using Bucket = std::list<std::pair<Key,Value>>;

    std::vector<Bucket> buckets;
    Hash hasher;
    size_t num_elements = 0;
    static constexpr double LOAD_FACTOR_THRESHOLD = 0.75;
    size_t getBucketIndex(const Key& key) const{
        return hasher(key)%buckets.size();
    }
    void rehash(){
        std::vector<Bucket> old_buckets = std::move(buckets);
        buckets.resize(std::max(size_t(1),old_buckets.size()*2));
        num_elements = 0;
        for(auto& bucket : old_buckets){
            for(auto& kv : bucket){
                insert(kv.first, kv.second);
            }
        }
    }
public:
    HashTable(size_t initial_size = 16):buckets(initial_size){}
    void insert(const Key& key,const Value& value){
        if(static_cast<double>(num_elements)/buckets.size()>LOAD_FACTOR_THRESHOLD){
            rehash();
        }
        size_t index = getBucketIndex(key);
        Bucket& bucket = buckets[index];
        auto it = std::find_if(bucket.begin(),bucket.end(),
                               [&key](const auto& kv){
            return kv.first == key;
        });
        if(it!=bucket.end()){
            it->second = value;
        }
        else{
            bucket.emplace_back(key,value);
            num_elements++;
        }
    }
    Value& find(const Key& key){
        size_t index = getBucketIndex(key);
        Bucket& bucket = buckets[index];
        auto it = std::find_if(bucket.begin(),bucket.end(),
                               [&key](const auto& kv){
            return kv.first == key;
        });
        if(it == bucket.end()){
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }
    const Value& find(const Key& key) const{
        size_t index = getBucketIndex(key);
        const Bucket& bucket = buckets[index];
        auto it = std::find_if(bucket.begin(),bucket.end(),
                               [&key](const auto& kv){
                                   return kv.first == key;
                               });
        if(it == bucket.end()){
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }
    bool erase(const Key& key){
        size_t index = getBucketIndex(key);
        Bucket& bucket = buckets[index];
        auto it = std::find_if(bucket.begin(),bucket.end(),
                               [&key](const auto& kv){
                                   return kv.first == key;
                               });
        if(it!=bucket.end()){
            bucket.erase(it);
            num_elements--;
            return true;
        }
        return false;
    }
    size_t size() const {
        return num_elements;
    }
    bool empty() const {
        return num_elements == 0;
    }

};

namespace HashTable_Test{
    void test() {
        HashTable<std::string, int> table;
        // Basic insert/find
        table.insert("apple", 10);
        table.insert("banana", 20);
        table.insert("orange", 30);
        std::cout << "Initial table:\n"
                  << "apple: " << table.find("apple") << '\n'
                  << "Size: " << table.size()<<'\n';
        // Force rehash by inserting many elements
        std::cout << "\nInserting 1000 elements...";
        for (int i = 0; i < 1000; ++i) {
            table.insert("key_" + std::to_string(i), i);
        }
        std::cout << "Done.\n"
                  << "Post-rehash stats - "
                  << "Size: " << table.size()<<'\n';
        // Verify data integrity post-rehash
        std::cout << "\nTesting pre-rehash keys:\n"
                  << "apple: " << table.find("apple") << '\n'
                  << "banana: " << table.find("banana") << '\n'
                  << "random new key: " << table.find("key_500") << '\n';
        // Test erase
        table.erase("banana");
        std::cout << "\nAfter erase 'banana':\n";
        try {
            std::cout << "banana: " << table.find("banana") << '\n';
        } catch (const std::out_of_range&) {
            std::cout << "banana: not found\n";
        }
    }
};

#endif //CPP_LEARN_HASHTABLE_H
