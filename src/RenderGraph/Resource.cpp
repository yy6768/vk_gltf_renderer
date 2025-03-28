#include "Resource.h"

namespace ame {

Resource::Resource(const std::string& name, Type type)
    : name_(name), type_(type) {}

Resource::~Resource() {}

const std::string& Resource::getName() const { return name_; }

Type Resource::getType() const { return type_; }

VkDeviceMemory Resource::getMemory() const { return memory_; }

VkDeviceSize Resource::getSize() const { return size_; }

VkImageLayout Resource::getImageLayout() const { return imageLayout_; }

void Resource::setImageLayout(VkImageLayout layout) { imageLayout_ = layout; }





}
