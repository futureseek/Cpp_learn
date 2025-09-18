//
// Created by 12793 on 2025/9/18.
//

#ifndef CPP_LEARN_BLOOMFILTER_H
#define CPP_LEARN_BLOOMFILTER_H

#include <iostream>
#include <vector>
#include <functional>
#include <cmath>


class BloomFilter{
private:
    std::vector<bool> bits;
    int size;
    int num_hashes;
    std::hash<std::string> hash1;
    std::hash<size_t> hash2;
public:
    BloomFilter(int expected_items,double false_positive_prob){
        // 计算最优位数组大小
        size = static_cast<int>(-(expected_items * log(false_positive_prob)) / pow(log(2), 2));
        // 计算最优哈希函数个数
        num_hashes = static_cast<int>(((double)size / expected_items) * log(2));

        bits.resize(size,false);
    }
    void insert(const std::string& item){
        size_t h1 = hash1(item);
        size_t h2 = hash2(h1);
        for(int i=0;i<num_hashes;i++){
            size_t idx = (h1+i*h2)%size;
            bits[idx]=true;
        }
    }
    bool contains(const std::string& item) const{
        size_t h1 = hash1(item);
        size_t h2 = hash2(h1);

        for(int i=0;i<num_hashes;i++){
            size_t idx = (h1+i*h2)%size;
            if(!bits[idx]){
                return false;
            }
        }
        return true;
    }

    // 返回当前误判率的理论估计值
    double false_positive_rate(int inserted_items) const {
        double exp = -(num_hashes * inserted_items) / static_cast<double>(size);
        return pow(1 - exp, num_hashes);
    }
};


namespace BloomFilter_Test{
    void test(){
        BloomFilter bf(100, 0.01);  // 预期存储 100 个元素，误判率 1%
        // 插入一些元素
        bf.insert("apple");
        bf.insert("banana");
        bf.insert("orange");
        // 查询测试
        std::cout << "Contains 'apple': " << bf.contains("apple") << std::endl;    // 1 (true)
        std::cout << "Contains 'grape': " << bf.contains("grape") << std::endl;    // 0 (false)，但有 1% 概率误判
        std::cout << "False Positive Rate: " << bf.false_positive_rate(3) << std::endl;
    }
};

#endif //CPP_LEARN_BLOOMFILTER_H
