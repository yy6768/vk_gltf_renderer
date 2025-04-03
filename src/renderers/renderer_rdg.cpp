#include <glm/glm.hpp>

#include "_autogen/raster.vert.glsl.h"
#include "_autogen/raster.frag.glsl.h"
#include "_autogen/raster_overlay.frag.glsl.h"

#include <vulkan/vulkan.h>
#include "core/renderer.hpp"
#include "core/resources.hpp"
#include "scene/scene.hpp"
#include "core/settings.hpp"

#include "RenderGraph/RenderPass.h"

#include "nvh/timesampler.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
namespace ame {

extern bool g_forceExternalShaders;

class TestTrianglePass : public RenderPass {
public:
    TestTrianglePass(VkDevice& device)
        : RenderPass("TestTrianglePass", device, createConfig())
    {}

    void execute(VkCommandBuffer cmdBuffer) override {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass_;
        renderPassInfo.framebuffer = framebuffer_;  // 需要创建
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent_;  // 需要设置

        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // 绑定pipeline（需要创建）
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        
        // 绘制一个三角形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmdBuffer);
    }

private:
    static RenderPassConfig createConfig() {
        RenderPassConfig config;
        config.colorFormats = {VK_FORMAT_B8G8R8A8_UNORM};  // 使用swapchain格式
        config.clearColor = true;
        return config;
    }

    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkExtent2D extent_ = {0, 0};
};


// Simple temporary implementation of RDG renderer
class RendererRDG : public Renderer
{
public:
  RendererRDG() = default;
  ~RendererRDG() override = default;

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
    eFragmentOverlay,
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
  void createFramebuffer();
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

  // init shaders
  if(!initShaders(res, false)) {
    return false;
  }

  createFramebuffer();
  createPipeline();
  return true;
}

bool RendererRDG::initShaders(Resources& res, bool reload) {
  nvh::ScopedTimer st(__FUNCTION__);
  if(res.hasGlslCompiler() && (reload || g_forceExternalShaders)) {
    // 加载顶点着色器
    m_spvShader.resize(2);
    m_spvShader[0] = res.compileGlslShader("triangle.vert.glsl", shaderc_shader_kind::shaderc_vertex_shader);
    // 加载片段着色器
    m_spvShader[1] = res.compileGlslShader("triangle.frag.glsl", shaderc_shader_kind::shaderc_fragment_shader);
    for (size_t i = 0; i < m_spvShader.size(); i++) {
      auto& s = m_spvShader[i];
      if(s.GetCompilationStatus() != shaderc_compilation_status_success) {
        LOGE("Error when loading shaders\n");
        LOGE("Error %s\n", s.GetErrorMessage().c_str());
      }
      m_shaderModules[i] = nvvk::createShaderModule(m_device, s);
    }
  } else {
    const auto& vert_shd = std::vector<uint32_t>{std::begin(raster_vert_glsl), std::end(raster_vert_glsl)};
    const auto& frag_shd = std::vector<uint32_t>{std::begin(raster_frag_glsl), std::end(raster_frag_glsl)};
    const auto& overlay_shd = std::vector<uint32_t>{std::begin(raster_overlay_frag_glsl), std::end(raster_overlay_frag_glsl)};

    m_shaderModules[eVertex]          = nvvk::createShaderModule(m_device, vert_shd);
    m_shaderModules[eFragment]        = nvvk::createShaderModule(m_device, frag_shd);
    m_shaderModules[eFragmentOverlay] = nvvk::createShaderModule(m_device, overlay_shd);
  }

} 

// Deinitialize the renderer
void RendererRDG::deinit(Resources& res) {
  // 释放资源
  vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
}


void RendererRDG::createFramebuffer() {
  // 创建framebuffer
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = m_testTrianglePass->getHandle();
}

void RendererRDG::createPipeline() {
  // 创建pipeline
  VkPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO;
  pipelineInfo.renderPass = m_testTrianglePass->getHandle();
}


void RendererRDG::render(VkCommandBuffer cmd, Resources& res, Scene& scene, Settings& settings, nvvk::ProfilerVK& profiler) {
  // 渲染
  m_testTrianglePass->execute(cmd);
}




} // namespace ame 