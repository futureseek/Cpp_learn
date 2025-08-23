//
// Created by 12793 on 2025/8/2
//

#ifndef CLION_TEST_CPP_JOSN_H
#define CLION_TEST_CPP_JOSN_H

#include <iostream>
#include <variant>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <cmath>


class JsonSerializer;
class JsonSerializer;

class Json{
public:
    using json_value = std::variant<std::nullptr_t,bool,int64_t,double,std::string,std::map<std::string,Json>,std::vector<Json>>;

    //多种构造方式
    Json():value_(nullptr){}
    Json(std::nullptr_t p):value_(p){}
    Json(bool b):value_(b){}
    Json(int64_t a):value_(a){}
    Json(int a):value_(static_cast<int64_t>(a)){}
    Json(double d):value_(d){}
    Json(const char* s):value_(std::string(s)){}
    Json(std::string s):value_(std::move(s)){}
    Json(std::map<std::string ,Json> mp):value_(std::move(mp)){}
    Json(std::vector<Json> v):value_(std::move(v)){}

    //判断类型
    bool is_null() const {return std::holds_alternative<std::nullptr_t>(value_);}
    bool is_bool() const{ return std::holds_alternative<bool>(value_);}
    bool is_number() const { return std::holds_alternative<int64_t>(value_) || std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_object() const { return std::holds_alternative<std::map<std::string, Json>>(value_); }
    bool is_array() const { return std::holds_alternative<std::vector<Json>>(value_); }
    bool is_int() const { return std::holds_alternative<int64_t>(value_); }
    //获取
    bool get_bool() const{ return std::get<bool>(value_); }
    int64_t get_int() const { return std::get<int64_t>(value_); }
    double get_double() const { return std::get<double>(value_); }
    const std::string& get_string() const { return std::get<std::string>(value_); }
    const std::map<std::string, Json>& get_object() const { return std::get<std::map<std::string, Json>>(value_); }
    const std::vector<Json>& get_array() const { return std::get<std::vector<Json>>(value_); }

    //set
    void set_null() { value_ = nullptr; }
    void set_bool(bool b) { value_ = b; }
    void set_int(int64_t i) { value_ = i; }
    void set_double(double d) { value_ = d; }
    void set_string(std::string s) { value_ = std::move(s); }
    void set_object(std::map<std::string, Json> m) { value_ = std::move(m); }
    void set_array(std::vector<Json> v) { value_ = std::move(v); }

    //重载[]
    Json& operator[](const std::string& key){
        if(!is_object()){
            throw std::runtime_error("Cannot use [] on non-object");
        }
        return std::get<std::map<std::string,Json>>(value_)[key];
    }
    const Json& operator[](const std::string& key) const{
        if (!is_object()) {
            throw std::runtime_error("Cannot use [] on non-object Json");
        }
        const auto& obj = std::get<std::map<std::string, Json>>(value_);
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw std::runtime_error("Key not found in JSON object");
        }
        return it->second;
    }


    Json& operator[](size_t index){
        if(!is_array()){
            throw std::runtime_error("Cannot use [] on non_array Json");
        }
        return std::get<std::vector<Json>>(value_).at(index);
    }
    const Json& operator[](size_t index) const{
        if(!is_array()){
            throw std::runtime_error("Cannot use [] on non_array Json");
        }
        return std::get<std::vector<Json>>(value_).at(index);
    }

    // 重载=
    Json& operator=(std::nullptr_t p){
        value_=p;
        return *this;
    }

    Json& operator=(bool b){
        value_=b;
        return *this;
    }

    Json& operator=(int64_t i){
        value_=i;
        return *this;
    }

    Json& operator=(int i){
        value_=static_cast<int64_t>(i);
        return *this;
    }

    Json& operator=(double d){
        value_=d;
        return *this;
    }
    Json& operator=(const char* s){
        value_=std::string(s);
        return *this;
    }

    Json& operator=(const std::string& s){
        value_=s;
        return *this;
    }

    Json& operator=(std::string&& s){
        value_=std::move(s);
        return *this;
    }

    Json& operator=(const std::map<std::string,Json>& m){
        value_=m;
        return *this;
    }
    Json& operator=(std::map<std::string,Json>&& m){
        value_=std::move(m);
        return *this;
    }
    Json& operator=(const std::vector<Json>& v){
        value_=v;
        return *this;
    }
    Json& operator=(std::vector<Json>&& v){
        value_=std::move(v);
        return *this;
    }

private:
    json_value value_;
};

class JsonParser{
public:
    Json parse(const std::string& str){
        std::istringstream iss(str);
        return parse_value(iss);
    }

private:
    void skip_whitespace(std::istream& iss){
        while(isspace(iss.peek())){
            iss.get();
        }
    }

    Json parse_value(std::istream& iss){
        skip_whitespace(iss);
        char ch = iss.peek();
        switch (ch) {
            case '{':
                return parse_object(iss);
            case '[':
                return parse_array(iss);
            case '"':
                return parse_string(iss);
            case 't':
            case 'f':
                return parse_boolean(iss);
            case 'n':
                return parse_null(iss);
            default:
                return parse_number(iss);
        }
    }

    Json parse_object(std::istream& iss){
        std::map<std::string,Json> obj;
        iss.get(); //get {
        skip_whitespace(iss);
        if(iss.peek()=='}'){
            iss.get();
            return obj;
        }
        while(iss){
            skip_whitespace(iss);
            Json key_json = parse_string(iss);
            std::string key = key_json.get_string();
            skip_whitespace(iss);
            if(iss.get() != ':'){
                throw std::runtime_error("Expected ':' after key in object");
            }
            skip_whitespace(iss);
            Json value = parse_value(iss);
            obj[key] = value;
            skip_whitespace(iss);
            
            char next_char = iss.get();
            if(next_char == '}'){
                return obj;
            }
            else if(next_char == ','){
                continue; // 继续解析下一个键值对
            }
            else{
                throw std::runtime_error("Expected ',' or '}' in object");
            }
        }
        throw std::runtime_error("Unexpected end of input in object");
    }
    Json parse_array(std::istream& iss) {
        std::vector<Json> arr;
        iss.get(); // Consume '['
        skip_whitespace(iss);
        if (iss.peek() == ']') {
            iss.get(); // Consume ']'
            return arr;
        }
        while (iss) {
            skip_whitespace(iss);
            Json value = parse_value(iss);
            arr.push_back(value);
            skip_whitespace(iss);
            
            char next_char = iss.get();
            if (next_char == ']') {
                return arr;
            }
            else if (next_char == ',') {
                continue; // 继续解析下一个元素
            }
            else {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
        }
        throw std::runtime_error("Unexpected end of input in array");
    }
    Json parse_string(std::istream& iss){
        std::string str;
        iss.get(); // get '"'
        while(iss&&iss.peek()!='"'){
            str+=iss.get();
        }
        if(!iss){
            throw std::runtime_error("Unterminated string");
        }
        iss.get();
        return str;
    }
    Json parse_boolean(std::istream& iss){
        std::string token;
        char ch;
        while(iss && (isalpha(iss.peek()))){
            token += iss.get();
        }
        if(token == "true"){
            return true;
        }
        else if(token == "false"){
            return false;
        }
        else{
            throw std::runtime_error("Invalid boolean value");
        }
    }
    Json parse_null(std::istream& iss) {
        std::string token;
        while(iss && isalpha(iss.peek())){
            token += iss.get();
        }
        if (token == "null") {
            return nullptr;
        } else {
            throw std::runtime_error("Invalid null value");
        }
    }
    Json parse_number(std::istream& iss){
        std::string token;
        char ch;
        
        // 处理负号
        if(iss.peek() == '-'){
            token += iss.get();
        }
        
        // 处理整数部分
        while(isdigit(iss.peek())){
            token += iss.get();
        }
        
        // 处理小数点和小数部分
        if(iss.peek() == '.'){
            token += iss.get();
            while(isdigit(iss.peek())){
                token += iss.get();
            }
        }
        
        // 处理科学计数法
        ch = iss.peek();
        if(ch == 'e' || ch == 'E'){
            token += iss.get();
            if (iss.peek() == '+' || iss.peek() == '-') {
                token += iss.get();
            }
            while (isdigit(iss.peek())) {
                token += iss.get();
            }
        }
        
        // 检查是否包含小数点或科学计数法
        bool is_float = token.find('.') != std::string::npos || 
                       token.find('e') != std::string::npos || 
                       token.find('E') != std::string::npos;
        
        if(is_float){
            double d;
            auto [p, ec] = std::from_chars(token.data(), token.data() + token.size(), d);
            if(ec != std::errc()){
                throw std::runtime_error("Invalid number format");
            }
            return d;
        } else {
            int64_t i;
            auto [p, ec] = std::from_chars(token.data(), token.data() + token.size(), i);
            if(ec != std::errc()){
                throw std::runtime_error("Invalid number format");
            }
            return i;
        }
    }
};

class JsonSerializer{
public:
    std::string serialize(const Json& json){
        return serialize_value(json);
    }
private:
    std::string serialize_value(const Json& json){
        if(json.is_null()){
            return "null";
        }
        else if(json.is_bool()){
            return json.get_bool()?"true":"false";
        }
        else if(json.is_number()){
            if(json.is_int()){
                return std::to_string(json.get_int());
            }
            else return std::to_string(json.get_double());
        }
        else if(json.is_string()){
            return serialize_string(json.get_string());
        }
        else if(json.is_object()){
            return serialize_object(json.get_object());
        }
        else if(json.is_array()){
            return serialize_array(json.get_array());
        }
        else{
            throw std::runtime_error("Unknow Json Type");
        }
    }

    std::string serialize_string(const std::string& str){
        std::ostringstream oss;
        oss<<'"';
        //处理特殊字符
        for(char ch:str){
            switch(ch){
                case '"': oss<<"\\\""; break;
                case '\\': oss<<"\\\\"; break;
                case '/': oss<<"\\/"; break;
                case '\b': oss << "\\b";  break;
                case '\f': oss << "\\f";  break;
                case '\n': oss << "\\n";  break;
                case '\r': oss << "\\r";  break;
                case '\t': oss << "\\t";  break;
                default:   oss << ch;     break;
            }
        }
        oss<<'"';
        return oss.str();
    }
    std::string serialize_object(const std::map<std::string,Json>& obj){
        std::ostringstream oss;
        oss<<'{';
        bool first = true;
        for(const auto& pair:obj){
            if(!first){
                oss<<',';
            }
            oss<<serialize_string(pair.first)<<':'<<serialize_value(pair.second);
            first = false;
        }
        oss<<'}';
        return oss.str();
    }
    std::string serialize_array(const std::vector<Json>& arr){
        std::ostringstream oss;
        oss<<'[';
        bool first =true;
        for(const auto& element:arr){
            if(!first){
                oss<<',';
            }
            oss<<serialize_value(element);
            first = false;
        }
        oss<<']';
        return oss.str();
    }
};

namespace CPP_JSON{

void test()
{
    try {
        // 测试简单的JSON字符串
        std::string json_str = R"({"name":"John Doe","age":30,"isAdult":true,"address":null,"numbers":[1,2,3]})";
        
        std::cout << "Original JSON: " << json_str << std::endl;

        JsonParser parser;
        Json json = parser.parse(json_str);
        std::cout << "JSON parsed successfully!" << std::endl;

        JsonSerializer serializer;
        std::string serialized = serializer.serialize(json);
        std::cout << "Parsed and serialized JSON: " << serialized << std::endl;

        // 访问对象中的键
        std::cout << "Name: " << json["name"].get_string() << std::endl;
        std::cout << "Age: " << json["age"].get_int() << std::endl;
        std::cout << "Is Adult: " << (json["isAdult"].get_bool() ? "true" : "false") << std::endl;
        std::cout << "Address: " << (json["address"].is_null() ? "null" : "not null") << std::endl;

        // 访问数组中的索引
        const auto& numbers = json["numbers"];
        std::cout << "Numbers: ";
        for (size_t i = 0; i < numbers.get_array().size(); ++i) {
            std::cout << numbers[i].get_int() << " ";
        }
        std::cout << std::endl;

        // 修改对象中的值
        std::cout << "Modifying values..." << std::endl;
        json["name"] = "Jane Doe";
        std::cout << "Name set to: " << json["name"].get_string() << std::endl;
        json["age"] = 28;
        json["isAdult"] = false;
        json["address"] = "123 Main St";
        std::cout << "Address set to: " << json["address"].get_string() << std::endl;

        // 修改数组中的值
        json["numbers"][1] = 5;

        // 重新序列化
        serialized = serializer.serialize(json);
        std::cout << "Modified and serialized JSON: " << serialized << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}


}




#endif