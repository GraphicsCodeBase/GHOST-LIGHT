// Technique: the constructor announces the object so its Param<T> members (constructed right after) register with it.
#include "Graphics/TechniqueRuntime/Technique.h"

#include "Graphics/TechniqueRuntime/Param.h"

namespace ghost::graphics::techniques {

Technique::Technique() {
    ParamBase::beginRegistration(this);
}

} // namespace ghost::graphics::techniques
