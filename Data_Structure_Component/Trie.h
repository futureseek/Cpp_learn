//
// Created by 12793 on 2025/9/19.
//

#ifndef CPP_LEARN_TRIE_H
#define CPP_LEARN_TRIE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cassert>

class Trie{
private:
    struct TrieNode{
        bool is_end;
        std::unordered_map<char,std::unique_ptr<TrieNode>> children;

        TrieNode():is_end(false){}
    };
    std::unique_ptr<TrieNode> root;

public:
    Trie() : root(std::make_unique<TrieNode>()) {}

    void insert(const std::string& word){
        TrieNode* now = root.get();
        for(auto& c:word){
            if(now->children.find(c)==now->children.end()){
                now->children[c]=std::make_unique<TrieNode>();
            }
            now = now->children[c].get();
        }
        now->is_end = true;
    }

    bool search(const std::string& word) const{
        const TrieNode* now = root.get();
        for(auto& c:word){
            if(now->children.find(c)==now->children.end()){
                return false;
            }
            now = now->children.at(c).get();
        }
        return now->is_end;
    }

    bool deleteHelper(TrieNode* now, const std::string& word, int index) {
        if (index == word.size()) {
            if (!now->is_end) {
                return false;  // 单词不存在
            }
            now->is_end = false;      // 逻辑删除单词
            return now->children.empty();  // 当前节点是否能被物理删除
        }
        char c = word[index];
        if (now->children.find(c) == now->children.end()) {
            return false;
        }
        bool shouldDeleteChild = deleteHelper(now->children[c].get(), word, index + 1);
        if (shouldDeleteChild) {
            now->children.erase(c);
            return !now->is_end && now->children.empty();  // 检查父节点是否能被删除
        }
        return false;  // 父节点不能被删除
    }
    bool deleteWord(const std::string& word) {
        bool wordDeleted = search(word);
        deleteHelper(root.get(), word, 0);
        return wordDeleted;
    }

    std::vector<std::string> getAllWords() const {
        std::vector<std::string> words;
        collectWords(root.get(), "", words);
        return words;
    }
    void collectWords(const TrieNode* node, std::string currentWord, std::vector<std::string>& words) const {
        if (node->is_end) {
            words.push_back(currentWord);
        }
        for (const auto& pair : node->children) {
            collectWords(pair.second.get(), currentWord + pair.first, words);
        }
    }

};


namespace Trie_Test{
    void test(){
        Trie trie;

        // 测试插入和查找功能
        trie.insert("apple");
        trie.insert("app");
        trie.insert("banana");
        trie.insert("band");

        assert(trie.search("apple") == true);
        assert(trie.search("app") == true);
        assert(trie.search("banana") == true);
        assert(trie.search("band") == true);
        assert(trie.search("orange") == false);  // 不存在的单词

        // 测试获取所有单词
        auto allWords = trie.getAllWords();
        assert(allWords.size() == 4);
        assert(std::find(allWords.begin(), allWords.end(), "apple") != allWords.end());
        assert(std::find(allWords.begin(), allWords.end(), "app") != allWords.end());
        assert(std::find(allWords.begin(), allWords.end(), "banana") != allWords.end());
        assert(std::find(allWords.begin(), allWords.end(), "band") != allWords.end());

        // 测试删除功能
        assert(trie.deleteWord("app") == true);
        assert(trie.search("app") == false);  // 已删除
        assert(trie.search("apple") == true); // 前缀仍保留

        // 测试删除不存在的单词
        assert(trie.deleteWord("orange") == false);

        // 测试嵌套删除
        trie.insert("application");
        assert(trie.deleteWord("apple") == true);  // apple是application的前缀，不能完全删除
        assert(trie.search("apple") == false); // 这里已经删除了不存在的单词了

        std::cout << "All Trie tests passed!" << std::endl;
    }  
};

#endif //CPP_LEARN_TRIE_H
