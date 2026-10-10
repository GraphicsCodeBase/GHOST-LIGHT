// RayQueryNormals: example technique (not from RTG). One compute pass traces a camera ray per pixel with an inline ray
// query against the scene TLAS and shows the normal of the surface it hits, blended over the lit image.
#include "Graphics/TechniqueRuntime/Param.h"
#include "Graphics/TechniqueRuntime/Technique.h"
#include "Graphics/TechniqueRuntime/TechniqueBuilder.h"
#include "Graphics/TechniqueRuntime/TechniqueContext.h"
#include "Graphics/TechniqueRuntime/TechniqueRegistry.h"

using namespace ghost::graphics::techniques;

namespace {

// Mirror of `Constants` in Shaders/RayQueryNormals.slang.
struct Constants {
    VkDeviceAddress frame;
    uint32_t color;
    float blend;
    uint32_t geometricNormals;
    uint32_t alphaTest;
};

} // namespace

class RayQueryNormals : public Technique {
public:
    TECHNIQUE_INFO("Ray query normals", "Examples");
    TechniqueStage stage() const override { return TechniqueStage::Post; }

    Param<float> blend{"Blend", 1.0f, 0.0f, 1.0f};
    Param<bool> geometricNormals{"Geometric normals", false};
    Param<bool> alphaTest{"Alpha test", true};

    void setup(TechniqueBuilder& builder) override {
        builder.readWrite("scene.color");
        builder.computePipeline("normals", "Shaders/RayQueryNormals.slang");
    }

    void execute(TechniqueContext& ctx) override {
        ctx.pushConstants(Constants{ctx.frameConstants(), ctx.storage("scene.color"), blend, geometricNormals ? 1u : 0u, alphaTest ? 1u : 0u});
        ctx.dispatch("normals", ctx.renderSize());
    }
};
REGISTER_TECHNIQUE(RayQueryNormals);
