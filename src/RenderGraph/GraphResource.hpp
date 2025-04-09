#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace ame {

/**
 * @brief Render Graph Resource
 * 
 */
class GraphResource {
public:
    using ResourceMap = std::unordered_map<std::string, std::shared_ptr<GraphResource>>;

    enum class Type {
        Buffer = 0x0, // Buffer 类型
        Image = 0x1, // Image 类型
        Sampler = 0x2, // Sampler 类型
        DescriptorSet = 0x3 // DescriptorSet 类型
    };

    GraphResource(const std::string& name, Type type);
    virtual ~GraphResource();

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