#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vk_gltf_renderer {

class Resource;

class RenderPass {
public:
    RenderPass(const std::string& name);
    virtual ~RenderPass();

    // 获取渲染通道名称
    const std::string& getName() const { return name_; }

    // 添加输入资源
    void addInput(const std::string& name, std::shared_ptr<Resource> resource);
    
    // 添加输出资源
    void addOutput(const std::string& name, std::shared_ptr<Resource> resource);
    
    // 执行渲染通道
    virtual void execute(VkCommandBuffer cmdBuffer) = 0;
    
    // 获取所有输入资源
    const std::unordered_map<std::string, std::shared_ptr<Resource>>& getInputs() const { return inputs_; }
    
    // 获取所有输出资源
    const std::unordered_map<std::string, std::shared_ptr<Resource>>& getOutputs() const { return outputs_; }

protected:
    std::string name_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> inputs_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> outputs_;
};

} // namespace vk_gltf_renderer 