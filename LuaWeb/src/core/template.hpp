#pragma once

#include <string>

namespace luaweb {

/**
 * Template Engine - Wrapper for Rust liblwtemplate
 */
class TemplateEngine {
public:
    TemplateEngine();
    ~TemplateEngine();

    /**
     * Render a template file with JSON data
     * @param template_path Path to .lwt template file
     * @param json_data JSON string of data to inject
     * @param cache_dir Optional cache directory (for __cacheweb__)
     * @return Rendered HTML string, or empty on error
     */
    std::string render(const std::string& template_path, 
                       const std::string& json_data,
                       const std::string& cache_dir = "");

    /**
     * Get the last error message
     */
    std::string get_error() const;

    /**
     * Clear the template cache
     */
    void clear_cache();

    /**
     * Check if the template engine is available
     */
    static bool is_available();

private:
    bool loaded_;
};

} // namespace luaweb
