#include "RenderGraph.h"
#include <algorithm>

namespace ame {

RenderGraph::RenderGraph() {}

RenderGraph::~RenderGraph() {}

void RenderGraph::addPass(const std::string& name, std::shared_ptr<RenderPass> pass) {
    passes_[name] = pass;
}

void RenderGraph::addResource(const std::string& name, std::shared_ptr<Resource> resource) {
    resources_[name] = resource;
}

void RenderGraph::build() {
    buildExecutionOrder();
}

void RenderGraph::execute(VkCommandBuffer cmdBuffer) {
    for (const auto& passName : executionOrder_) {
        auto it = passes_.find(passName);
        if (it != passes_.end()) {
            it->second->execute(cmdBuffer);
        }
    }
}

void RenderGraph::buildExecutionOrder() {
    // TODO: 实现拓扑排序算法来确定渲染通道的执行顺序
    // 这里需要根据资源依赖关系构建DAG并进行排序1
    
    executionOrder_.clear();
    for (const auto& pair : passes_) {
        executionOrder_.push_back(pair.first);
    }
}

} // namespace ame 