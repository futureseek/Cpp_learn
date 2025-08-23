//
// Created by 12793 on 2025/8/1.
//

#ifndef CLION_TEST_KNNCLASSIFIER_H
#define CLION_TEST_KNNCLASSIFIER_H

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <random>
#include <iomanip>
#include <chrono>
struct Sample{
    std::vector<double> features;
    int label;

    Sample(std::vector<double> feat,int lab):features(std::move(feat)),label(lab){}
};

class KNNClassifier{
public:


    explicit KNNClassifier(int k=3):K(k){
        if(k<=0){
            throw std::invalid_argument("k is not acc");
        }
    }
    ~KNNClassifier(){
        ;
    }

    void fit(const std::vector<Sample>& data){
        train_data=data;
    }

    int predict(const Sample& sample) const{
        if(train_data.empty()){
            throw std::runtime_error("haven't train_data");
        }
        if(sample.features.empty()){
            throw std::invalid_argument("error features");
        }

        std::vector<std::pair<double,int>> dist_label;
        for(const auto& train_sample:train_data){
            double dist = euclideanDistance(sample,train_sample);
            dist_label.emplace_back(std::make_pair(dist,train_sample.label));
        }

        std::sort(dist_label.begin(),dist_label.end(),[](const auto& a,const auto& b){
            return a.first<b.first;
        });

        std::map<int,int> label_count;
        for(int i=0;i<K;++i){
            label_count[dist_label[i].second]++;
        }

        int best_label = -1;
        int max_count = 0;
        for(const auto& [label,count]:label_count){
            if(count > max_count){
                max_count = count;
                best_label = label;
            }
        }
        return best_label;
    }
    std::vector<int> predict(const std::vector<Sample>& samples) const {
        std::vector<int> results;
        for (const auto& sample : samples) {
            results.push_back(predict(sample));
        }
        return results;
    }

private:
    const int K;
    std::vector<Sample> train_data;


    // 采用欧氏距离
    double euclideanDistance(const Sample& a,const Sample& b) const{
        if(a.features.size()!=b.features.size()){
            throw std::invalid_argument("feature nums error");
        }
        double dist = 0.0;
        for(size_t i=0;i<a.features.size();i++){
            dist += std::pow(a.features[i]-b.features[i],2);
        }
        return std::sqrt(dist);
    }

};
#endif //CLION_TEST_KNNCLASSIFIER_H
