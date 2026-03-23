#ifndef SHARED_FRAME_IPC_H
#define SHARED_FRAME_IPC_H

#include <cstddef>
#include <cstdint>

namespace SharedFrameIPC
{
constexpr const char* kShmName = "/frame_shm";
constexpr const char* kEventFdEnv = "EGO_FRAME_READY_FD";
constexpr const char* kShmNameEnv = "EGO_FRAME_SHM_NAME";
constexpr std::size_t kMaxFrameBytes = 3840u * 2160u * 4u;

struct Header
{
    std::uint64_t sequence;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t channels;
    std::uint32_t size;
};

constexpr std::size_t kMappingSize = sizeof(Header) + kMaxFrameBytes;
} // namespace SharedFrameIPC

#endif
