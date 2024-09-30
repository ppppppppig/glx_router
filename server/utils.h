#pragma once

template <typename T>
class Singleton {
public:
    static T& Get() {
        static T instance; // 声明一个静态实例，确保只创建一次
        return instance;
    }

    // 删除构造函数、拷贝构造函数和赋值操作符
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    Singleton() {} // 私有构造函数，防止直接实例化
};
