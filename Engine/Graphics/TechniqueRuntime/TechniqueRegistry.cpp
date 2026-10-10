// TechniqueRegistry: a function-local list (safe during static initialization) and the source-file -> folder mapping.
#include "Graphics/TechniqueRuntime/TechniqueRegistry.h"

#include "Core/Paths.h"
#include "Graphics/TechniqueRuntime/Param.h"

namespace ghost::graphics::techniques {

namespace {

// "D:/anywhere/Techniques/Shadows/X/X.cpp" -> <repo root>/Techniques/Shadows/X. Only the part from "Techniques" on is
// used, so the result is right even if the repository moved after the build.
std::filesystem::path techniqueFolder(const std::string& sourceFile) {
    const std::filesystem::path source = core::Paths::fromUtf8(sourceFile);
    std::filesystem::path relative;
    bool found = false;
    for (const std::filesystem::path& part : source.parent_path()) {
        if (!found && part == "Techniques") {
            found = true;
        }
        if (found) {
            relative /= part;
        }
    }
    return found ? core::Paths::root() / relative : source.parent_path();
}

} // namespace

std::vector<TechniqueRegistry::Entry>& TechniqueRegistry::storage() {
    static std::vector<Entry> entries;
    return entries;
}

bool TechniqueRegistry::add(const char* id, const char* sourceFile, Factory factory) {
    storage().push_back({id, sourceFile, std::move(factory)});
    return true;
}

const std::vector<TechniqueRegistry::Entry>& TechniqueRegistry::entries() {
    return storage();
}

std::unique_ptr<Technique> TechniqueRegistry::create(const Entry& entry) {
    std::unique_ptr<Technique> technique = entry.factory();
    ParamBase::beginRegistration(nullptr); // Params created from here on belong to no technique
    technique->m_id = entry.id;
    technique->m_folder = techniqueFolder(entry.sourceFile);
    return technique;
}

} // namespace ghost::graphics::techniques
