#include <glm/glm.hpp>

// 不使用预编译的着色器，我们将创建自己的
// #include "_autogen/raster.vert.glsl.h"
// #include "_autogen/raster.frag.glsl.h"
// #include "_autogen/raster_overlay.frag.glsl.h"

#include <vulkan/vulkan.h>
#include "core/renderer.hpp"
#include "core/resources.hpp"
#include "scene/scene.hpp"
#include "core/settings.hpp"

#include "RenderGraph/RenderPass.hpp"
#include "RenderGraph/GraphResource.hpp"

#include "nvh/timesampler.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/dynamicrendering_vk.hpp"

namespace ame {

extern bool g_forceExternalShaders;

// 自定义三角形着色器的GLSL代码，编译时从字符串编译


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


// Simple temporary implementation of RDG renderer
class RendererRDG : public Renderer
{
public:
  RendererRDG() = default;
  ~RendererRDG() override {
    deinit(*m_resources);
  }

  bool init(Resources& res, Scene& scene) override;

  void deinit(Resources& res) override;

  void render(VkCommandBuffer cmd, Resources& res, Scene& scene, Settings& settings, nvvk::ProfilerVK& profiler) override;

  bool onUI() override;

  void handleChange(Resources& res, Scene& scene) override;

  VkDescriptorImageInfo getOutputImage() const override;


  bool reloadShaders(Resources& res, Scene& scene) override;
private:

  enum ShaderStages {
    eVertex,
    eFragment,
    // Last entry is the number of shaders
    eShaderGroupCount
  };
  Resources* m_resources{nullptr};
  // Scene* m_scene{nullptr};
  VkDevice m_device{VK_NULL_HANDLE};
  VkCommandPool m_commandPool{VK_NULL_HANDLE};
  VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
  std::vector<shaderc::SpvCompilationResult>    m_spvShader{};
  std::array<VkShaderModule, eShaderGroupCount> m_shaderModules{};
  void createPipeline();
  bool initShaders(Resources& res, bool reload);


  std::unique_ptr<TestTrianglePass> m_testTrianglePass;
  std::unique_ptr<nvvk::DebugUtil> m_dutil;
  std::unique_ptr<nvvkhl::PipelineContainer> m_pipelineContainer;
  
};

// Factory function to create the RDG renderer
std::unique_ptr<Renderer> makeRendererRDG()
{
  return std::make_unique<RendererRDG>();
}

bool RendererRDG::init(Resources& res, Scene& scene) {
  m_device = res.ctx.device;
  m_commandPool = res.m_tempCommandPool->getCommandPool();
  m_resources = &res;
  m_dutil = std::make_unique<nvvk::DebugUtil>(m_device);

  // 创建三角形渲染通道
  m_testTrianglePass = std::make_unique<TestTrianglePass>(m_device);

  // 初始化着色器
  if(!initShaders(res, false)) {
    return false;
  }

  // 设置渲染区域尺寸
  m_testTrianglePass->setExtent(res.m_finalImage->getSize());
  
  // 创建管线
  createPipeline();
  
  return true;
}

bool RendererRDG::initShaders(Resources& res, bool reload) {
  nvh::ScopedTimer st(__FUNCTION__);
  
  // 清空结果
  m_shaderModules = {};
  
  if(res.hasGlslCompiler()) {
    // 从文件加载着色器
    m_spvShader.resize(eShaderGroupCount);
    
    // 使用已经创建好的着色器文件
    m_spvShader[eVertex] = res.compileGlslShader("triangle.vert.glsl", 
                                               shaderc_shader_kind::shaderc_vertex_shader);
    
    m_spvShader[eFragment] = res.compileGlslShader("triangle.frag.glsl", 
                                                 shaderc_shader_kind::shaderc_fragment_shader);
    
    for (size_t i = 0; i < m_spvShader.size(); i++) {
      auto& s = m_spvShader[i];
      if(s.GetCompilationStatus() != shaderc_compilation_status_success) {
        LOGE("Error when compiling shaders\n");
        LOGE("Error %s\n", s.GetErrorMessage().c_str());
        return false;
      }
      m_shaderModules[i] = res.createShaderModule(s);
    }
  } else {
    // 如果没有编译器，我们需要从预编译的着色器加载
    // 这部分需要在将着色器预编译为SPIR-V文件后使用，当前示例代码不实现
    LOGE("No GLSL compiler available. Custom shaders require a GLSL compiler.\n");
    return false;
  }
  
  return true;
} 

// Deinitialize the renderer
void RendererRDG::deinit(Resources& res) {
  // 释放资源
  m_testTrianglePass.reset();
  
  // 释放着色器模块
  for (auto& module : m_shaderModules) {
    if (module != VK_NULL_HANDLE) {
      vkDestroyShaderModule(m_device, module, nullptr);
      module = VK_NULL_HANDLE;
    }
  }
}

void RendererRDG::createPipeline() {
  if (!m_testTrianglePass || !m_shaderModules[eVertex] || !m_shaderModules[eFragment]) {
    return;
  }
  
  // 创建渲染管线，如果有深度图像则传递深度视图
  VkImageView depthView = m_resources->m_finalImage->getDepthImageView();
  m_testTrianglePass->createPipeline(m_shaderModules[eVertex], m_shaderModules[eFragment], depthView);
}

void RendererRDG::render(VkCommandBuffer cmd, Resources& res, Scene& scene, Settings& settings, nvvk::ProfilerVK& profiler) {
  // 添加profiler标记
  auto sec = profiler.timeRecurring("Triangle", cmd);
  
  // 获取输出图像视图
  VkImageView colorView = res.m_finalImage->getColorImageView();
  
  // 获取深度视图（如果有的话）
  VkImageView depthView = res.m_finalImage->getDepthImageView();
  
  // 刷新渲染区域尺寸
  m_testTrianglePass->setExtent(res.m_finalImage->getSize());
  
  // 渲染三角形
  m_testTrianglePass->execute(cmd, colorView, depthView);
  
}

VkDescriptorImageInfo RendererRDG::getOutputImage() const {
  if (m_resources) {
    return m_resources->m_finalImage->getDescriptorImageInfo();
  }
  
  VkDescriptorImageInfo emptyInfo{};
  return emptyInfo;
}

bool RendererRDG::onUI() {
  // 暂时没有UI元素
  return false;
}

void RendererRDG::handleChange(Resources& res, Scene& scene) {
  // 处理资源变化
  bool needUpdatePipeline = false;
  
  // 检查swapchain是否已更改
  if (res.hasGBuffersChanged()) {
    // 更新尺寸
    m_testTrianglePass->setExtent(res.m_finalImage->getSize());
  }
}

bool RendererRDG::reloadShaders(Resources& res, Scene& scene) {
  // 释放现有着色器
  for (auto& module : m_shaderModules) {
    if (module != VK_NULL_HANDLE) {
      vkDestroyShaderModule(m_device, module, nullptr);
      module = VK_NULL_HANDLE;
    }
  }
  
  // 重新加载着色器
  if (!initShaders(res, true)) {
    return false;
  }
  
  // 重新创建pipeline
  createPipeline();
  
  return true;
}

} // namespace ame 