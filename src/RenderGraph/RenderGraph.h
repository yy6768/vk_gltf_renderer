#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vk_gltf_renderer {

class RenderPass;
class Resource;

class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    // 添加渲染通道
    void addPass(const std::string& name, std::shared_ptr<RenderPass> pass);
    
    // 添加资源
    void addResource(const std::string& name, std::shared_ptr<Resource> resource);
    
    // 构建渲染图
    void build();
    
    // 执行渲染图
    void execute(VkCommandBuffer cmdBuffer);

private:
    std::unordered_map<std::string, std::shared_ptr<RenderPass>> passes_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> resources_;
    
    // 执行顺序
    std::vector<std::string> executionOrder_;
    
    // 构建执行顺序
    void buildExecutionOrder();
};

} // namespace vk_gltf_renderer 