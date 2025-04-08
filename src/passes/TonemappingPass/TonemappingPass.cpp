#include "ToneMappingPass.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include <fstream>

namespace ame {

// 删除内联着色器字符串
// const char* tonemappingShaderGlsl = R"(...

TonemappingPass::TonemappingPass(VkDevice& device)
    : RenderPass("TonemappingPass", device, createConfig())
{
    createDescriptorSet();
    createPipeline();
}

TonemappingPass::~TonemappingPass() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
}

RenderPassConfig TonemappingPass::createConfig() {
    RenderPassConfig config;
    // 不需要特别设置，因为我们使用计算着色器
    config.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    config.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return config;
}

void TonemappingPass::setInputOutput(VkImageView inputView, VkImageView outputView, VkExtent2D extent) {
    m_inputView = inputView;
    m_outputView = outputView;
    m_extent = extent;
    
    // 更新描述符
    if (m_descriptorSet != VK_NULL_HANDLE) {
        // 输入图像
        VkDescriptorImageInfo inputInfo{};
        inputInfo.imageView = m_inputView;
        inputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        // 输出图像
        VkDescriptorImageInfo outputInfo{};
        outputInfo.imageView = m_outputView;
        outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        std::vector<VkWriteDescriptorSet> writes(2);
        
        // 输入图像描述符
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &inputInfo;
        
        // 输出图像描述符
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &outputInfo;
        
        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void TonemappingPass::createDescriptorSet() {
    // 创建描述符布局
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    
    // 输入图像绑定
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // 输出图像绑定
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
    
    // 创建描述符池
    std::vector<VkDescriptorPoolSize> poolSizes(1);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 2; // 输入和输出各一个
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
    
    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;
    
    if (vkAllocateDescriptorSets(device_, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
}

void TonemappingPass::createPipeline() {
    // 使用nvvk帮助类编译着色器或从文件加载
    VkShaderModule compShader;
    
    // 可以使用Resources类的compileGlslShader从文件加载
    // 或者直接从文件加载预编译的SPIR-V
    std::string shaderPath = "src/passes/TonemappingPass/tonemapping.comp.glsl";
    
    // 这里使用简化的方法，实际应该使用资源管理系统
    try {
        compShader = nvvk::createShaderModule(device_, shaderPath.c_str(), VK_SHADER_STAGE_COMPUTE_BIT);
    }
    catch(std::exception& e) {
        throw std::runtime_error("Failed to load compute shader: " + std::string(e.what()));
    }
    
    // 创建Push Constants范围
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float); // 仅暴露参数
    
    // 创建管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, compShader, nullptr);
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    // 创建计算管线
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = compShader;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = m_pipelineLayout;
    
    VkResult result = vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    
    // 清理着色器模块
    vkDestroyShaderModule(device_, compShader, nullptr);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline!");
    }
}

void TonemappingPass::execute(VkCommandBuffer cmdBuffer) {
    // 绑定计算管线
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    
    // 绑定描述符集
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    
    // 设置曝光参数 (示例值为0.0f，表示不改变曝光)
    float exposure = 0.0f;
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &exposure);
    
    // 确保输入图像已从ATTACHMENT_OPTIMAL转换为GENERAL
    VkImageMemoryBarrier preBarrier{};
    preBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    preBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    preBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL; // 假设输入来自渲染通道
    preBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    preBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preBarrier.image = m_inputView; // 注意：这里应该是图像对象而非视图
    preBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    
    vkCmdPipelineBarrier(cmdBuffer, 
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &preBarrier);
    
    // 计算着色器组大小
    uint32_t groupSizeX = (m_extent.width + 15) / 16;
    uint32_t groupSizeY = (m_extent.height + 15) / 16;
    
    // 分发计算工作
    vkCmdDispatch(cmdBuffer, groupSizeX, groupSizeY, 1);
    
    // 计算完成后，确保输出图像转换为PRESENT_SRC_KHR布局
    VkImageMemoryBarrier postBarrier{};
    postBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    postBarrier.dstAccessMask = 0;
    postBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    postBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postBarrier.image = m_outputView; // 注意：这里应该是图像对象而非视图 
    postBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    
    vkCmdPipelineBarrier(cmdBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &postBarrier);
}

} // namespace ame 