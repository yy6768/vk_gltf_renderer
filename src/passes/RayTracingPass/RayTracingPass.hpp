#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

#include "RenderGraph/RenderPass.hpp"
#include "RenderGraph/GraphResource.hpp"

#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvk/sbtwrapper_vk.hpp"
#include "nvvkhl/pipelines_vk.hpp"
#include "nvvkhl/gbuffer_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/debug_util_vk.hpp"

namespace ame {

// 光追渲染通道的配置
struct RayTracingPassConfig {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t maxDepth = 5;     // 光线最大弹射次数
    uint32_t samplesPerPixel = 1; // 每像素采样数
    bool denoising = false;    // 是否进行降噪
};

// 光追绑定集布局
enum RtBindings {
    eTlas = 0,       // 顶层加速结构
    eOutImage,       // 输出图像
    eNormalDepth,    // 法线深度图
    eSelect,         // 选择图像
    eBindingsCount
};

// 光追渲染通道
class RayTracingPass : public RenderPass {
public:
    RayTracingPass(VkDevice& device, const RayTracingPassConfig& config = {});
    ~RayTracingPass() override;

    // 执行光追渲染
    void execute(RenderGraphContext& context) override;

    // 设置场景的顶层加速结构
    void setTlas(VkAccelerationStructureKHR tlas);

    // 设置渲染范围
    void setExtent(VkExtent2D extent);

    // 设置最大深度
    void setMaxDepth(uint32_t depth) { m_config.maxDepth = depth; }

    // 设置每像素采样数
    void setSamplesPerPixel(uint32_t samples) { m_config.samplesPerPixel = samples; }

    // 启用或禁用降噪
    void setDenoising(bool enable) { m_config.denoising = enable; }

private:
    // 创建光追管线和着色器绑定表
    void createRayTracingPipeline();
    
    // 创建描述符集
    void createDescriptorSets();
    
    // 更新描述符集
    void updateDescriptorSets();
    
    // 加载着色器
    void loadShaders();
    
    // 创建输出资源
    void createOutputResources();
    
    // 清理资源
    void cleanup();

    static RenderPassConfig createRenderPassConfig();

    // 配置
    RayTracingPassConfig m_config;
    
    // 渲染范围
    VkExtent2D m_extent{0, 0};
    
    // 光追资源
    VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
    
    // 光追管线
    std::unique_ptr<nvvkhl::PipelineContainer> m_rtPipeline;
    
    // 着色器绑定表
    std::unique_ptr<nvvk::SBTWrapper> m_sbt;
    
    // 描述符集
    std::unique_ptr<nvvk::DescriptorSetContainer> m_rtSet;
    
    // 调试工具
    std::unique_ptr<nvvk::DebugUtil> m_debugUtil;
    
    // 光追属性
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    
    // 着色器模块
    std::vector<VkShaderModule> m_shaderModules;
    
    // 帧计数
    uint32_t m_frameCount = 0;
};

} // namespace ame 