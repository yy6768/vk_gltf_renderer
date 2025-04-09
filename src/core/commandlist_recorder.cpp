#include "commandlist_recorder.hpp"
#include "nvh/nvprint.hpp"

namespace ame {

CommandListRecorder::CommandListRecorder(VkDevice device)
    : m_device(device)
{
}

CommandListRecorder::~CommandListRecorder()
{
    m_commandLists.clear();
}

void CommandListRecorder::RecordCommandList(const std::string& name, CommandListType type, CommandRecordFunction recordFunction)
{
    // 检查是否已经存在相同名称的命令列表
    auto it = m_commandLists.find(name);
    if (it == m_commandLists.end()) {
        // 创建新的命令列表
        auto cmdList = std::make_unique<CommandList>(m_device, type, name.c_str());
        it = m_commandLists.emplace(name, std::move(cmdList)).first;
    } else {
        // 重置现有命令列表
        it->second->Reset();
    }
    
    // 记录命令
    CommandList& cmdList = *it->second;
    cmdList.Begin();
    
    if (recordFunction) {
        try {
            recordFunction(cmdList);
        } catch (const std::exception& e) {
            LOGE("Exception during command recording: %s", e.what());
        } catch (...) {
            LOGE("Unknown exception during command recording");
        }
    }
    
    cmdList.End();
}

CommandList* CommandListRecorder::GetCommandList(const std::string& name)
{
    auto it = m_commandLists.find(name);
    if (it != m_commandLists.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CommandListRecorder::SubmitAll(VkQueue queue, VkFence fence)
{
    if (m_commandLists.empty()) {
        return;
    }
    
    std::vector<VkCommandBuffer> cmdBuffers;
    cmdBuffers.reserve(m_commandLists.size());
    
    for (const auto& [name, cmdList] : m_commandLists) {
        cmdBuffers.push_back(cmdList->GetNative());
    }
    
    // 创建提交信息
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    submitInfo.pCommandBuffers = cmdBuffers.data();
    
    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS) {
        LOGE("Failed to submit command buffers: %d", result);
    }
}

void CommandListRecorder::WaitAll(VkQueue queue)
{
    VkResult result = vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS) {
        LOGE("Failed to wait for queue idle: %d", result);
    }
}

void CommandListRecorder::ResetAll()
{
    for (auto& [name, cmdList] : m_commandLists) {
        cmdList->Reset();
    }
}

std::vector<VkCommandBuffer> CommandListRecorder::GetAllCommandBuffers() const
{
    std::vector<VkCommandBuffer> cmdBuffers;
    cmdBuffers.reserve(m_commandLists.size());
    
    for (const auto& [name, cmdList] : m_commandLists) {
        cmdBuffers.push_back(cmdList->GetNative());
    }
    
    return cmdBuffers;
}

} // namespace ame 