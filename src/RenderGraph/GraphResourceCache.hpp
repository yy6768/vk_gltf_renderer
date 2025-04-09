#include "GraphResource.hpp"

namespace ame {

class GraphResourceCache {
public:
    GraphResourceCache() = default;
    ~GraphResourceCache() = default;

private:
    std::unordered_map<std::string, std::shared_ptr<GraphResource>> m_resources;
};

}
