#include "RenderGraph.hpp"
#include "RenderPass.hpp"
#include "nvh/nvprint.hpp"

namespace ame {

RenderGraph::RenderGraph() {}

RenderGraph::~RenderGraph() {}

void RenderGraph::addPass(std::shared_ptr<RenderPass> pass) {
    if (m_compiled) {
        LOGW("Cannot add pass to compiled RenderGraph");
        return;
    }
    
    m_passes.push_back(pass);
}

void RenderGraph::addResource(const std::string& name, std::shared_ptr<GraphResource> resource) {
    if (m_compiled) {
        LOGW("Cannot add resource to compiled RenderGraph");
        return;
    }
    
    m_resources[name] = resource;
}

void RenderGraph::compile() {
    if (m_compiled) {
        LOGW("RenderGraph already compiled");
        return;
    }
    
    // 构建有向图
    m_graph.clear();
    
    // 创建节点
    std::unordered_map<std::string, uint32_t> passNameToNodeId;
    for (const auto& pass : m_passes) {
        uint32_t nodeId = m_graph.addNode();
        passNameToNodeId[pass->getName()] = nodeId;
    }
    
    // 创建边（基于资源依赖）
    for (size_t i = 0; i < m_passes.size(); i++) {
        const auto& pass = m_passes[i];
        uint32_t passNodeId = passNameToNodeId[pass->getName()];
        
        // 分析输入依赖
        for (const auto& [inputName, inputResource] : pass->getInputs()) {
            // 寻找提供这个资源的Pass
            for (size_t j = 0; j < m_passes.size(); j++) {
                if (i == j) continue; // 跳过自己
                
                const auto& otherPass = m_passes[j];
                uint32_t otherPassNodeId = passNameToNodeId[otherPass->getName()];
                
                for (const auto& [outputName, outputResource] : otherPass->getOutputs()) {
                    if (inputResource == outputResource) {
                        // 找到依赖，创建边
                        m_graph.addEdge(otherPassNodeId, passNodeId);
                        break;
                    }
                }
            }
        }
    }
    
    // 确定执行顺序（拓扑排序）
    m_executionOrder.clear();
    
    // 计算入度
    std::unordered_map<uint32_t, uint32_t> inDegree;
    for (const auto& [passName, nodeId] : passNameToNodeId) {
        inDegree[nodeId] = 0;
    }
    
    for (const auto& [passName, nodeId] : passNameToNodeId) {
        auto node = m_graph.getNode(nodeId);
        for (uint32_t i = 0; i < node.getOutgoingEdgeCount(); i++) {
            uint32_t edgeId = node.getOutgoingEdge(i);
            auto edge = m_graph.getEdge(edgeId);
            inDegree[edge.getDst()]++;
        }
    }
    
    // 拓扑排序队列
    std::vector<uint32_t> queue;
    for (const auto& [nodeId, degree] : inDegree) {
        if (degree == 0) {
            queue.push_back(nodeId);
        }
    }
    
    // 执行拓扑排序
    while (!queue.empty()) {
        uint32_t nodeId = queue.back();
        queue.pop_back();
        
        // 反向查找Pass
        for (const auto& pass : m_passes) {
            if (passNameToNodeId[pass->getName()] == nodeId) {
                m_executionOrder.push_back(pass);
                break;
            }
        }
        
        auto node = m_graph.getNode(nodeId);
        for (uint32_t i = 0; i < node.getOutgoingEdgeCount(); i++) {
            uint32_t edgeId = node.getOutgoingEdge(i);
            auto edge = m_graph.getEdge(edgeId);
            inDegree[edge.getDst()]--;
            
            if (inDegree[edge.getDst()] == 0) {
                queue.push_back(edge.getDst());
            }
        }
    }
    
    // 检查是否有环
    if (m_executionOrder.size() != m_passes.size()) {
        LOGE("Cyclic dependencies detected in RenderGraph");
        m_compiled = false;
        return;
    }
    
    m_compiled = true;
    
    // 输出执行顺序
    LOGI("RenderGraph execution order:");
    for (const auto& pass : m_executionOrder) {
        LOGI("  - %s", pass->getName().c_str());
    }
}

void RenderGraph::execute(uint32_t frameIndex) {
    if (!m_compiled) {
        LOGE("Cannot execute RenderGraph that has not been compiled");
        return;
    }
    
    
    
    // 执行每个通道
    for (const auto& pass : m_passes) {
        
    }
}

void RenderGraph::setExtent(VkExtent2D extent) {
    m_extent = extent;
    
    // 传递给所有渲染通道
    for (auto& pass : m_passes) {
        pass->setExtent(extent);
    }
}

DirectedGraph::Node RenderGraph::getNode(uint32_t nodeId) const {
    return m_graph.getNode(nodeId);
}

DirectedGraph::Edge RenderGraph::getEdge(uint32_t edgeId) const {
    return m_graph.getEdge(edgeId);
}

} // namespace ame 