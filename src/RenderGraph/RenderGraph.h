#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "nvh/nvprint.hpp"

namespace ame {

class RenderPass;
class Resource;

class DirectedGraph {
public:
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max() - 1;

    class Node;
    class Edge;

    uint32_t addNode() {
        mNodes[mCurrentNodeIndex] = Node();
        return mCurrentNodeIndex++;
    }

    std::unordered_set<uint32_t> removeNode(uint32_t index) {
        if (mNodes.find(index) == mNodes.end())
        {
            LOGW("Can't remove node from DirectGraph, node ID doesn't exist");
            return {};
        }
        // Remove all edges connected to the node
        std::unordered_set<uint32_t> removedEdges;
        for (auto& edge : mEdges) {
            if (edge.second.src == index || edge.second.dst == index) {
                removedEdges.insert(edge.first);
            }
        }
        mNodes.erase(index);
        return removedEdges;
    }

    uint32_t addEdge(uint32_t src, uint32_t dst) {
        mEdges[mCurrentEdgeIndex] = Edge(src, dst);
        return mCurrentEdgeIndex++;
    }
    void removeNode(uint32_t index);
    void removeEdge(uint32_t src, uint32_t dst);
    void clear();
    bool isCyclic() const;
    
    DirectedGraph();
    ~DirectedGraph();
    
private:
    std::unordered_map<uint32_t, Node> mNodes;
    std::unordered_map<uint32_t, Edge> mEdges;
    uint32_t mCurrentNodeIndex = 0;
    uint32_t mCurrentEdgeIndex = 0;
};


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

} // namespace ame 