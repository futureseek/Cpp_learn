//
// Created by 12793 on 2025/8/3.
//

#ifndef CLION_TEST_DECISONTREE_H
#define CLION_TEST_DECISONTREE_H


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <memory>
#include <cassert>

namespace DecisionTree_ID3{



    struct Example{
        int age;
        std::string income;
        std::string marital_status;
        std::string label;
    };

    using Dataset = std::vector<Example>;

    class DecisionTree{
    public:
        // 节点类型枚举
        enum class NodeType{
            INTERNAL,
            LEAF
        };

        // 节点结构
        struct Node{
            NodeType type;
            std::string feature;
            std::map<std::string, std::unique_ptr<Node>> children;
            std::optional<std::string> label;
        };

        void fit(const Dataset& data, const std::vector<std::string>& features);
        std::string predict(const Example& example) const;

    private:
        std::unique_ptr<Node> build_tree(const Dataset& data, const std::vector<std::string>& features);
        double entropy(const Dataset& data) const;
        double information_gain(const Dataset& data, const std::string& feature) const;
        std::string choose_best_feature(const Dataset& data, const std::vector<std::string>& features) const;
        std::string get_feature_value(const Example& example, const std::string& feature) const;

        std::unique_ptr<Node> root_;
        std::string predict_helper(const Node* node, const Example& example) const;
    };

// 计算熵
    double DecisionTree::entropy(const Dataset& data) const{
        if(data.empty()) return 0.0;

        std::unordered_map<std::string,int> label_counts;
        for(const auto& ex : data){
            label_counts[ex.label]++;
        }

        double total = static_cast<double>(data.size());
        double entropy_value = 0.0;

        for(const auto& [label, count] : label_counts){
            double p = static_cast<double>(count) / total;
            if(p > 0) { // 避免log(0)
                entropy_value -= p * std::log2(p);
            }
        }
        return entropy_value;
    }

// 获取特征值的辅助函数
    std::string DecisionTree::get_feature_value(const Example& example, const std::string& feature) const{
        if(feature == "income"){
            return example.income;
        }
        else if(feature == "marital_status"){
            return example.marital_status;
        }
        else if(feature == "age"){
            if(example.age < 30){
                return "young";
            }
            else if(example.age >= 30 && example.age < 50){
                return "middle";
            }
            else{
                return "old";
            }
        }
        else{
            throw std::runtime_error("Unknown feature: " + feature);
        }
    }

// 计算信息增益
    double DecisionTree::information_gain(const Dataset& data, const std::string& feature) const{
        double initial_entropy = entropy(data);
        double weighted_entropy = 0.0;

        std::map<std::string, Dataset> split_data;
        for(const auto& ex : data){
            std::string feature_value = get_feature_value(ex, feature);
            split_data[feature_value].push_back(ex);
        }

        for(const auto& [value, subset] : split_data){
            double weight = static_cast<double>(subset.size()) / data.size();
            weighted_entropy += weight * entropy(subset);
        }

        return initial_entropy - weighted_entropy;
    }

// 选择最佳特征
    std::string DecisionTree::choose_best_feature(const Dataset& data, const std::vector<std::string>& features) const{
        double max_gain = -1.0;
        std::string best_feature;

        std::cout << "  Information gains:" << std::endl;
        for(const auto& feature : features){
            double gain = information_gain(data, feature);
            std::cout << "    " << feature << ": " << gain << std::endl;
            if(gain > max_gain){
                max_gain = gain;
                best_feature = feature;
            }
        }
        std::cout << "  Best feature: " << best_feature << " (gain: " << max_gain << ")" << std::endl;
        return best_feature;
    }

    std::unique_ptr<DecisionTree::Node> DecisionTree::build_tree(const Dataset& data, const std::vector<std::string>& features){
        if(data.empty()){
            return nullptr;
        }

        // 统计标签
        std::unordered_map<std::string, int> label_counts;
        for(const auto& ex : data){
            label_counts[ex.label]++;
        }

        // 如果所有样本都属于同一类，创建叶子节点
        if(label_counts.size() == 1){
            auto node = std::make_unique<Node>();
            node->type = NodeType::LEAF;
            node->label = label_counts.begin()->first;
            return node;
        }

        // 如果没有更多特征可用，选择最常见的标签
        if(features.empty()){
            auto most_common_label = std::max_element(label_counts.begin(), label_counts.end(),
                                                      [](const auto& a, const auto& b){
                                                          return a.second < b.second;
                                                      });
            auto node = std::make_unique<Node>();
            node->type = NodeType::LEAF;
            node->label = most_common_label->first;
            return node;
        }

        // 选择最佳特征
        std::string best_feature = choose_best_feature(data, features);
        auto node = std::make_unique<Node>();
        node->type = NodeType::INTERNAL;
        node->feature = best_feature;

        // 调试信息：显示选择的最佳特征
        std::cout << "Selected best feature: " << best_feature << " for " << data.size() << " samples" << std::endl;

        // 按最佳特征分割数据
        std::map<std::string, Dataset> split_data;
        for(const auto& ex : data){
            std::string feature_value = get_feature_value(ex, best_feature);
            split_data[feature_value].push_back(ex);
        }

        // 创建剩余特征列表
        std::vector<std::string> remaining_features;
        for(const auto& feature : features){
            if(feature != best_feature){
                remaining_features.push_back(feature);
            }
        }

        // 递归构建子树
        for(const auto& [value, subset] : split_data){
            node->children[value] = build_tree(subset, remaining_features);
        }

        return node;
    }

    void DecisionTree::fit(const Dataset& data, const std::vector<std::string>& features){
        root_ = build_tree(data, features);
    }

    std::string DecisionTree::predict(const Example& example) const{
        if(!root_){
            throw std::runtime_error("Decision tree has not been trained");
        }
        return predict_helper(root_.get(), example);
    }

    std::string DecisionTree::predict_helper(const Node* node, const Example& example) const{
        if(node->type == NodeType::LEAF){
            return node->label.value();
        }

        const std::string& feature = node->feature;
        std::string feature_value = get_feature_value(example, feature);

        auto it = node->children.find(feature_value);
        if (it == node->children.end()) {
            throw std::runtime_error("Feature value '" + feature_value + "' not found in tree for feature '" + feature + "'");
        }

        return predict_helper(it->second.get(), example);
    }

    void test()
    {
        try {
            Dataset data = {
                    {30, "High", "Single", "Class A"},
                    {35, "Low", "Married", "Class B"},
                    {40, "Medium", "Divorced", "Class A"},
                    {25, "Low", "Single", "Class C"},
                    {50, "High", "Married", "Class B"},
                    {45, "Low", "Divorced", "Class A"}
            };

            std::vector<std::string> features = {"income", "marital_status", "age"};

            // 训练决策树
            std::cout << "Training decision tree..." << std::endl;
            DecisionTree dt;
            dt.fit(data, features);
            std::cout << "Training completed!" << std::endl;

            // 预测新样本
            Example new_example = {30, "High", "Single", ""};
            std::cout << "Predicting for example: Age=" << new_example.age
                      << ", Income=" << new_example.income
                      << ", Marital Status=" << new_example.marital_status << std::endl;

            std::string prediction = dt.predict(new_example);
            std::cout << "Prediction: " << prediction << std::endl;

            // 测试更多样本
            std::vector<Example> test_examples = {
                    {25, "Low", "Single", ""},
                    {45, "High", "Married", ""},
                    {35, "Medium", "Divorced", ""}
            };

            std::cout << "\nTesting additional examples:" << std::endl;
            for(const auto& example : test_examples){
                std::string pred = dt.predict(example);
                std::cout << "Age=" << example.age << ", Income=" << example.income
                          << ", Marital=" << example.marital_status << " -> " << pred << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
    }



}


#endif //CLION_TEST_DECISONTREE_H
