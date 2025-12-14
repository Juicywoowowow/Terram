#include "template.hpp"
#include <dlfcn.h>
#include <iostream>
#include <filesystem>

namespace luaweb {

// Function pointers for Rust library
typedef char* (*lwtemplate_render_fn)(const char*, const char*, const char*);
typedef void (*lwtemplate_free_fn)(char*);
typedef const char* (*lwtemplate_get_error_fn)();
typedef void (*lwtemplate_clear_cache_fn)();

// Dynamic library handle
static void* lib_handle = nullptr;
static lwtemplate_render_fn fn_render = nullptr;
static lwtemplate_free_fn fn_free = nullptr;
static lwtemplate_get_error_fn fn_get_error = nullptr;
static lwtemplate_clear_cache_fn fn_clear_cache = nullptr;

static bool load_library() {
    if (lib_handle) {
        return true;  // Already loaded
    }
    
    // Try to find the library in various locations
    std::vector<std::string> search_paths = {
        "./liblwtemplate.so",
        "./liblwtemplate.dylib",
        "../template/target/release/liblwtemplate.so",
        "../template/target/release/liblwtemplate.dylib",
        "template/target/release/liblwtemplate.so",
        "template/target/release/liblwtemplate.dylib",
    };
    
    for (const auto& path : search_paths) {
        lib_handle = dlopen(path.c_str(), RTLD_NOW);
        if (lib_handle) {
            break;
        }
    }
    
    if (!lib_handle) {
        std::cerr << "[LuaWeb] Template engine not available: " << dlerror() << std::endl;
        return false;
    }
    
    // Load functions
    fn_render = (lwtemplate_render_fn)dlsym(lib_handle, "lwtemplate_render");
    fn_free = (lwtemplate_free_fn)dlsym(lib_handle, "lwtemplate_free");
    fn_get_error = (lwtemplate_get_error_fn)dlsym(lib_handle, "lwtemplate_get_error");
    fn_clear_cache = (lwtemplate_clear_cache_fn)dlsym(lib_handle, "lwtemplate_clear_cache");
    
    if (!fn_render || !fn_free) {
        std::cerr << "[LuaWeb] Template engine missing required functions" << std::endl;
        dlclose(lib_handle);
        lib_handle = nullptr;
        return false;
    }
    
    return true;
}

TemplateEngine::TemplateEngine() : loaded_(load_library()) {
}

TemplateEngine::~TemplateEngine() {
    // Don't unload library - it's shared across instances
}

std::string TemplateEngine::render(const std::string& template_path, 
                                   const std::string& json_data,
                                   const std::string& cache_dir) {
    if (!loaded_ || !fn_render) {
        return "";
    }
    
    const char* cache_ptr = cache_dir.empty() ? nullptr : cache_dir.c_str();
    char* result = fn_render(template_path.c_str(), json_data.c_str(), cache_ptr);
    
    if (!result) {
        return "";
    }
    
    std::string html(result);
    
    if (fn_free) {
        fn_free(result);
    }
    
    return html;
}

std::string TemplateEngine::get_error() const {
    if (!loaded_ || !fn_get_error) {
        return "Template engine not loaded";
    }
    
    const char* err = fn_get_error();
    return err ? std::string(err) : "";
}

void TemplateEngine::clear_cache() {
    if (loaded_ && fn_clear_cache) {
        fn_clear_cache();
    }
}

bool TemplateEngine::is_available() {
    return load_library();
}

} // namespace luaweb
