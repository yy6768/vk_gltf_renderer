#pragma once
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>
#include <memory>

#include "nvvk/commands_vk.hpp"

#include "core/resources.hpp"
#include "RenderGraph/GraphResource.hpp"
#include "RenderGraph/RenderGraph.hpp"

namespace ame {

class RenderGraphContext {
    using ResourceMap = GraphResource::ResourceMap;
public:
    RenderGraphContext() = delete;
    RenderGraphContext( RenderGraphContext  const & ) = delete;
	RenderGraphContext( RenderGraphContext  && ) = delete;
	RenderGraphContext & operator=( RenderGraphContext  const & ) = delete;
	RenderGraphContext & operator=( RenderGraphContext  && ) = delete;
   
    void beginFrame();
    void endFrame();

    const VulkanInfo& getVulkanInfo() const;
private:
    RenderGraphContext(RenderGraph& graph, Resources& resources);
    uint32_t frameIndex;
    // 渲染区域信息
    VkExtent2D extent;
    
    // 全局资源表 - 允许Pass之间共享资源
    ResourceMap resources;
    
    // 添加资源
    void addResource(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 获取资源
    std::shared_ptr<GraphResource> getResource(const std::string& name);
};


}
