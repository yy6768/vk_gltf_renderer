#pragma once

#include <string>
namespace ame {
class Field {
public:
    Field(const std::string& name, const std::string& type, const std::string& description);
    ~Field();

private:
    std::string name_;
    std::string type_;
    std::string description_;
};
}
