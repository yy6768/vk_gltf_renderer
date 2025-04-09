#pragma once

#include "core/commandlists.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace ame {

// 命令列表记录器，用于在渲染图中记录并保存命令列表
class CommandListRecorder {
public:
    using CommandRecordFunction = std::function<void(CommandList&)>;

    CommandListRecorder(VkDevice device);
    ~CommandListRecorder();

    // 记录命令列表
    void RecordCommandList(const std::string& name, CommandListType type, CommandRecordFunction recordFunction);
    
    // 获取命令列表
    CommandList* GetCommandList(const std::string& name);
    
    // 提交所有命令列表
    void SubmitAll(VkQueue queue, VkFence fence = VK_NULL_HANDLE);
    
    // 等待所有命令列表完成
    void WaitAll(VkQueue queue);
    
    // 重置所有命令列表
    void ResetAll();
    
    // 获取所有命令列表
    std::vector<VkCommandBuffer> GetAllCommandBuffers() const;

private:
    VkDevice m_device;
    std::unordered_map<std::string, std::unique_ptr<CommandList>> m_commandLists;
};

// 范围命令记录器，用于自动开始和结束命令列表
class ScopedCommandRecorder {
public:
    ScopedCommandRecorder(CommandList& cmdList) : m_cmdList(cmdList) {
        m_cmdList.Begin();
    }
    
    ~ScopedCommandRecorder() {
        m_cmdList.End();
    }
    
    CommandList& GetCommandList() { return m_cmdList; }

private:
    CommandList& m_cmdList;
};

// 渲染通道范围记录器，用于自动开始和结束渲染通道
class ScopedRenderPassRecorder {
public:
    ScopedRenderPassRecorder(CommandList& cmdList, const RenderPassDesc& renderPassDesc, VkFramebuffer framebuffer, VkRenderPass renderPass)
        : m_cmdList(cmdList) {
        m_cmdList.BeginRenderPass(renderPassDesc, framebuffer, renderPass);
    }
    
    ~ScopedRenderPassRecorder() {
        m_cmdList.EndRenderPass();
    }
    
    CommandList& GetCommandList() { return m_cmdList; }

private:
    CommandList& m_cmdList;
};

// 动态渲染范围记录器，用于自动开始和结束动态渲染
class ScopedDynamicRenderingRecorder {
public:
    ScopedDynamicRenderingRecorder(CommandList& cmdList, const RenderPassDesc& renderPassDesc)
        : m_cmdList(cmdList) {
        m_cmdList.BeginDynamicRendering(renderPassDesc);
    }
    
    ~ScopedDynamicRenderingRecorder() {
        m_cmdList.EndDynamicRendering();
    }
    
    CommandList& GetCommandList() { return m_cmdList; }

private:
    CommandList& m_cmdList;
};

// 调试标记范围记录器，用于自动开始和结束调试标记
class ScopedDebugLabelRecorder {
public:
    ScopedDebugLabelRecorder(CommandList& cmdList, const char* label, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : m_cmdList(cmdList) {
        m_cmdList.BeginDebugLabel(label, r, g, b, a);
    }
    
    ~ScopedDebugLabelRecorder() {
        m_cmdList.EndDebugLabel();
    }
    
    CommandList& GetCommandList() { return m_cmdList; }

private:
    CommandList& m_cmdList;
};

} // namespace ame 