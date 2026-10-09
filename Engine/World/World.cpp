// World: loads scenes into a fresh registry and swaps it in only when the file was readable (a broken edit never
// destroys the running scene), watches the files involved, and runs the systems.
#include "World/World.h"

#include "World/Systems/TransformSystem.h"

#include <system_error>
#include <utility>

namespace ghost::world {

namespace {

std::filesystem::file_time_type modificationTime(const std::filesystem::path& path) {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(path, ec);
    return ec ? std::filesystem::file_time_type::min() : time;
}

} // namespace

SceneLoadResult World::loadScene(const std::string& contentPath, assets::AssetRegistry& assets) {
    entt::registry registry;
    SceneSettings settings;
    SceneLoadResult result = SceneLoader::load(contentPath, registry, settings, assets);
    if (!result.loaded) {
        return result;
    }

    m_registry = std::move(registry);
    m_settings = std::move(settings);
    m_watched.clear();
    for (const std::filesystem::path& file : result.files) {
        m_watched.push_back({file, modificationTime(file)});
    }
    ++m_sceneRevision;
    TransformSystem::update(m_registry); // world matrices exist from the very first frame
    return result;
}

bool World::reloadIfChanged(assets::AssetRegistry& assets, SceneLoadResult& result) {
    const auto now = std::chrono::steady_clock::now();
    if (m_watched.empty() || now - m_lastCheck < std::chrono::milliseconds(500)) {
        return false;
    }
    m_lastCheck = now;

    bool changed = false;
    for (WatchedFile& file : m_watched) {
        const auto time = modificationTime(file.path);
        if (time != file.time) {
            file.time = time; // a file that stays broken is not retried every half second
            changed = true;
        }
    }
    if (!changed) {
        return false;
    }
    result = loadScene(m_settings.file, assets);
    return true;
}

void World::update() {
    TransformSystem::update(m_registry);
}

} // namespace ghost::world
