# Introduction


这个仓库是由VkGLTF Renderer改进过来的，主要希望实现UE的一些先进特性：
- [ ] Render Graph
- [ ] SVGF/NRD
- [ ] DDGI
- [ ] Surfel GI
- [ ] Mesh SDF
- [ ] AI Denoising + SR

# Update
- 2025/04/06 Single Pass Render

# Render Graph (RDG)

A Render Graph (RDG) system, similar to Unreal Engine's RenderGraph, has been implemented. This allows for:

- Automatic resource management (creation, destruction, reuse)
- Automatic barrier insertion between passes
- Dependency tracking and potential optimization of render pass order

Resources
- RDGTexture: Represents textures used in rendering
- RDGBuffer: Represents buffers used in rendering
- RDGPass: Represents a single render pass with inputs and outputs

## Usage Example

```cpp
// Create the render graph
RenderGraph graph(device);

// Create resources
auto colorTex = graph.createTexture(colorDesc);
auto depthTex = graph.createTexture(depthDesc);

// Add passes and define dependencies
auto geometryPass = graph.addPass("GeometryPass", [&](VkCommandBuffer cmd) {
  // Render geometry here
});
geometryPass->addOutput(colorTex, RDGResourceState::WriteOnly);
geometryPass->addOutput(depthTex, RDGResourceState::WriteOnly);

auto postProcessPass = graph.addPass("PostProcessPass", [&](VkCommandBuffer cmd) {
  // Post-process here
});
postProcessPass->addInput(colorTex, RDGResourceState::ReadOnly);
postProcessPass->addOutput(outputTex, RDGResourceState::WriteOnly);

// Execute the graph
graph.execute(cmd);

// Reset for next frame
graph.reset();
```
