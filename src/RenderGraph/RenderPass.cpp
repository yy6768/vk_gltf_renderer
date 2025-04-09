#include "RenderPass.hpp"

namespace ame {

RenderPass::RenderPass(const std::string& name, 
                       VkDevice& device, 
                       RenderPassType type,
                       RenderPassFlags flags) 
    : name_(name), device_(device), type_(type), flags_(flags) {
}



void RenderPass::addInput(const std::string& name, std::shared_ptr<GraphResource> resource) {
    inputs_[name] = resource;
}

void RenderPass::addOutput(const std::string& name, std::shared_ptr<GraphResource> resource) {
    outputs_[name] = resource;
}





} // namespace ame