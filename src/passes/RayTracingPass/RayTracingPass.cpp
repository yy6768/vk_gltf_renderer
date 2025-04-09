#include "RayTracingPass.hpp"

#include <glm/glm.hpp>
#include <stdexcept>

namespace ame {

RayTracingPass::RayTracingPass(VkDevice& device, const RayTracingPassConfig& config)
    : RenderPass("RayTracingPass", device, createRenderPassConfig())
    , m_config(config)
{
    m_debugUtil = std::make_unique<nvvk::DebugUtil>(device);
    m_rtPipeline = std::make_unique<nvvkhl::PipelineContainer>();
    m_sbt = std::make_unique<nvvk::SBTWrapper>();
    m_rtSet = std::make_unique<nvvk::DescriptorSetContainer>(device);

    // 获取光线追踪属性
    VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    prop2.pNext = &m_rtProperties;
    // 在实际应用中需要获取物理设备并查询属性
    // vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);

    // 创建描述符集布局
    createDescriptorSets();
}

RayTracingPass::~RayTracingPass()
{
    cleanup();
}

void RayTracingPass::cleanup()
{
    if (m_sbt)
        m_sbt->destroy();
    
    if (m_rtPipeline)
        m_rtPipeline->destroy(device_);
    
    for (auto& module : m_shaderModules)
    {
        if (module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(device_, module, nullptr);
        }
    }
    m_shaderModules.clear();
}

// 设置场景的顶层加速结构
void RayTracingPass::setTlas(VkAccelerationStructureKHR tlas)
{
    m_tlas = tlas;
    updateDescriptorSets();
}

// 设置渲染范围
void RayTracingPass::setExtent(VkExtent2D extent)
{
    m_extent = extent;
    m_config.width = extent.width;
    m_config.height = extent.height;
}

// 创建描述符集
void RayTracingPass::createDescriptorSets()
{
    m_rtSet->addBinding(RtBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    m_rtSet->addBinding(RtBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    m_rtSet->addBinding(RtBindings::eNormalDepth, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    m_rtSet->addBinding(RtBindings::eSelect, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    m_rtSet->initLayout();
    m_rtSet->initPool(1);
}

// 更新描述符集
void RayTracingPass::updateDescriptorSets()
{
    if (m_tlas == VK_NULL_HANDLE)
        return;

    // 针对每个输出和输入资源，需要更新描述符
    // 这里只是一个示例，实际应用中需要根据真实资源进行设置
    VkWriteDescriptorSetAccelerationStructureKHR descAS{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    descAS.accelerationStructureCount = 1;
    descAS.pAccelerationStructures = &m_tlas;

    // 输出图像的描述符更新
    // 在实际应用中这些应该从GraphResource中获取
    // VkDescriptorImageInfo outImageInfo = outputs_["outImage"]->getImageInfo();
    // VkDescriptorImageInfo normalDepthInfo = outputs_["normalDepth"]->getImageInfo();
    // VkDescriptorImageInfo selectInfo = outputs_["select"]->getImageInfo();

    // 更新描述符集
    // std::vector<VkWriteDescriptorSet> writes;
    // writes.push_back(m_rtSet->makeWrite(0, RtBindings::eTlas, &descAS));
    // writes.push_back(m_rtSet->makeWrite(0, RtBindings::eOutImage, &outImageInfo));
    // writes.push_back(m_rtSet->makeWrite(0, RtBindings::eNormalDepth, &normalDepthInfo));
    // writes.push_back(m_rtSet->makeWrite(0, RtBindings::eSelect, &selectInfo));
    // vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

// 加载着色器
void RayTracingPass::loadShaders()
{
    // 加载光线追踪的着色器
    // 在实际应用中，这些应该从文件中加载
    // 例如：m_shaderModules.push_back(nvvk::createShaderModule(device_, nvh::loadFile("shaders/raygen.rgen.spv", true, nullptr, true)));
}

// 创建光追管线
void RayTracingPass::createRayTracingPipeline()
{
    // 加载着色器
    loadShaders();

    // 在实际应用中，这里需要创建光线追踪管线
    // 包括设置着色器阶段、着色器组和管线布局等
    // ...
}

// 创建输出资源
void RayTracingPass::createOutputResources()
{
    // 在实际应用中，这里需要创建输出资源
    // 例如渲染目标图像、法线深度图等
    // ...
}

// 执行光追渲染
void RayTracingPass::execute(RenderGraphContext& context)
{
    // 在实际应用中，这里需要执行光线追踪渲染
    // 包括绑定描述符集、推送常量和触发光线追踪等
    // ...

    // 示例代码：
    auto cmdBuffer = context.commandBuffer;
    
    if (m_tlas == VK_NULL_HANDLE || m_extent.width == 0 || m_extent.height == 0)
        return;
    
    m_frameCount++;
    
    // 绑定描述符集
    // vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline->plines[0]);
    // vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline->layout, 0, 1, &m_rtSet->getSet(), 0, nullptr);
    
    // 推送常量
    // struct PushConstant {
    //     uint32_t maxDepth;
    //     uint32_t samples;
    //     uint32_t frameCount;
    // } pushConstant;
    // 
    // pushConstant.maxDepth = m_config.maxDepth;
    // pushConstant.samples = m_config.samplesPerPixel;
    // pushConstant.frameCount = m_frameCount;
    // 
    // vkCmdPushConstants(cmdBuffer, m_rtPipeline->layout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstant), &pushConstant);
    
    // 设置着色器绑定表
    // VkStridedDeviceAddressRegionKHR raygenSbt = m_sbt->getRegions()[0];
    // VkStridedDeviceAddressRegionKHR missSbt = m_sbt->getRegions()[1];
    // VkStridedDeviceAddressRegionKHR hitSbt = m_sbt->getRegions()[2];
    // VkStridedDeviceAddressRegionKHR callableSbt = {};
    
    // 执行光线追踪
    // vkCmdTraceRaysKHR(cmdBuffer, &raygenSbt, &missSbt, &hitSbt, &callableSbt, m_extent.width, m_extent.height, 1);
}

// 创建渲染通道配置
RenderPassConfig RayTracingPass::createRenderPassConfig()
{
    RenderPassConfig config;
    config.colorFormats = {VK_FORMAT_R32G32B32A32_SFLOAT};  // 高精度HDR输出
    config.clearColor = true;
    config.finalLayout = VK_IMAGE_LAYOUT_GENERAL;  // 设为GENERAL以便后续处理
    return config;
}

} // namespace ame 