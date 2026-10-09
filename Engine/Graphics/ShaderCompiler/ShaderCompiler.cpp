// Slang compilation: one fresh session per compile (so edited files are re-read), all entry points linked together, SPIR-V 1.6 out.
#include "Graphics/ShaderCompiler/ShaderCompiler.h"

#include "Core/Log.h"
#include "Core/Paths.h"

#include <algorithm>
#include <cstring>
#include <regex>
#include <sstream>

#include <slang-com-ptr.h>
#include <slang.h>

namespace ghost::graphics::shader {

namespace {

std::string blobText(slang::IBlob* blob) {
    if (!blob || blob->getBufferSize() == 0) {
        return {};
    }
    std::string text(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
    while (!text.empty() && (text.back() == '\0' || text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
    return text;
}

void appendDiagnostics(std::string& all, slang::IBlob* blob) {
    const std::string text = blobText(blob);
    if (!text.empty()) {
        if (!all.empty()) {
            all += '\n';
        }
        all += text;
    }
}

// Slang 2026 reports errors Rust-style:      Older releases used one line:
//   error[E20001]: unexpected token            path(4): error 20001: unexpected token
//    --> path:4:53
//     |   ...  ^^^ unexpected identifier, expected ';'
void fillFirstError(CompileResult& result) {
    static const std::regex kHeader(R"(^\s*(?:fatal\s+)?error(?:\[\w+\])?\s*:\s*(.*)$)");
    static const std::regex kLocation(R"(^\s*-->\s*(.+):(\d+):(\d+)\s*$)");
    static const std::regex kCaret(R"(^\s*\|\s*\^+\s*(.*)$)");
    static const std::regex kOldStyle(R"(^(.+)\((\d+)\)\s*:\s*(?:fatal\s+)?error\s*\w*\s*:\s*(.*)$)");

    std::istringstream lines(result.diagnostics);
    std::string line;
    std::string firstErrorLine;
    bool inError = false;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::smatch match;
        if (!inError && std::regex_match(line, match, kHeader)) {
            result.errorMessage = match[1].str();
            inError = true;
        } else if (inError && result.errorFile.empty() && std::regex_match(line, match, kLocation)) {
            result.errorFile = core::Paths::display(core::Paths::fromUtf8(match[1].str()));
            result.errorLine = std::stoi(match[2].str());
        } else if (inError && std::regex_match(line, match, kCaret)) {
            if (!match[1].str().empty()) {
                result.errorMessage += ": " + match[1].str();
            }
            return;
        } else if (!inError && std::regex_match(line, match, kOldStyle)) {
            result.errorFile = core::Paths::display(core::Paths::fromUtf8(match[1].str()));
            result.errorLine = std::stoi(match[2].str());
            result.errorMessage = match[3].str();
            return;
        }
        if (firstErrorLine.empty() && line.find("error") != std::string::npos) {
            firstErrorLine = line;
        }
    }
    if (inError) {
        return;
    }
    if (result.errorMessage.empty()) {
        result.errorMessage = !firstErrorLine.empty() ? firstErrorLine : (result.diagnostics.empty() ? "unknown shader error" : result.diagnostics);
    }
}

CompileResult failure(CompileResult result) {
    result.ok = false;
    fillFirstError(result);
    return result;
}

void addUnique(std::vector<std::filesystem::path>& list, const std::filesystem::path& path) {
    const std::filesystem::path normal = path.lexically_normal();
    if (std::find(list.begin(), list.end(), normal) == list.end()) {
        list.push_back(normal);
    }
}

} // namespace

struct ShaderCompiler::Impl {
    Slang::ComPtr<slang::IGlobalSession> global;
    std::vector<std::filesystem::path> searchPaths;
    SlangProfileID profile = SLANG_PROFILE_UNKNOWN;
};

ShaderCompiler::ShaderCompiler() = default;

ShaderCompiler::~ShaderCompiler() {
    shutdown();
}

bool ShaderCompiler::initialize(std::vector<std::filesystem::path> searchPaths) {
    m_impl = std::make_unique<Impl>();
    m_impl->searchPaths = std::move(searchPaths);
    if (SLANG_FAILED(slang::createGlobalSession(m_impl->global.writeRef()))) {
        core::Log::error("Could not start the Slang shader compiler (slang.dll / slang-compiler.dll missing next to the exe?)");
        m_impl.reset();
        return false;
    }
    m_impl->profile = m_impl->global->findProfile("spirv_1_6");
    core::Log::info("Slang shader compiler ready");
    return true;
}

void ShaderCompiler::shutdown() {
    m_impl.reset();
}

CompileResult ShaderCompiler::compile(std::span<const ShaderEntryPoint> entryPoints) {
    CompileResult result;
    if (!m_impl) {
        result.diagnostics = "shader compiler not initialized";
        return failure(std::move(result));
    }

    // Files in first-use order; each one is a Slang module named after the file (Splash.slang -> Splash).
    std::vector<std::filesystem::path> files;
    for (const ShaderEntryPoint& entry : entryPoints) {
        addUnique(files, entry.file);
        addUnique(result.dependencies, entry.file);
    }
    for (const std::filesystem::path& file : files) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(file, ec)) {
            result.errorFile = core::Paths::display(file);
            result.diagnostics = result.errorFile + ": shader file not found";
            result.errorMessage = "shader file not found";
            return result;
        }
    }

    std::vector<std::string> searchPathsUtf8;
    for (const std::filesystem::path& path : m_impl->searchPaths) {
        searchPathsUtf8.push_back(core::Paths::toUtf8(path));
    }
    for (const std::filesystem::path& file : files) {
        const std::string dir = core::Paths::toUtf8(file.parent_path());
        if (std::find(searchPathsUtf8.begin(), searchPathsUtf8.end(), dir) == searchPathsUtf8.end()) {
            searchPathsUtf8.push_back(dir);
        }
    }
    std::vector<const char*> searchPathPointers;
    for (const std::string& path : searchPathsUtf8) {
        searchPathPointers.push_back(path.c_str());
    }

    std::vector<slang::CompilerOptionEntry> options;
    {
        slang::CompilerOptionEntry entry{};
        entry.name = slang::CompilerOptionName::EmitSpirvDirectly;
        entry.value.intValue0 = 1;
        options.push_back(entry);
    }
#ifdef GHOST_DEBUG
    {
        // Source-level shader debugging in Nsight / RenderDoc.
        slang::CompilerOptionEntry entry{};
        entry.name = slang::CompilerOptionName::DebugInformation;
        entry.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_STANDARD;
        options.push_back(entry);
    }
#endif

    slang::TargetDesc target;
    target.format = SLANG_SPIRV;
    target.profile = m_impl->profile;
    target.forceGLSLScalarBufferLayout = true; // C++ structs and shader structs share the same (scalar) layout
    target.compilerOptionEntries = options.data();
    target.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

    slang::SessionDesc sessionDesc;
    sessionDesc.targets = &target;
    sessionDesc.targetCount = 1;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR; // same memory layout as glm
    sessionDesc.searchPaths = searchPathPointers.data();
    sessionDesc.searchPathCount = static_cast<SlangInt>(searchPathPointers.size());

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(m_impl->global->createSession(sessionDesc, session.writeRef()))) {
        result.diagnostics = "Slang could not create a compile session";
        return failure(std::move(result));
    }

    std::vector<slang::IModule*> modules;
    for (const std::filesystem::path& file : files) {
        Slang::ComPtr<slang::IBlob> diagnostics;
        slang::IModule* module = session->loadModule(core::Paths::toUtf8(file.stem()).c_str(), diagnostics.writeRef());
        appendDiagnostics(result.diagnostics, diagnostics);
        if (!module) {
            return failure(std::move(result));
        }
        modules.push_back(module);
    }

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entries;
    for (const ShaderEntryPoint& entry : entryPoints) {
        const size_t moduleIndex = static_cast<size_t>(std::find(files.begin(), files.end(), entry.file.lexically_normal()) - files.begin());
        Slang::ComPtr<slang::IEntryPoint> found;
        if (SLANG_FAILED(modules[moduleIndex]->findEntryPointByName(entry.function.c_str(), found.writeRef()))) {
            result.errorFile = core::Paths::display(entry.file);
            result.errorMessage = "entry point '" + entry.function + "' not found (does it have a [shader(\"...\")] attribute?)";
            result.diagnostics = result.errorFile + ": " + result.errorMessage;
            return result;
        }
        entries.push_back(found);
    }

    std::vector<slang::IComponentType*> components;
    for (slang::IModule* module : modules) {
        components.push_back(module);
    }
    for (const auto& entry : entries) {
        components.push_back(entry.get());
    }

    Slang::ComPtr<slang::IComponentType> composite;
    {
        Slang::ComPtr<slang::IBlob> diagnostics;
        const SlangResult ok = session->createCompositeComponentType(components.data(), static_cast<SlangInt>(components.size()),
                                                                     composite.writeRef(), diagnostics.writeRef());
        appendDiagnostics(result.diagnostics, diagnostics);
        if (SLANG_FAILED(ok)) {
            return failure(std::move(result));
        }
    }
    Slang::ComPtr<slang::IComponentType> linked;
    {
        Slang::ComPtr<slang::IBlob> diagnostics;
        const SlangResult ok = composite->link(linked.writeRef(), diagnostics.writeRef());
        appendDiagnostics(result.diagnostics, diagnostics);
        if (SLANG_FAILED(ok)) {
            return failure(std::move(result));
        }
    }

    // Modules contribute no entry points to the composite, so entry point i is entries[i].
    for (size_t i = 0; i < entries.size(); ++i) {
        Slang::ComPtr<slang::IBlob> code;
        Slang::ComPtr<slang::IBlob> diagnostics;
        const SlangResult ok = linked->getEntryPointCode(static_cast<SlangInt>(i), 0, code.writeRef(), diagnostics.writeRef());
        appendDiagnostics(result.diagnostics, diagnostics);
        if (SLANG_FAILED(ok) || !code) {
            return failure(std::move(result));
        }
        std::vector<uint32_t> words(code->getBufferSize() / sizeof(uint32_t));
        std::memcpy(words.data(), code->getBufferPointer(), words.size() * sizeof(uint32_t));
        result.spirv.push_back(std::move(words));
    }

    for (slang::IModule* module : modules) {
        const SlangInt32 count = module->getDependencyFileCount();
        for (SlangInt32 i = 0; i < count; ++i) {
            if (const char* path = module->getDependencyFilePath(i)) {
                addUnique(result.dependencies, core::Paths::fromUtf8(path));
            }
        }
    }
    result.ok = true;
    return result;
}

} // namespace ghost::graphics::shader
