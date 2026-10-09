// Scene file format (version 1): name, environment, exposure, player, cameraBookmarks, entities, techniques.
// Entities: name, prefab, components, transform, overrides (applied in that order). See Content/Scenes/README.md.
#include "World/Serialization/SceneLoader.h"

#include "Core/JsonFile.h"
#include "Core/JsonReader.h"
#include "Core/Paths.h"
#include "World/Components/Name.h"
#include "World/Components/PrefabInstance.h"
#include "World/Components/Transform.h"
#include "World/Serialization/ComponentReader.h"

#include <map>
#include <memory>

#include <entt/entity/registry.hpp>

namespace ghost::world {

namespace {

constexpr int kSceneVersion = 1;

// A parsed prefab file, kept with its own JsonReader context so its problems point into the prefab file.
struct Prefab {
    bool ok = false;
    core::JsonFile::LoadResult file;
    std::unique_ptr<core::JsonReader::Context> context;
};

class Loader {
public:
    Loader(SceneLoadResult& result, entt::registry& registry, assets::AssetRegistry& assets)
        : m_result(result), m_registry(registry), m_assets(assets) {}

    void readEntity(const core::JsonReader& json, size_t index) {
        if (!json.expectObject("an entity")) {
            return;
        }
        json.warnUnknownKeys({"name", "prefab", "components", "transform", "overrides"});

        const entt::entity entity = m_registry.create();
        m_registry.emplace<Name>(entity, json.string("name", "Entity" + std::to_string(index)));
        Transform& transform = m_registry.emplace<Transform>(entity);

        if (json.has("prefab")) {
            const std::string prefabPath = json.string("prefab");
            if (const Prefab* prefab = loadPrefab(prefabPath, json.child("prefab"))) {
                const core::JsonReader root(*prefab->context, prefab->file.value, "");
                applyComponents(root.child("components"), entity);
                m_registry.emplace<PrefabInstance>(entity, prefabPath);
            }
        }
        if (json.has("components")) {
            applyComponents(json.child("components"), entity);
        }
        if (json.has("transform")) {
            ComponentReader::readTransform(json.child("transform"), transform);
        }
        if (json.has("overrides")) {
            applyComponents(json.child("overrides"), entity);
        }
        ++m_result.entityCount;
    }

    void finish() {
        // Problems found inside prefab files are reported with the prefab's own file and line.
        for (auto& [path, prefab] : m_prefabs) {
            if (prefab->context) {
                m_result.errors.insert(m_result.errors.end(), prefab->context->errors.begin(), prefab->context->errors.end());
                m_result.warnings.insert(m_result.warnings.end(), prefab->context->warnings.begin(), prefab->context->warnings.end());
            }
        }
        m_result.warnings.insert(m_result.warnings.end(), m_assetWarnings.begin(), m_assetWarnings.end());
    }

private:
    void applyComponents(const core::JsonReader& components, entt::entity entity) {
        if (!components.exists()) {
            return;
        }
        if (!components.expectObject("components")) {
            return;
        }
        for (const std::string& name : components.keys()) {
            ComponentReader::apply(name, components.child(name), m_registry, entity, m_assets, m_assetWarnings);
        }
    }

    const Prefab* loadPrefab(const std::string& path, const core::JsonReader& reference) {
        auto found = m_prefabs.find(path);
        if (found == m_prefabs.end()) {
            auto prefab = std::make_unique<Prefab>();
            const std::filesystem::path file = core::Paths::resolveContent(path);
            m_result.files.push_back(file);
            prefab->file = core::JsonFile::load(file);
            if (!prefab->file.ok) {
                reference.error(prefab->file.error);
            } else {
                prefab->context = std::make_unique<core::JsonReader::Context>();
                prefab->context->file = core::Paths::display(file);
                prefab->context->text = prefab->file.text;
                const core::JsonReader root(*prefab->context, prefab->file.value, "");
                if (root.expectObject("a prefab")) {
                    root.warnUnknownKeys({"version", "name", "category", "components"});
                    if (root.integer("version", kSceneVersion) > kSceneVersion) {
                        root.child("version").warning("written by a newer engine version; loading what is understood");
                    }
                    prefab->ok = true;
                }
            }
            found = m_prefabs.emplace(path, std::move(prefab)).first;
        }
        return found->second->ok ? found->second.get() : nullptr;
    }

    SceneLoadResult& m_result;
    entt::registry& m_registry;
    assets::AssetRegistry& m_assets;
    std::map<std::string, std::unique_ptr<Prefab>> m_prefabs;
    std::vector<std::string> m_assetWarnings;
};

void readSettings(const core::JsonReader& root, SceneSettings& settings, assets::AssetRegistry& assets) {
    settings.name = root.string("name", settings.name);

    const core::JsonReader environment = root.child("environment");
    if (environment.exists() && environment.expectObject("environment settings")) {
        environment.warnUnknownKeys({"hdri", "intensity"});
        settings.environmentHdri = environment.string("hdri");
        settings.environmentIntensity = environment.number("intensity", settings.environmentIntensity);
        if (!settings.environmentHdri.empty()) {
            std::string error;
            settings.environment = assets.loadEnvironment(settings.environmentHdri, error);
            if (settings.environment == assets::kInvalidEnvironment) {
                environment.child("hdri").error(error);
            }
        }
    }

    const core::JsonReader exposure = root.child("exposure");
    if (exposure.exists() && exposure.expectObject("exposure settings")) {
        exposure.warnUnknownKeys({"ev100"});
        settings.exposureEv100 = exposure.number("ev100", settings.exposureEv100);
    }

    const core::JsonReader player = root.child("player");
    if (player.exists() && player.expectObject("player settings")) {
        player.warnUnknownKeys({"mode", "start"});
        settings.playerMode = player.string("mode", settings.playerMode);
        settings.playerStart = player.string("start");
        if (settings.playerMode != "Fly" && settings.playerMode != "Walk") {
            player.child("mode").error("expected \"Fly\" or \"Walk\"");
            settings.playerMode = "Fly";
        }
    }

    const core::JsonReader bookmarks = root.child("cameraBookmarks");
    if (bookmarks.exists()) {
        if (!bookmarks.isArray()) {
            bookmarks.error("expected an array of bookmarks");
        }
        for (size_t i = 0; i < bookmarks.size(); ++i) {
            const core::JsonReader bookmark = bookmarks.element(i);
            if (!bookmark.expectObject("a camera bookmark")) {
                continue;
            }
            bookmark.warnUnknownKeys({"name", "position", "yawPitch"});
            CameraBookmark out;
            out.name = bookmark.string("name", "Bookmark" + std::to_string(i));
            out.position = bookmark.vec3("position", out.position);
            const glm::vec2 yawPitch = bookmark.vec2("yawPitch", glm::vec2(0.0f));
            out.yawDeg = yawPitch.x;
            out.pitchDeg = yawPitch.y;
            settings.bookmarks.push_back(out);
        }
    }
    if (!settings.playerStart.empty() && !settings.findBookmark(settings.playerStart)) {
        player.child("start").warning("no camera bookmark is named \"" + settings.playerStart + "\"");
    }

    const core::JsonReader techniques = root.child("techniques");
    if (techniques.exists() && techniques.expectObject("technique settings")) {
        settings.techniques = techniques.raw();
    }
}

} // namespace

SceneLoadResult SceneLoader::load(const std::string& contentPath, entt::registry& registry, SceneSettings& settings,
                                  assets::AssetRegistry& assets) {
    SceneLoadResult result;
    const std::filesystem::path path = core::Paths::resolveContent(contentPath);
    result.files.push_back(path);

    core::JsonFile::LoadResult file = core::JsonFile::load(path);
    if (!file.ok) {
        result.errors.push_back(file.error);
        return result;
    }

    core::JsonReader::Context context;
    context.file = core::Paths::display(path);
    context.text = std::move(file.text);
    const core::JsonReader root(context, file.value, "");
    if (!root.expectObject("a scene")) {
        result.errors = context.errors;
        return result;
    }
    root.warnUnknownKeys({"version", "name", "environment", "exposure", "player", "cameraBookmarks", "entities", "techniques"});
    if (root.integer("version", kSceneVersion) > kSceneVersion) {
        root.child("version").warning("written by a newer engine version; loading what is understood");
    }

    settings.file = contentPath;
    settings.name = core::Paths::toUtf8(path.stem().stem()); // Sponza.scene.json -> Sponza
    readSettings(root, settings, assets);

    Loader loader(result, registry, assets);
    const core::JsonReader entities = root.child("entities");
    if (entities.exists() && !entities.isArray()) {
        entities.error("expected an array of entities");
    }
    for (size_t i = 0; i < entities.size(); ++i) {
        loader.readEntity(entities.element(i), i);
    }
    loader.finish();

    result.errors.insert(result.errors.begin(), context.errors.begin(), context.errors.end());
    result.warnings.insert(result.warnings.begin(), context.warnings.begin(), context.warnings.end());
    result.loaded = true;
    return result;
}

} // namespace ghost::world
