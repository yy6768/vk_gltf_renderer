#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>

namespace ame {

class Resource {
public:
    enum class Type {
        Buffer,
        Image,
        Sampler,
        DescriptorSet
    };

    Resource(const std::string& name, Type type);
    virtual ~Resource();

    // 获取资源名称
    const std::string& getName() const { return name_; }
    
    // 获取资源类型
    Type getType() const { return type_; }
    
    // 获取Vulkan资源句柄
    virtual VkDeviceMemory getMemory() const = 0;
    
    // 获取资源大小
    virtual VkDeviceSize getSize() const = 0;
    
    // 获取资源状态
    virtual VkImageLayout getImageLayout() const = 0;
    
    // 设置资源状态
    virtual void setImageLayout(VkImageLayout layout) = 0;

protected:
    std::string name_;
    Type type_;
};

} // namespace ame 