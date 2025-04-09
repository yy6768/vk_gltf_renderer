#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#include "nvvk/commands_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/buffers_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"

namespace ame {


// 命令列表类型
enum class CommandListType : uint8_t {
    Graphics,
    Compute,
    Transfer
};

// 资源状态（图像布局）
enum class ResourceState : uint32_t {
    Undefined               = VK_IMAGE_LAYOUT_UNDEFINED,
    Common                  = VK_IMAGE_LAYOUT_GENERAL,
    RenderTarget            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    UnorderedAccess         = VK_IMAGE_LAYOUT_GENERAL,
    DepthStencilWrite       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    DepthStencilRead        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    ShaderResource          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    CopyDest                = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    CopySource              = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    Present                 = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    AccelerationStructure   = VK_IMAGE_LAYOUT_GENERAL
};

// 命令列表上下文，表示当前的绑定状态
enum class CommandContext {
    Invalid,
    Graphics,
    Compute,
    Transfer
};

// 渲染通道描述
struct RenderPassDesc {
    std::vector<VkImageView> colorAttachments;
    std::vector<VkImageView> resolveAttachments;
    VkImageView depthStencilAttachment = VK_NULL_HANDLE;
    std::vector<VkClearValue> clearValues;
    VkRect2D renderArea = {{0, 0}, {0, 0}};
    uint32_t layers = 1;
};

// 命令列表类
class CommandList {
public:
    explicit CommandList(VkDevice device, CommandListType type = CommandListType::Graphics, const char* name = "");
    ~CommandList();

    // 删除拷贝构造函数和赋值操作符
    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    // 获取Vulkan原生命令缓冲区
    VkCommandBuffer GetNative() const { return m_cmdBuffer; }

    // 命令列表操作
    void Begin();
    void End();
    void Reset();
    void Submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE);
    void SubmitAndWait(VkQueue queue);
    void SetName(const char* name);

    // 同步和事件
    void WaitForFence(VkFence fence, uint64_t value = 0);
    void SignalFence(VkFence fence, uint64_t value = 0);
    
    // 调试标记
    void BeginDebugLabel(const char* label, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    void EndDebugLabel();
    void InsertDebugLabel(const char* label, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // 渲染通道操作
    void BeginRenderPass(const RenderPassDesc& renderPassDesc, VkFramebuffer framebuffer, VkRenderPass renderPass);
    void EndRenderPass();
    
    // 动态渲染（VK_KHR_dynamic_rendering）
    void BeginDynamicRendering(const RenderPassDesc& renderPassDesc);
    void EndDynamicRendering();

    // 管线操作
    void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
    void BindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
    void BindDescriptorSet(VkPipelineLayout layout, uint32_t set, VkDescriptorSet descriptorSet, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
    
    // 推送常量
    void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data);
    template<typename T>
    void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, const T& data) {
        PushConstants(layout, stageFlags, offset, sizeof(T), &data);
    }
    
    // 顶点和索引缓冲区
    void BindVertexBuffers(uint32_t firstBinding, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets);
    void BindVertexBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset = 0);
    void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    
    // 绘制命令
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);
    void DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    void DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    
    // 计算着色器调度
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ = 1);
    void DispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
    
    // 光线追踪
    void TraceRays(const VkStridedDeviceAddressRegionKHR& raygenShaderBindingTable,
                   const VkStridedDeviceAddressRegionKHR& missShaderBindingTable,
                   const VkStridedDeviceAddressRegionKHR& hitShaderBindingTable,
                   const VkStridedDeviceAddressRegionKHR& callableShaderBindingTable,
                   uint32_t width, uint32_t height, uint32_t depth = 1);

    // 资源屏障和布局转换
    void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, 
                           VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
    void TransitionImageLayout(VkImage image, ResourceState oldState, ResourceState newState,
                           VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
    void BufferBarrier(VkBuffer buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
    void FlushBarriers();

    // 复制操作
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
    void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, 
                        const std::vector<VkBufferImageCopy>& regions);
    void CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, 
                        const std::vector<VkBufferImageCopy>& regions);
    void CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                const std::vector<VkImageCopy>& regions);
    
    // 清除操作
    void ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue& color,
                      const std::vector<VkImageSubresourceRange>& ranges);
    void ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue& value,
                             const std::vector<VkImageSubresourceRange>& ranges);
    void ClearAttachments(const std::vector<VkClearAttachment>& attachments, const std::vector<VkClearRect>& rects);

    // 视口和裁剪矩形
    void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
    void SetViewportAndScissor(float x, float y, float width, float height);

    // 查询
    void BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags = 0);
    void EndQuery(VkQueryPool queryPool, uint32_t query);
    void ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
    void WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
    void CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                           VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);

    // 设置当前上下文
    void SetContext(CommandContext context);
    CommandContext GetContext() const { return m_currentContext; }

    // 添加资源引用，确保资源在命令列表执行期间不会被销毁
    void AddResourceReference(const std::shared_ptr<void>& resource);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    CommandListType m_type;
    CommandContext m_currentContext = CommandContext::Invalid;
    
    // 调试工具
    std::unique_ptr<nvvk::DebugUtil> m_debugUtil;
    
    // 屏障集合
    std::vector<VkImageMemoryBarrier> m_imageBarriers;
    std::vector<VkBufferMemoryBarrier> m_bufferBarriers;
    std::vector<VkMemoryBarrier> m_memoryBarriers;

    // 资源引用，确保在命令列表执行期间资源不会被销毁
    std::vector<std::shared_ptr<void>> m_resourceReferences;
    
    // 当前绑定状态
    VkRenderPass m_currentRenderPass = VK_NULL_HANDLE;
    bool m_inRenderPass = false;
    bool m_inDynamicRendering = false;

    // 辅助函数
    void CreateCommandBuffer();
    void DestroyCommandBuffer();
};

} // namespace ame
