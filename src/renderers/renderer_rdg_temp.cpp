#include "renderer.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "settings.hpp"

namespace ame {

// Simple temporary implementation of RDG renderer
class RendererRDG : public Renderer
{
public:
  RendererRDG() = default;
  ~RendererRDG() override = default;

  bool init(Resources& res, Scene& scene) override
  {
    m_resources = &res;
    m_scene = &scene;
    return true;
  }

  void deinit(Resources& res) override
  {
    // Nothing to clean up in this temporary implementation
  }

  void render(VkCommandBuffer cmd, Resources& res, Scene& scene, Settings& settings, nvvk::ProfilerVK& profiler) override
  {
    // Empty render function
    m_resources = &res; // Store for getOutputImage
  }

  bool onUI() override
  {
    // No UI changes
    return false;
  }

  void handleChange(Resources& res, Scene& scene) override
  {
    // Nothing to handle
  }

  VkDescriptorImageInfo getOutputImage() const override
  {
    // Return the final image from resources if available
    if(m_resources && m_resources->m_finalImage)
    {
      return m_resources->m_finalImage->getDescriptorImageInfo(0);
    }
    // Otherwise return an empty descriptor
    return {};
  }

  bool reloadShaders(Resources& res, Scene& scene) override
  {
    // No shaders to reload
    return true;
  }

private:
  Resources* m_resources{nullptr};
  Scene* m_scene{nullptr};
};

// Factory function to create the RDG renderer
std::unique_ptr<Renderer> makeRendererRDG()
{
  return std::make_unique<RendererRDG>();
}

} // namespace ame 