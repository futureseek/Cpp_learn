//
// Created by 12793 on 2025/8/25.
//

#ifndef CPP_LEARN_DECORATOR_H
#define CPP_LEARN_DECORATOR_H

// 抽象组件- 饮品接口
class Beverage{
public:
    virtual ~Beverage() = default;
    virtual std::string getDescription() const = 0;
    virtual double cost() const = 0;
};
// 具体组件
class Coffee : public Beverage{
public:
    std::string getDescription() const override{
        return "coffee";
    }

    double cost() const override{
        return 15.0;
    }
};

class Tea : public  Beverage{
public:
    std::string getDescription() const override{
        return "tea";
    }

    double cost() const override{
        return 10.0;
    }
};

// 抽象装饰器
class BeverageDecorator:public Beverage{
protected:
    std::shared_ptr<Beverage> beverage_;
public:
    BeverageDecorator(std::shared_ptr<Beverage> beverage):beverage_(beverage){}
    std::string getDescription() const override{
        return beverage_->getDescription();
    }
    double cost() const override{
        return beverage_->cost();
    }
};
// 具体装饰器
class Milk : public BeverageDecorator{
public:
    Milk(std::shared_ptr<Beverage> beverage):BeverageDecorator(beverage){}
    std::string getDescription() const override {
        return beverage_->getDescription() + "and milk";
    }

    double cost() const override {
        return beverage_->cost() + 3.0;
    }
};

class Sugar : public BeverageDecorator {
public:
    Sugar(std::shared_ptr<Beverage> beverage)
            : BeverageDecorator(beverage) {}

    std::string getDescription() const override {
        return beverage_->getDescription() + "and sugar";
    }

    double cost() const override {
        return beverage_->cost() + 1.0;
    }
};

class Chocolate : public BeverageDecorator {
public:
    Chocolate(std::shared_ptr<Beverage> beverage)
            : BeverageDecorator(beverage) {}

    std::string getDescription() const override {
        return beverage_->getDescription() + "and chocolate";
    }

    double cost() const override {
        return beverage_->cost() + 5.0;
    }
};

namespace Decorator_Test{
    void test() {
        // 基础咖啡
        auto coffee = std::make_shared<Coffee>();
        std::cout << coffee->getDescription() << ": " << coffee->cost() << std::endl;

        // 加牛奶的咖啡
        auto milkCoffee = std::make_shared<Milk>(coffee);
        std::cout << milkCoffee->getDescription() << ": " << milkCoffee->cost() << std::endl;

        // 加牛奶和糖的咖啡
        auto milkSugarCoffee = std::make_shared<Sugar>(milkCoffee);
        std::cout << milkSugarCoffee->getDescription() << ": " << milkSugarCoffee->cost() << std::endl;

        // 加牛奶、糖和巧克力的咖啡
        auto deluxeCoffee = std::make_shared<Chocolate>(milkSugarCoffee);
        std::cout << deluxeCoffee->getDescription() << ": " << deluxeCoffee->cost() << std::endl;

        // 基础茶
        auto tea = std::make_shared<Tea>();
        std::cout << tea->getDescription() << ": " << tea->cost() << std::endl;

        // 加糖的茶
        auto sugarTea = std::make_shared<Sugar>(tea);
        std::cout << sugarTea->getDescription() << ": " << sugarTea->cost() << std::endl;
    }
};


#endif //CPP_LEARN_DECORATOR_H
