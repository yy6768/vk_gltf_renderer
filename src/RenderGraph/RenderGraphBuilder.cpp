#include "RenderGraphBuilder.hpp"
#include "RenderGraph.hpp"
#include "GraphResource.hpp"

namespace ame {

RenderGraphBuilder::RenderGraphBuilder() {
    m_graph = std::make_shared<RenderGraph>();
}

RenderGraphBuilder& RenderGraphBuilder::addPass(std::shared_ptr<RenderPass> pass) {
    if (!pass) {
        return *this;
    }
    
    m_passes[pass->getName()] = pass;
    m_graph->addPass(pass);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addResource(const std::string& name, std::shared_ptr<GraphResource> resource) {
    if (!resource) {
        return *this;
    }
    
    m_resources[name] = resource;
    m_graph->addResource(name, resource);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setInput(const std::string& passName, const std::string& inputName, const std::string& resourceName) {
    auto passIt = m_passes.find(passName);
    auto resourceIt = m_resources.find(resourceName);
    
    if (passIt != m_passes.end() && resourceIt != m_resources.end()) {
        passIt->second->addInput(inputName, resourceIt->second);
    }
    
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setOutput(const std::string& passName, const std::string& outputName, const std::string& resourceName) {
    auto passIt = m_passes.find(passName);
    auto resourceIt = m_resources.find(resourceName);
    
    if (passIt != m_passes.end() && resourceIt != m_resources.end()) {
        passIt->second->addOutput(outputName, resourceIt->second);
    }
    
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::connect(
    const std::string& outputPassName, 
    const std::string& outputName, 
    const std::string& inputPassName, 
    const std::string& inputName
) {
    auto outputPassIt = m_passes.find(outputPassName);
    auto inputPassIt = m_passes.find(inputPassName);
    
    if (outputPassIt != m_passes.end() && inputPassIt != m_passes.end()) {
        const auto& outputs = outputPassIt->second->getOutputs();
        auto outputIt = outputs.find(outputName);
        
        if (outputIt != outputs.end()) {
            inputPassIt->second->addInput(inputName, outputIt->second);
        }
    }
    
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setExternalInput(const std::string& passName, const std::string& inputName, std::shared_ptr<GraphResource> resource) {
    auto passIt = m_passes.find(passName);
    
    if (passIt != m_passes.end() && resource) {
        std::string resourceName = "external_input_" + passName + "_" + inputName;
        m_resources[resourceName] = resource;
        m_graph->addResource(resourceName, resource);
        passIt->second->addInput(inputName, resource);
    }
    
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setExternalOutput(const std::string& passName, const std::string& outputName, std::shared_ptr<GraphResource> resource) {
    auto passIt = m_passes.find(passName);
    
    if (passIt != m_passes.end() && resource) {
        std::string resourceName = "external_output_" + passName + "_" + outputName;
        m_resources[resourceName] = resource;
        m_graph->addResource(resourceName, resource);
        passIt->second->addOutput(outputName, resource);
    }
    
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::setExtent(VkExtent2D extent) {
    m_extent = extent;
    m_graph->setExtent(extent);
    return *this;
}

std::shared_ptr<RenderGraph> RenderGraphBuilder::build() {
    // 编译渲染图
    m_graph->compile();
    return m_graph;
}

std::shared_ptr<RenderGraph> RenderGraphBuilder::build(std::function<void(RenderGraphBuilder&)> setup) {
    if (setup) {
        setup(*this);
    }
    return build();
}

} // namespace ame 