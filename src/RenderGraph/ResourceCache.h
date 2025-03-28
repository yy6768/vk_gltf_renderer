#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include "Resource.h"

namespace ame {

class ResourceCache {
public:
    ResourceCache();
    ~ResourceCache();

    std::shared_ptr<Resource> getResource(const std::string& name);

private:
    std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
};


}


