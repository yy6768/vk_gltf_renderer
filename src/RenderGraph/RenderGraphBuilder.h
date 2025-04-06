#pragma once

#include "RenderGraph.h"
#include "RenderPass.h"
#include "Resource.h"
#include <string>
#include <memory>

namespace ame {

class RenderGraphBuilder {
public:
    RenderGraphBuilder();
    
    // // 添加渲染通道
    // RenderGraphBuilder& addPass(const std::string& name, std::shared_ptr<RenderPass> pass);
    
    // // 添加资源
    // RenderGraphBuilder& addResource(const std::string& name, std::shared_ptr<Resource> resource);
    
    // // 连接资源到渲染通道
    // RenderGraphBuilder& connect(const std::string& resourceName, const std::string& passName, bool isInput);
    
    // // 构建渲染图
    // std::shared_ptr<RenderGraph> build();

private:
    std::shared_ptr<RenderGraph> graph_;
};

} // namespace ame 