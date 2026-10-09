// Slang -> SPIR-V compiler: compiles a set of entry points in one session and reports errors with file and line.
#pragma once

#include "Graphics/ShaderCompiler/ShaderEntryPoint.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace ghost::graphics::shader {

struct CompileResult {
    bool ok = false;
    std::vector<std::vector<uint32_t>> spirv;           // one module per requested entry point, same order
    std::vector<std::filesystem::path> dependencies;    // every source file read, imports included (for hot reload)
    std::string errorFile;                              // first error location, root-relative ("" if unknown)
    int errorLine = 0;
    std::string errorMessage;                           // first error, one line
    std::string diagnostics;                            // full compiler output (errors and warnings)
};

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();
    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    // searchPaths: where `import X;` looks (ShaderLibrary/). Each entry point's own folder is added per compile.
    bool initialize(std::vector<std::filesystem::path> searchPaths);
    void shutdown();

    // Reads the sources fresh from disk every time, so edited files are always picked up.
    CompileResult compile(std::span<const ShaderEntryPoint> entryPoints);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ghost::graphics::shader
