#pragma once

#include <iostream>

class Engine{
public:
    std::string getStatus() const {
        return "Engine is ready!";
    }
    void run() {
        std::cout << getStatus() << std::endl;
    }
};