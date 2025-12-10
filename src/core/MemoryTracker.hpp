#pragma once

#include <cstddef>
#include <string>

namespace terram {

// Memory tracking utilities
class MemoryTracker {
public:
    static void* allocate(size_t bytes, const char* source, const char* file, int line);
    static void deallocate(void* ptr, const char* source);

    static void logStats();

private:
    static size_t s_totalAllocated;
    static size_t s_totalFreed;
    static size_t s_allocationCount;
    static size_t s_freeCount;
};

} // namespace terram

// Macros for tracked allocation
#define TERRAM_NEW(Type, ...) \
    new (terram::MemoryTracker::allocate(sizeof(Type), #Type, __FILE__, __LINE__)) Type(__VA_ARGS__)

#define TERRAM_DELETE(ptr, Type) \
    do { \
        if (ptr) { \
            (ptr)->~Type(); \
            terram::MemoryTracker::deallocate(ptr, #Type); \
        } \
    } while(0)

#define TERRAM_NEW_ARRAY(Type, count) \
    static_cast<Type*>(terram::MemoryTracker::allocate(sizeof(Type) * (count), #Type "[]", __FILE__, __LINE__))

#define TERRAM_DELETE_ARRAY(ptr, Type) \
    terram::MemoryTracker::deallocate(ptr, #Type "[]")
