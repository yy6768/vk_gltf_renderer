#include "commandlists.hpp"
#include "nvh/nvprint.hpp"

namespace ame {

CommandList::CommandList(VkDevice device, CommandListType type, const char* name)
    : m_device(device)
    , m_type(type)
    , m_currentContext(CommandContext::Invalid)
{
    CreateCommandBuffer();
    m_debugUtil = std::make_unique<nvvk::DebugUtil>(device);
    
    if (name && *name) {
        SetName(name);
    }
}

CommandList::~CommandList() {
    DestroyCommandBuffer();
}

void CommandList::CreateCommandBuffer() {
    // 创建命令池
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    
    // 根据命令列表类型设置队列族
    switch (m_type) {
        case CommandListType::Graphics:
            poolInfo.queueFamilyIndex = 0; // 图形队列索引，实际应用中应动态获取
            break;
        case CommandListType::Compute:
            poolInfo.queueFamilyIndex = 0; // 计算队列索引，实际应用中应动态获取
            break;
        case CommandListType::Transfer:
            poolInfo.queueFamilyIndex = 0; // 传输队列索引，实际应用中应动态获取
            break;
    }
    
    // 允许单独重置命令缓冲区并优化短期命令缓冲区
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    
    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create command pool: %d", result);
        return;
    }
    
    // 分配命令缓冲区
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    result = vkAllocateCommandBuffers(m_device, &allocInfo, &m_cmdBuffer);
    if (result != VK_SUCCESS) {
        LOGE("Failed to allocate command buffer: %d", result);
    }
}

void CommandList::DestroyCommandBuffer() {
    if (m_cmdBuffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBuffer);
        m_cmdBuffer = VK_NULL_HANDLE;
    }
    
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
    
    // 清除资源引用
    m_resourceReferences.clear();
}

void CommandList::SetName(const char* name) {
    if (m_debugUtil && name && *name) {
        m_debugUtil->setObjectName(m_cmdBuffer, name);
    }
}

void CommandList::Begin() {
    // 重置命令缓冲区
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // 指示命令缓冲区将只提交一次
    
    VkResult result = vkBeginCommandBuffer(m_cmdBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        LOGE("Failed to begin command buffer: %d", result);
    }
    
    // 设置上下文
    switch (m_type) {
        case CommandListType::Graphics:
            m_currentContext = CommandContext::Graphics;
            break;
        case CommandListType::Compute:
            m_currentContext = CommandContext::Compute;
            break;
        case CommandListType::Transfer:
            m_currentContext = CommandContext::Transfer;
            break;
    }
    
    // 清理资源引用和屏障
    m_resourceReferences.clear();
    m_imageBarriers.clear();
    m_bufferBarriers.clear();
    m_memoryBarriers.clear();
}

void CommandList::End() {
    // 确保所有挂起的屏障都被刷新
    FlushBarriers();
    
    // 结束命令缓冲区记录
    VkResult result = vkEndCommandBuffer(m_cmdBuffer);
    if (result != VK_SUCCESS) {
        LOGE("Failed to end command buffer: %d", result);
    }
}

void CommandList::Reset() {
    // 重置命令缓冲区
    VkResult result = vkResetCommandBuffer(m_cmdBuffer, 0);
    if (result != VK_SUCCESS) {
        LOGE("Failed to reset command buffer: %d", result);
    }
    
    // 清理状态
    m_currentContext = CommandContext::Invalid;
    m_inRenderPass = false;
    m_inDynamicRendering = false;
    m_currentRenderPass = VK_NULL_HANDLE;
    
    // 清理资源引用和屏障
    m_resourceReferences.clear();
    m_imageBarriers.clear();
    m_bufferBarriers.clear();
    m_memoryBarriers.clear();
}

void CommandList::Submit(VkQueue queue, VkFence fence) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmdBuffer;
    
    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS) {
        LOGE("Failed to submit command buffer: %d", result);
    }
}

void CommandList::SubmitAndWait(VkQueue queue) {
    Submit(queue);
    vkQueueWaitIdle(queue);
}

void CommandList::BeginDebugLabel(const char* label, float r, float g, float b, float a) {
    if (m_debugUtil) {
        VkDebugUtilsLabelEXT labelInfo{};
        labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        labelInfo.pLabelName = label;
        labelInfo.color[0] = r;
        labelInfo.color[1] = g;
        labelInfo.color[2] = b;
        labelInfo.color[3] = a;
        
        m_debugUtil->beginLabel(m_cmdBuffer, label);
    }
}

void CommandList::EndDebugLabel() {
    if (m_debugUtil) {
        m_debugUtil->endLabel(m_cmdBuffer);
    }
}

void CommandList::InsertDebugLabel(const char* label, float r, float g, float b, float a) {
    if (m_debugUtil) {
        m_debugUtil->insertLabel(m_cmdBuffer, label);
    }
}

void CommandList::BeginRenderPass(const RenderPassDesc& renderPassDesc, VkFramebuffer framebuffer, VkRenderPass renderPass) {
    // 确保不在渲染通道中
    if (m_inRenderPass || m_inDynamicRendering) {
        LOGE("Cannot begin render pass while already in a render pass");
        return;
    }
    
    // 构建渲染通道开始信息
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea = renderPassDesc.renderArea;
    
    // 设置清除值
    beginInfo.clearValueCount = static_cast<uint32_t>(renderPassDesc.clearValues.size());
    beginInfo.pClearValues = renderPassDesc.clearValues.data();
    
    // 开始渲染通道
    vkCmdBeginRenderPass(m_cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    m_inRenderPass = true;
    m_currentRenderPass = renderPass;
    
    // 设置上下文为图形
    SetContext(CommandContext::Graphics);
}

void CommandList::EndRenderPass() {
    if (m_inRenderPass) {
        vkCmdEndRenderPass(m_cmdBuffer);
        m_inRenderPass = false;
        m_currentRenderPass = VK_NULL_HANDLE;
    } else {
        LOGW("Attempting to end a render pass when not in one");
    }
}

void CommandList::BeginDynamicRendering(const RenderPassDesc& renderPassDesc) {
    // 确保不在渲染通道中
    if (m_inRenderPass || m_inDynamicRendering) {
        LOGE("Cannot begin dynamic rendering while already in a render pass");
        return;
    }
    
    std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
    colorAttachments.reserve(renderPassDesc.colorAttachments.size());
    
    // 设置颜色附件
    for (size_t i = 0; i < renderPassDesc.colorAttachments.size(); ++i) {
        VkRenderingAttachmentInfoKHR colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageView = renderPassDesc.colorAttachments[i];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        // 如果有清除值，设置清除值
        if (i < renderPassDesc.clearValues.size()) {
            colorAttachment.clearValue = renderPassDesc.clearValues[i];
        }
        
        // 如果有解析附件，设置解析附件
        if (i < renderPassDesc.resolveAttachments.size() && renderPassDesc.resolveAttachments[i] != VK_NULL_HANDLE) {
            colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            colorAttachment.resolveImageView = renderPassDesc.resolveAttachments[i];
            colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        
        colorAttachments.push_back(colorAttachment);
    }
    
    // 设置深度模板附件
    VkRenderingAttachmentInfoKHR depthAttachment{};
    if (renderPassDesc.depthStencilAttachment != VK_NULL_HANDLE) {
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView = renderPassDesc.depthStencilAttachment;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
        // 设置深度清除值
        if (!renderPassDesc.clearValues.empty()) {
            depthAttachment.clearValue = renderPassDesc.clearValues.back();
        }
    }
    
    // 动态渲染信息
    VkRenderingInfoKHR renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea = renderPassDesc.renderArea;
    renderingInfo.layerCount = renderPassDesc.layers;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    
    if (renderPassDesc.depthStencilAttachment != VK_NULL_HANDLE) {
        renderingInfo.pDepthAttachment = &depthAttachment;
    }
    
    // 开始动态渲染
    vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);
    
    m_inDynamicRendering = true;
    
    // 设置上下文为图形
    SetContext(CommandContext::Graphics);
}

void CommandList::EndDynamicRendering() {
    if (m_inDynamicRendering) {
        vkCmdEndRendering(m_cmdBuffer);
        m_inDynamicRendering = false;
    } else {
        LOGW("Attempting to end dynamic rendering when not in one");
    }
}

void CommandList::BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) {
    vkCmdBindPipeline(m_cmdBuffer, bindPoint, pipeline);
}

void CommandList::BindDescriptorSets(VkPipelineLayout layout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets, VkPipelineBindPoint bindPoint) {
    if (!descriptorSets.empty()) {
        vkCmdBindDescriptorSets(m_cmdBuffer, bindPoint, layout, firstSet, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
    }
}

void CommandList::BindDescriptorSet(VkPipelineLayout layout, uint32_t set, VkDescriptorSet descriptorSet, VkPipelineBindPoint bindPoint) {
    vkCmdBindDescriptorSets(m_cmdBuffer, bindPoint, layout, set, 1, &descriptorSet, 0, nullptr);
}

void CommandList::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data) {
    vkCmdPushConstants(m_cmdBuffer, layout, stageFlags, offset, size, data);
}

void CommandList::BindVertexBuffers(uint32_t firstBinding, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets) {
    if (buffers.size() == offsets.size() && !buffers.empty()) {
        vkCmdBindVertexBuffers(m_cmdBuffer, firstBinding, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets.data());
    }
}

void CommandList::BindVertexBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize offset) {
    vkCmdBindVertexBuffers(m_cmdBuffer, binding, 1, &buffer, &offset);
}

void CommandList::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    vkCmdBindIndexBuffer(m_cmdBuffer, buffer, offset, indexType);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    if (m_currentContext != CommandContext::Graphics) {
        LOGW("Draw called outside of graphics context");
        SetContext(CommandContext::Graphics);
    }
    
    vkCmdDraw(m_cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    if (m_currentContext != CommandContext::Graphics) {
        LOGW("DrawIndexed called outside of graphics context");
        SetContext(CommandContext::Graphics);
    }
    
    vkCmdDrawIndexed(m_cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandList::DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    if (m_currentContext != CommandContext::Graphics) {
        LOGW("DrawIndirect called outside of graphics context");
        SetContext(CommandContext::Graphics);
    }
    
    vkCmdDrawIndirect(m_cmdBuffer, buffer, offset, drawCount, stride);
}

void CommandList::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    if (m_currentContext != CommandContext::Graphics) {
        LOGW("DrawIndexedIndirect called outside of graphics context");
        SetContext(CommandContext::Graphics);
    }
    
    vkCmdDrawIndexedIndirect(m_cmdBuffer, buffer, offset, drawCount, stride);
}

void CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (m_currentContext != CommandContext::Compute) {
        LOGW("Dispatch called outside of compute context");
        SetContext(CommandContext::Compute);
    }
    
    vkCmdDispatch(m_cmdBuffer, groupCountX, groupCountY, groupCountZ);
}

void CommandList::DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) {
    if (m_currentContext != CommandContext::Compute) {
        LOGW("DispatchIndirect called outside of compute context");
        SetContext(CommandContext::Compute);
    }
    
    vkCmdDispatchIndirect(m_cmdBuffer, buffer, offset);
}

void CommandList::TraceRays(const VkStridedDeviceAddressRegionKHR& raygenShaderBindingTable,
                            const VkStridedDeviceAddressRegionKHR& missShaderBindingTable,
                            const VkStridedDeviceAddressRegionKHR& hitShaderBindingTable,
                            const VkStridedDeviceAddressRegionKHR& callableShaderBindingTable,
                            uint32_t width, uint32_t height, uint32_t depth) {
    if (m_currentContext != CommandContext::Compute) {
        LOGW("TraceRays called outside of compute context");
        SetContext(CommandContext::Compute);
    }
    
    vkCmdTraceRaysKHR(m_cmdBuffer, 
                     &raygenShaderBindingTable, 
                     &missShaderBindingTable, 
                     &hitShaderBindingTable, 
                     &callableShaderBindingTable, 
                     width, height, depth);
}

void CommandList::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange) {
    if (oldLayout == newLayout) {
        return; // 无需转换相同的布局
    }
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;
    
    // 根据旧布局设置源访问掩码
    switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            barrier.srcAccessMask = 0;
            break;
    }
    
    // 根据新布局设置目标访问掩码
    switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            barrier.dstAccessMask = 0;
            break;
    }
    
    m_imageBarriers.push_back(barrier);
}

void CommandList::TransitionImageLayout(VkImage image, ResourceState oldState, ResourceState newState, VkImageSubresourceRange subresourceRange) {
    TransitionImageLayout(image, static_cast<VkImageLayout>(oldState), static_cast<VkImageLayout>(newState), subresourceRange);
}

void CommandList::BufferBarrier(VkBuffer buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    
    m_bufferBarriers.push_back(barrier);
}

void CommandList::FlushBarriers() {
    if (m_imageBarriers.empty() && m_bufferBarriers.empty() && m_memoryBarriers.empty()) {
        return; // 没有屏障需要刷新
    }
    
    // 确定源和目标管线阶段
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    
    // 执行屏障
    vkCmdPipelineBarrier(
        m_cmdBuffer,
        srcStageMask, dstStageMask,
        0,
        static_cast<uint32_t>(m_memoryBarriers.size()), m_memoryBarriers.empty() ? nullptr : m_memoryBarriers.data(),
        static_cast<uint32_t>(m_bufferBarriers.size()), m_bufferBarriers.empty() ? nullptr : m_bufferBarriers.data(),
        static_cast<uint32_t>(m_imageBarriers.size()), m_imageBarriers.empty() ? nullptr : m_imageBarriers.data()
    );
    
    // 清理屏障
    m_imageBarriers.clear();
    m_bufferBarriers.clear();
    m_memoryBarriers.clear();
}

void CommandList::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    
    vkCmdCopyBuffer(m_cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void CommandList::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, const std::vector<VkBufferImageCopy>& regions) {
    vkCmdCopyBufferToImage(m_cmdBuffer, srcBuffer, dstImage, dstImageLayout, static_cast<uint32_t>(regions.size()), regions.data());
}

void CommandList::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, const std::vector<VkBufferImageCopy>& regions) {
    vkCmdCopyImageToBuffer(m_cmdBuffer, srcImage, srcImageLayout, dstBuffer, static_cast<uint32_t>(regions.size()), regions.data());
}

void CommandList::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, const std::vector<VkImageCopy>& regions) {
    vkCmdCopyImage(m_cmdBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, static_cast<uint32_t>(regions.size()), regions.data());
}

void CommandList::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue& color, const std::vector<VkImageSubresourceRange>& ranges) {
    vkCmdClearColorImage(m_cmdBuffer, image, imageLayout, &color, static_cast<uint32_t>(ranges.size()), ranges.data());
}

void CommandList::ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue& value, const std::vector<VkImageSubresourceRange>& ranges) {
    vkCmdClearDepthStencilImage(m_cmdBuffer, image, imageLayout, &value, static_cast<uint32_t>(ranges.size()), ranges.data());
}

void CommandList::ClearAttachments(const std::vector<VkClearAttachment>& attachments, const std::vector<VkClearRect>& rects) {
    vkCmdClearAttachments(m_cmdBuffer, static_cast<uint32_t>(attachments.size()), attachments.data(), static_cast<uint32_t>(rects.size()), rects.data());
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
    VkViewport viewport{};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    
    vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);
}

void CommandList::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    VkRect2D scissor{};
    scissor.offset = {x, y};
    scissor.extent = {width, height};
    
    vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);
}

void CommandList::SetViewportAndScissor(float x, float y, float width, float height) {
    SetViewport(x, y, width, height);
    SetScissor(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void CommandList::SetContext(CommandContext context) {
    m_currentContext = context;
}

void CommandList::AddResourceReference(const std::shared_ptr<void>& resource) {
    if (resource) {
        m_resourceReferences.push_back(resource);
    }
}

} // namespace ame 