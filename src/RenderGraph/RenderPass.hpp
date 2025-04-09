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
class RenderGraphBuilder;

using ResourceMap = GraphResource::ResourceMap;

enum class RenderPassType: uint8_t {
    Graphics,
	Compute,
	AsyncCompute,
	Copy
};

enum class RenderPassFlags: uint8_t {
    None = 0,
    ClearColor = 1 << 0,
    ClearDepth = 1 << 1,
    ClearColorAndDepth = ClearColor | ClearDepth,
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
    ) : mName(passName), mResources(resources), mDefaultTexDims(defaultTexDims), mDefaultTexFormat(defaultTexFormat) {}

    const std::string& mName;
    ResourceMap& mResources;
    glm::uvec2 mDefaultTexDims;
    VkFormat mDefaultTexFormat;
};

class RenderPass {
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    struct RenderTargetData{
        
    };
    struct DepthStencilData{

    };
public:
    RenderPass(const std::string& name, 
               VkDevice& device, 
               RenderPassType type,
               RenderPassFlags flags = RenderPassFlags::None);

    virtual ~RenderPass() = default;


    // 获取渲染通道名称
    const std::string& getName() const { return name_; }

    // 添加输入资源
    void addInput(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 添加输出资源
    void addOutput(const std::string& name, std::shared_ptr<GraphResource> resource);
    
    // 执行渲染通道
    virtual void execute(RenderGraphContext& context, RenderData& data) = 0;
    
    // 获取所有输入资源
    const ResourceMap& getInputs() const { return inputs_; }
    
    // 获取所有输出资源
    const ResourceMap& getOutputs() const { return outputs_; }

    // 获取资源
    const std::shared_ptr<GraphResource>& getResource(const std::string& name) const;

    // 设置渲染区域大小
    void setExtent(VkExtent2D extent) { extent_ = extent; }
protected:
    
    std::string name_;
    uint32_t ref_count_;
    RenderPassType type_;
    RenderPassFlags flags_ = RenderPassFlags::None;
    VkDevice device_;
    ResourceMap inputs_;
    ResourceMap outputs_;
    VkExtent2D extent_;
    friend class RenderGraph;
    
    inline std::string PassTypeToString() const {
        switch (type_) {
            case RenderPassType::Graphics: return "Graphics";
            case RenderPassType::Compute: return "Compute";
            case RenderPassType::AsyncCompute: return "AsyncCompute";
            case RenderPassType::Copy: return "Copy";
        }
        return "Unknown";
    }
};

} // namespace ame 