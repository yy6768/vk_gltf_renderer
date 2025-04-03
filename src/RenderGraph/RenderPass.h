#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "nvvk/renderpasses_vk.hpp"

namespace ame {

class Resource;

struct RenderPassConfig {
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    uint32_t subpassCount = 1;
    bool clearColor = true;
    bool clearDepth = true;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

class RenderPass {
public:
    RenderPass(const std::string& name, 
               VkDevice& device, 
               const RenderPassConfig& config);

    virtual ~RenderPass() = default;


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

    // 获取渲染通道句柄
    VkRenderPass getHandle() const { return renderPass_; }
protected:
    
    std::string name_;
    VkDevice device_;
    VkRenderPass renderPass_;
    RenderPassConfig config_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> inputs_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> outputs_;

    friend class RenderGraph;
};

} // namespace ame 