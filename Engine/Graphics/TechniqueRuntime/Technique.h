// Base class of every ray tracing technique in Techniques/: declare passes in setup(), record them in execute(), expose
// Param<T> members for the UI and the scene file. Register the class with REGISTER_TECHNIQUE (TechniqueRegistry.h).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ghost::graphics::techniques {

class ParamBase;
class TechniqueBuilder;
class TechniqueContext;

// Where a technique's passes run in the frame (see Graphics/TechniqueRuntime/README.md for the full order).
enum class TechniqueStage {
    PreLighting, // after the G-buffer, before the engine's lighting (e.g. shadow masks, AO)
    Lighting,    // after the engine's lighting pass; may overwrite scene.color
    Denoise,     // cleans up noisy outputs of earlier stages
    Post,        // last word on scene.color before tonemapping (debug views, effects)
};

class Technique {
public:
    Technique();
    virtual ~Technique() = default;
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;

    // Declares the technique's passes, the textures/buffers they read and write, and its pipelines. Called whenever
    // the render graph is rebuilt (startup, enabling or disabling a technique, switching render modes).
    virtual void setup(TechniqueBuilder& builder) = 0;
    // Records one pass; ctx.pass() is its index in declaration order (always 0 for single-pass techniques).
    virtual void execute(TechniqueContext& ctx) = 0;

    // Filled in by TECHNIQUE_INFO.
    virtual const char* name() const = 0;
    virtual const char* category() const = 0;
    virtual TechniqueStage stage() const { return TechniqueStage::Lighting; }

    bool enabled = false;

    // The class name: the key in a scene's "techniques" block.
    const std::string& id() const { return m_id; }
    // Folder of the technique's .cpp (shader paths in setup() are relative to it).
    const std::filesystem::path& folder() const { return m_folder; }
    const std::vector<ParamBase*>& params() const { return m_params; }

private:
    friend class ParamBase;
    friend class TechniqueRegistry;

    std::string m_id;
    std::filesystem::path m_folder;
    std::vector<ParamBase*> m_params;
};

} // namespace ghost::graphics::techniques

// Inside the class body: the name shown in the UI and the category (a Techniques/ subfolder name).
#define TECHNIQUE_INFO(displayName, categoryName)                \
    const char* name() const override { return displayName; }     \
    const char* category() const override { return categoryName; }
