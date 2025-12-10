#include "MemoryTracker.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <sstream>

namespace terram {

size_t MemoryTracker::s_totalAllocated = 0;
size_t MemoryTracker::s_totalFreed = 0;
size_t MemoryTracker::s_allocationCount = 0;
size_t MemoryTracker::s_freeCount = 0;

struct AllocationInfo {
    size_t size;
    const char* source;
    const char* file;
    int line;
    std::chrono::steady_clock::time_point timestamp;
};

static std::unordered_map<void*, AllocationInfo> s_allocations;
static std::mutex s_mutex;

static std::string formatBytes(size_t bytes) {
    std::ostringstream oss;
    if (bytes >= 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
    } else if (bytes >= 1024) {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
    } else {
        oss << bytes << " bytes";
    }
    return oss.str();
}

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void* MemoryTracker::allocate(size_t bytes, const char* source, const char* file, int line) {
    void* ptr = std::malloc(bytes);

    if (ptr) {
        std::lock_guard<std::mutex> lock(s_mutex);

        s_totalAllocated += bytes;
        s_allocationCount++;

        s_allocations[ptr] = {bytes, source, file, line, std::chrono::steady_clock::now()};

        // Extract just the filename from path
        std::string filepath(file);
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;

        std::cout << "\033[32m[" << getTimestamp() << "] [ALLOC]\033[0m "
                  << "\033[1m" << formatBytes(bytes) << "\033[0m"
                  << " at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec << "\033[0m"
                  << std::endl
                  << "        └─ source: \033[33m" << source << "\033[0m"
                  << " (" << filename << ":" << line << ")"
                  << " | total allocs: " << s_allocationCount
                  << " | heap: " << formatBytes(s_totalAllocated - s_totalFreed)
                  << std::endl;
    }

    return ptr;
}

void MemoryTracker::deallocate(void* ptr, const char* source) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = s_allocations.find(ptr);
    if (it != s_allocations.end()) {
        size_t bytes = it->second.size;
        auto lifetime = std::chrono::steady_clock::now() - it->second.timestamp;
        auto lifetimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(lifetime).count();

        s_totalFreed += bytes;
        s_freeCount++;

        std::cout << "\033[31m[" << getTimestamp() << "] [FREE]\033[0m  "
                  << "\033[1m" << formatBytes(bytes) << "\033[0m"
                  << " at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec << "\033[0m"
                  << std::endl
                  << "        └─ source: \033[33m" << source << "\033[0m"
                  << " | lifetime: " << lifetimeMs << "ms"
                  << " | heap: " << formatBytes(s_totalAllocated - s_totalFreed)
                  << std::endl;

        s_allocations.erase(it);
    } else {
        std::cout << "\033[35m[" << getTimestamp() << "] [FREE?]\033[0m "
                  << "unknown block at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec << "\033[0m"
                  << " (source: " << source << ")"
                  << std::endl;
    }

    std::free(ptr);
}

void MemoryTracker::logStats() {
    std::lock_guard<std::mutex> lock(s_mutex);

    std::cout << std::endl
              << "\033[1m╔════════════════════════════════════════╗\033[0m" << std::endl
              << "\033[1m║       TERRAM MEMORY STATISTICS         ║\033[0m" << std::endl
              << "\033[1m╠════════════════════════════════════════╣\033[0m" << std::endl
              << "\033[1m║\033[0m Total Allocated:  " << std::setw(18) << formatBytes(s_totalAllocated) << " \033[1m║\033[0m" << std::endl
              << "\033[1m║\033[0m Total Freed:      " << std::setw(18) << formatBytes(s_totalFreed) << " \033[1m║\033[0m" << std::endl
              << "\033[1m║\033[0m Still in use:     " << std::setw(18) << formatBytes(s_totalAllocated - s_totalFreed) << " \033[1m║\033[0m" << std::endl
              << "\033[1m║\033[0m Allocation count: " << std::setw(18) << s_allocationCount << " \033[1m║\033[0m" << std::endl
              << "\033[1m║\033[0m Free count:       " << std::setw(18) << s_freeCount << " \033[1m║\033[0m" << std::endl
              << "\033[1m║\033[0m Leaks:            " << std::setw(18) << s_allocations.size() << " \033[1m║\033[0m" << std::endl
              << "\033[1m╚════════════════════════════════════════╝\033[0m" << std::endl;

    if (!s_allocations.empty()) {
        std::cout << std::endl << "\033[31m⚠ POTENTIAL MEMORY LEAKS:\033[0m" << std::endl;
        for (const auto& [ptr, info] : s_allocations) {
            std::cout << "  • " << formatBytes(info.size)
                      << " at 0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec
                      << " (" << info.source << ")"
                      << std::endl;
        }
    }
}

} // namespace terram
