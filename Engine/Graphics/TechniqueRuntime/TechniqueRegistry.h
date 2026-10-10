// Every technique class compiled into the executable registers itself here (REGISTER_TECHNIQUE), so adding a technique
// never needs an engine edit: the build picks up the new folder and the registry picks up the new class.
#pragma once

#include "Graphics/TechniqueRuntime/Technique.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ghost::graphics::techniques {

class TechniqueRegistry {
public:
    using Factory = std::function<std::unique_ptr<Technique>()>;

    struct Entry {
        std::string id;         // class name
        std::string sourceFile; // __FILE__ of the registering .cpp (locates the technique folder)
        Factory factory;
    };

    // Called by REGISTER_TECHNIQUE during static initialization. Returns true (so it can initialize a static).
    static bool add(const char* id, const char* sourceFile, Factory factory);
    static const std::vector<Entry>& entries();
    // Builds one instance with its id, folder and params set up.
    static std::unique_ptr<Technique> create(const Entry& entry);

private:
    static std::vector<Entry>& storage();
};

} // namespace ghost::graphics::techniques

// After the class definition: REGISTER_TECHNIQUE(MyTechnique);
#define REGISTER_TECHNIQUE(Class)                                                                                     \
    static const bool g_ghostTechniqueRegistered_##Class = ::ghost::graphics::techniques::TechniqueRegistry::add(    \
        #Class, __FILE__, []() -> std::unique_ptr<::ghost::graphics::techniques::Technique> { return std::make_unique<Class>(); })
