#pragma once

#include "RenderGraph.hpp"
#include "RenderPass.hpp"
#include "GraphResource.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ame {

// 渲染图构建器
class RenderGraphBuilder {
public:
    RenderGraphBuilder();
    ~RenderGraphBuilder() = default;
    
    // 添加渲染通道
    RenderGraphBuilder& addPass(std::shared_ptr<RenderPass> pass);
    
    // 添加资源
    RenderGraphBuilder& addResource(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 设置输入资源
    RenderGraphBuilder& setInput(const std::string& passName, const std::string& inputName, const std::string& resourceName);
    
    // 设置输出资源
    RenderGraphBuilder& setOutput(const std::string& passName, const std::string& outputName, const std::string& resourceName);
    
    // 连接两个通道
    RenderGraphBuilder& connect(
        const std::string& outputPassName, 
        const std::string& outputName, 
        const std::string& inputPassName, 
        const std::string& inputName
    );
    
    // 设置外部输入资源（从外部进入RenderGraph的资源）
    RenderGraphBuilder& setExternalInput(const std::string& passName, const std::string& inputName, std::shared_ptr<GraphResource> resource);
    
    // 设置外部输出资源（从RenderGraph输出到外部的资源）
    RenderGraphBuilder& setExternalOutput(const std::string& passName, const std::string& outputName, std::shared_ptr<GraphResource> resource);
    
    // 设置渲染区域大小
    RenderGraphBuilder& setExtent(VkExtent2D extent);
    
    // 构建渲染图
    std::shared_ptr<RenderGraph> build();
    
    // 使用回调函数构建渲染图，可以实现链式调用
    std::shared_ptr<RenderGraph> build(std::function<void(RenderGraphBuilder&)> setup);

private:
    std::shared_ptr<RenderGraph> m_graph;
    std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_passes;
    std::unordered_map<std::string, std::shared_ptr<GraphResource>> m_resources;
    VkExtent2D m_extent{0, 0};
};

} // namespace ame 