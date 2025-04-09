#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <limits>

#include "nvh/nvprint.hpp"

#include "DirectedGraph.hpp"
#include "RenderGraphContext.hpp"

#include "core/commandlists.hpp"
#include "core/resources.hpp"
#include "vk/vk_context.hpp"

namespace ame {

class RenderPass;
class GraphResource;


class RenderGraph {
friend class RenderGraphBuilder;
friend class RenderGraphContext;
struct ExecutionContext{
    std::unique_ptr<VulkanContext> vulkan_context;
    std::unique_ptr<CommandList> graphics_cmd;
    std::unique_ptr<CommandList> compute_cmd;
    std::unique_ptr<CommandList> transfer_cmd;

};
public:
    static constexpr uint32_t kInvalidIndex = -1;
    RenderGraph();
    ~RenderGraph();

    // 添加渲染通道
    void addPass(std::shared_ptr<RenderPass> pass);
    
    // 添加资源
    void addResource(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 编译渲染图 - 分析依赖，确定执行顺序
    void compile();
    
    // 执行渲染图
    void execute(uint32_t frameIndex);
    
    // 设置渲染区域尺寸
    void setExtent(VkExtent2D extent);
    
    // 获取节点
    DirectedGraph::Node getNode(uint32_t nodeId) const;
    
    // 获取边
    DirectedGraph::Edge getEdge(uint32_t edgeId) const;

private:
    // 边数据
    struct EdgeData {
        std::string src;
        std::string dst;
    };
    // 节点数据
    struct NodeData {
        std::string name;
        std::shared_ptr<RenderPass> pass;
    };
    // 图输出
    struct GraphOutput {
        uint32_t nodeId = kInvalidIndex;
        std::string field;
        std::unordered_set<VkFormat> formats;
    };
    
    DirectedGraph m_graph;
    std::vector<std::shared_ptr<RenderPass>> m_passes;
    std::vector<std::shared_ptr<RenderPass>> m_executionOrder;
    std::unordered_map<std::string, std::shared_ptr<GraphResource>> m_resources;
    VkExtent2D m_extent{0, 0};
    bool m_compiled = false;
    
};

} // namespace ame 