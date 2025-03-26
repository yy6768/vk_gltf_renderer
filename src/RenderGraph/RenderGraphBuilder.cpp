#include "RenderGraphBuilder.h"

namespace vk_gltf_renderer {


RenderGraphBuilder::RenderGraphBuilder() : graph_(std::make_shared<RenderGraph>()) {}
    
RenderGraphBuilder& RenderGraphBuilder::addPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
    graph_->addPass(name, pass);
    return *this;
}
    
RenderGraphBuilder& RenderGraphBuilder::addResource(const std::string& name, std::shared_ptr<Resource> resource) {
    graph_->addResource(name, resource);
    return *this;
}
    
RenderGraphBuilder& RenderGraphBuilder::connect(const std::string& resourceName, const std::string& passName, bool isInput) {
    // TODO: 实现资源与渲染通道的连接逻辑
    return *this;
}
    
std::shared_ptr<RenderGraph> RenderGraphBuilder::build() {
    graph_->build();
    return graph_;
}

} // namespace vk_gltf_renderer 