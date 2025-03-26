#include "RenderPass.h"

namespace vk_gltf_renderer {

RenderPass::RenderPass(const std::string& name) : name_(name) {}

RenderPass::~RenderPass() {}

void RenderPass::addInput(const std::string& name, std::shared_ptr<Resource> resource) {
    inputs_[name] = resource;
}

void RenderPass::addOutput(const std::string& name, std::shared_ptr<Resource> resource) {
    outputs_[name] = resource;
}

} // namespace vk_gltf_renderer 