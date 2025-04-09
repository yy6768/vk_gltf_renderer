#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

#include "nvvk/renderpasses_vk.hpp"

#include "RenderGraph/GraphResource.hpp"
#include "RenderGraph/RenderGraphContext.hpp"

namespace ame {

class GraphResource;

using ResourceMap = GraphResource::ResourceMap;
struct RenderPassConfig {
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    uint32_t subpassCount = 1;
    bool clearColor = true;
    bool clearDepth = true;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

class RenderData {
public:
    /**
     * Get a resource
     * @param[in] name The name of the pass' resource (i.e. "outputColor"). No need to specify the pass' name
     * @return If the name exists, a pointer to the resource. Otherwise, nullptr
     */
    const std::shared_ptr<GraphResource>& operator[](const std::string_view name) const { return getResource(name); }

    /**
     * Get a resource
     * @param[in] name The name of the pass' resource (i.e. "outputColor"). No need to specify the pass' name
     * @return If the name exists, a pointer to the resource. Otherwise, nullptr
     */
    const std::shared_ptr<GraphResource>& getResource(const std::string_view name) const;

    /**
     * Get a texture
     * @param[in] name The name of the pass' texture (i.e. "outputColor"). No need to specify the pass' name
     * @return If the texture exists, a pointer to the texture. Otherwise, nullptr
     */
    const std::shared_ptr<GraphResource>& getTexture(const std::string_view name) const;

    /**
     * Get the default dimensions used for Texture2Ds (when `0` is specified as the dimensions in `RenderPassReflection`)
     */
    const glm::uvec2& getDefaultTextureDims() const { return mDefaultTexDims; }

    /**
     * Get the default format used for Texture2Ds (when `Unknown` is specified as the format in `RenderPassReflection`)
     */
    VkFormat getDefaultTextureFormat() const { return mDefaultTexFormat; }

protected:
    RenderData(
        const std::string& passName,
        const glm::uvec2& defaultTexDims,
        VkFormat defaultTexFormat,
        ResourceMap& resources
    );

    const std::string& mName;
    ResourceMap& mResources;
    glm::uvec2 mDefaultTexDims;
    VkFormat mDefaultTexFormat;
};

class RenderPass {
public:
    RenderPass(const std::string& name, 
               VkDevice& device, 
               const RenderPassConfig& config);

    virtual ~RenderPass() = default;


    // 获取渲染通道名称
    const std::string& getName() const { return name_; }

    // 添加输入资源
    void addInput(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 添加输出资源
    void addOutput(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 执行渲染通道
    virtual void execute(RenderGraphContext& context) = 0;
    
    // 获取所有输入资源
    const ResourceMap& getInputs() const { return inputs_; }
    
    // 获取所有输出资源
    const ResourceMap& getOutputs() const { return outputs_; }

    // 获取渲染通道句柄
    VkRenderPass getHandle() const { return renderPass_; }
protected:
    
    std::string name_;
    VkDevice device_;
    VkRenderPass renderPass_;
    RenderPassConfig config_;
    ResourceMap inputs_;
    ResourceMap outputs_;

    friend class RenderGraph;
};

} // namespace ame 