#pragma once

#include "RenderGraph/RenderPass.h"
#include <vulkan/vulkan.h>

namespace ame {

class TonemappingPass : public RenderPass {
public:
    TonemappingPass(VkDevice& device);
    ~TonemappingPass() override;

    // 设置输入和输出图像
    void setInputOutput(VkImageView inputView, VkImageView outputView, VkExtent2D extent);
    
    // 执行tonemapping
    void execute(VkCommandBuffer cmdBuffer) override;

private:
    static RenderPassConfig createConfig();
    void createDescriptorSet();
    void createPipeline();
    
    // 资源
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    
    // 输入输出
    VkImageView m_inputView = VK_NULL_HANDLE;
    VkImageView m_outputView = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
};

} // namespace ame

