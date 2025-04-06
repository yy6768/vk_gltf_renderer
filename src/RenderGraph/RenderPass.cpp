#include "RenderPass.h"

namespace ame {

RenderPass::RenderPass(const std::string& name, 
                       VkDevice& device, 
                       const RenderPassConfig& config) 
    : name_(name), device_(device), config_(config) {
    renderPass_ = nvvk::createRenderPass(device, 
                                        config_.colorFormats, 
                                        config_.depthFormat, 
                                        config_.subpassCount, 
                                        config_.clearColor, 
                                        config_.clearDepth, 
                                        config_.initialLayout, 
                                        config_.finalLayout);
}


void RenderPass::addInput(const std::string& name, std::shared_ptr<Resource> resource) {
    inputs_[name] = resource;
}

void RenderPass::addOutput(const std::string& name, std::shared_ptr<Resource> resource) {
    outputs_[name] = resource;
}

} // namespace ame