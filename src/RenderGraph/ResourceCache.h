#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include "Resource.h"
#include "Field.h"

#include "glm/vec2.hpp"

namespace ame {

class ResourceCache {
    using ResourceMap = std::unordered_map<std::string, std::weak_ptr<Resource>>;
public:
    ResourceCache();
    ~ResourceCache();


    /**
     * Properties to use during resource creation when its property has not been fully specified.
     */
    struct DefaultProperties
    {
        glm::vec2 dims;                                      ///< Width, height of the swap chain
        VkFormat format = VK_FORMAT_UNDEFINED; ///< Format to use for texture creation
    };

    std::shared_ptr<Resource> getResource(const std::string& name);

    /**
     * Add/Remove reference to a graph input resource not owned by the cache
     * @param[in] name The resource's name
     * @param[in] pResource The resource to register. If this is null, will unregister the resource
     */
    void registerExternalResource(const std::string& name, const std::shared_ptr<Resource> pResource);

    /**
     * Register a field that requires resources to be allocated.
     * @param[in] name String in the format of PassName.FieldName
     * @param[in] field Reflection data for the field
     * @param[in] timePoint The point in time for when this field is used. Normally this is an index into the execution order.
     * @param[in] alias Optional. Another field name described in the same way as 'name'.
     * If specified, and the field exists in the cache, the resource will be aliased with 'name' and field properties will be merged.
     */
    void registerField(
        const std::string& name,
        const Field& field,
        uint32_t timePoint,
        const std::string& alias = ""
    );

    /**
     * Get a resource by name. Includes external resources known by the cache.
     */
    const std::shared_ptr<Resource>& getResource(const std::string& name) const;

    /**
     * Get the field-reflection of a resource
     */
    const Field& getResourceReflection(const std::string& name) const;

    /**
     * Allocate all resources that need to be created/updated.
     * This includes new resources, resources whose properties have been updated since last allocation call.
     */
    void allocateResources(const std::shared_ptr<VkDevice>& pDevice, const DefaultProperties& params);

    /**
     * Clears all registered field/resource properties and allocated resources.
     */
    void reset();
private:
    ResourceMap cache_;
};


}


