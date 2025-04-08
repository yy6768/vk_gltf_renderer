#pragma once

#include "RenderGraph/RenderPass.hpp"
#include "RenderGraph/RenderGraph.hpp"

namespace ame {

class TestTrianglePass : public RenderPass {
public:
    TestTrianglePass(VkDevice& device)
        : RenderPass("TestTrianglePass", device, createConfig())
    {}

    void execute(VkCommandBuffer cmdBuffer, VkImageView colorView, VkImageView depthView) override {
        // 使用动态渲染API而不是传统的FrameBuffer
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {0.0f, 0.0f, 0.4f, 1.0f}; // 深蓝色背景

        VkRenderingAttachmentInfo depthAttachment{};
        if (depthView != VK_NULL_HANDLE) {
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = depthView;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil = {1.0f, 0};
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, extent_};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        if (depthView != VK_NULL_HANDLE) {
            renderingInfo.pDepthAttachment = &depthAttachment;
        }

        // 开始动态渲染
        vkCmdBeginRendering(cmdBuffer, &renderingInfo);
        
        // 设置视口和裁剪矩形
        VkViewport viewport{};
        viewport.width = static_cast<float>(extent_.width);
        viewport.height = static_cast<float>(extent_.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.extent = extent_;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
        
        // 绑定pipeline
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        
        // 绘制一个三角形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

        // 结束动态渲染
        vkCmdEndRendering(cmdBuffer);
    }

    // 设置渲染区域尺寸
    void setExtent(VkExtent2D extent) {
        extent_ = extent;
    }

    // 创建渲染管线
    void createPipeline(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule, VkImageView depthView = VK_NULL_HANDLE) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }

        // 创建管线布局 - 不需要Push Constants
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;  // 不使用Push Constants
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        
        VkPipelineLayout pipelineLayout;
        VkResult result = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // 使用Dynamic Rendering创建管线
        std::vector<VkFormat> colorFormats = config_.colorFormats;
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        renderingInfo.pColorAttachmentFormats = colorFormats.data();
        renderingInfo.depthAttachmentFormat = config_.depthFormat;

        // 创建图形管线状态
        nvvk::GraphicsPipelineState pipelineState;
        
        // 顶点输入状态 - 没有顶点输入（三角形硬编码在shader中）
        pipelineState.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        
        // 视口状态
        pipelineState.viewportState.viewportCount = 1;
        pipelineState.viewportState.scissorCount = 1;
        
        // 配置颜色混合状态
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        pipelineState.colorBlendState.attachmentCount = 1;
        pipelineState.colorBlendState.pAttachments = &colorBlendAttachment;
        
        // 光栅化状态
        pipelineState.rasterizationState.cullMode = VK_CULL_MODE_NONE;
        pipelineState.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineState.rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        pipelineState.rasterizationState.lineWidth = 1.0f;

        // 动态状态
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        pipelineState.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        pipelineState.dynamicState.pDynamicStates = dynamicStates.data();
        
        // 多重采样状态
        pipelineState.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 深度模板状态
        pipelineState.depthStencilState.depthTestEnable = depthView != VK_NULL_HANDLE;
        pipelineState.depthStencilState.depthWriteEnable = depthView != VK_NULL_HANDLE;
        pipelineState.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
        pipelineState.depthStencilState.stencilTestEnable = VK_FALSE;
        
        // 着色器阶段
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        shaderStages.push_back(vertShaderStageInfo);
        
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        shaderStages.push_back(fragShaderStageInfo);

        // 创建图形管线
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo; // 使用动态渲染信息
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &pipelineState.vertexInputState;
        pipelineInfo.pInputAssemblyState = &pipelineState.inputAssemblyState;
        pipelineInfo.pViewportState = &pipelineState.viewportState;
        pipelineInfo.pRasterizationState = &pipelineState.rasterizationState;
        pipelineInfo.pMultisampleState = &pipelineState.multisampleState;
        pipelineInfo.pDepthStencilState = &pipelineState.depthStencilState;
        pipelineInfo.pColorBlendState = &pipelineState.colorBlendState;
        pipelineInfo.pDynamicState = &pipelineState.dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        // 创建图形管线
        result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(device_, pipelineLayout, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        // 清理管线布局
        vkDestroyPipelineLayout(device_, pipelineLayout, nullptr);
    }

    // 清理资源
    ~TestTrianglePass() override {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
    }

private:
    static RenderPassConfig createConfig() {
        RenderPassConfig config;
        config.colorFormats = {VK_FORMAT_R8G8B8A8_UNORM};  // 使用swapchain格式
        config.clearColor = true;
        config.finalLayout = VK_IMAGE_LAYOUT_GENERAL;  // 修改为GENERAL供后续tonemapper使用
        return config;
    }

    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkExtent2D extent_ = {0, 0};
};




}
