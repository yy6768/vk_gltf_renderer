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

namespace ame {

class RenderPass;
class Resource;


class RenderGraph {
public:
    static constexpr uint32_t kInvalidIndex = -1;
    RenderGraph();
    ~RenderGraph();

     // 添加渲染通道
    void addPass(std::shared_ptr<RenderPass> pass);
    
    // 添加资源
    void addResource(const std::string& name, std::shared_ptr<Resource> resource);
    
    // 编译渲染图 - 分析依赖，确定执行顺序
    void compile();
    
    // 执行渲染图
    void execute(uint32_t frameIndex);
    
    // 设置渲染区域尺寸
    void setExtent(VkExtent2D extent);

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
    
    RenderGraphContext m_context;
    DirectedGraph m_graph;
    std::vector<std::shared_ptr<RenderPass>> m_passes;
    std::vector<std::shared_ptr<RenderPass>> m_executionOrder;
    bool m_compiled = false;
    
};

} // namespace ame 