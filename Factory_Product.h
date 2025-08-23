//
// Created by 12793 on 2025/8/23.
//

#ifndef CLION_TEST_FACTORY_PRODUCT_H
#define CLION_TEST_FACTORY_PRODUCT_H

#include <iostream>

class Shape{
public:
    virtual ~Shape() = default;
    virtual void draw() const = 0; // 纯虚函数
};

class Circle : public Shape{
    void draw() const override{
        std::cout<<"Draw Circle"<<std::endl;
    }
};

class Rectangle : public Shape{
    void draw() const override{
        std::cout<<"Draw Rectangle"<<std::endl;
    }
};

//虚拟工厂
class ShapeFactory{
public:
    virtual ~ShapeFactory() = default;
    virtual Shape* createShape(const std::string& type) const = 0;
};

//具体工厂
class ConcreateShapeFactory : public ShapeFactory{
public:
    Shape* createShape(const std::string& type) const override{
        if(type == "circle"){
            return new Circle();
        }
        else return new Rectangle();
    }
};

namespace Factory_Test{
  void test(){
      ConcreateShapeFactory factory;
      auto circle = factory.createShape("circle");
      auto rectangle = factory.createShape("rectangle");

      circle->draw();
      rectangle->draw();
  }
};




#endif //CLION_TEST_FACTORY_PRODUCT_H
