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
    struct EdgeData {
        std::string src;
        std::string dst;
    };

    struct NodeData {
        std::string name;
        std::shared_ptr<RenderPass> pass;
    };

    
};

} // namespace ame 