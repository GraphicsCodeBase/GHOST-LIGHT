// TechniqueName: <one line: what this technique does and why a game would want it>.
// Created by new_technique.bat from Techniques/_Template. See Graphics/TechniqueRuntime/README.md for the API.
#include "Graphics/TechniqueRuntime/Param.h"
#include "Graphics/TechniqueRuntime/Technique.h"
#include "Graphics/TechniqueRuntime/TechniqueBuilder.h"
#include "Graphics/TechniqueRuntime/TechniqueContext.h"
#include "Graphics/TechniqueRuntime/TechniqueRegistry.h"

#include <glm/vec4.hpp>

using namespace ghost::graphics::techniques;

namespace {

// Mirror of `Constants` in Shaders/TechniqueName.slang: same order, same types. Use 4-byte fields, uint64 addresses
// and float4; avoid float3 (its alignment differs between C++ and the shader).
struct Constants {
    VkDeviceAddress frame; // FrameConstants* (import Scene;)
    uint32_t color;        // storage index of scene.color
    float strength;
    glm::vec4 tint;        // rgb used
};

} // namespace

class TechniqueName : public Technique {
public:
    TECHNIQUE_INFO("TechniqueName", "TechniqueCategory");
    TechniqueStage stage() const override { return TechniqueStage::Post; }

    // Shown in the Techniques panel (F2) and loaded from the scene: "params": { "strength": 0.5, "tint": [1, 0.8, 0.6] }.
    Param<float> strength{"Strength", 0.0f, 0.0f, 1.0f};
    Param<glm::vec3> tint{"Tint", glm::vec3(1.0f, 0.8f, 0.6f)};

    void setup(TechniqueBuilder& builder) override {
        builder.readWrite("scene.color");
        builder.computePipeline("main", "Shaders/TechniqueName.slang");
    }

    void execute(TechniqueContext& ctx) override {
        const glm::vec3 color = tint;
        ctx.pushConstants(Constants{ctx.frameConstants(), ctx.storage("scene.color"), strength, glm::vec4(color, 0.0f)});
        ctx.dispatch("main", ctx.renderSize());
    }
};
REGISTER_TECHNIQUE(TechniqueName);
