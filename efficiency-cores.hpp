#pragma once
#include <windows.h>
#include <intrin.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

inline std::vector<ULONG> intelEfficiencyCpuSets()
{
    int cpu[4]{};
    __cpuid(cpu, 0);
    if (cpu[1] != 0x756e6547 || cpu[3] != 0x49656e69 || cpu[2] != 0x6c65746e)
        return {};

    ULONG bytes = 0;
    const auto process = GetCurrentProcess();
    GetSystemCpuSetInformation(nullptr, 0, &bytes, process, 0);
    if (!bytes)
        return {};
    std::vector<unsigned char> storage(bytes);
    if (!GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(storage.data()),
                                    bytes, &bytes, process, 0))
        return {};

    std::vector<std::pair<ULONG, BYTE>> eligible;
    BYTE lowest = 255, highest = 0;
    for (ULONG offset = 0; offset < bytes;) {
        SYSTEM_CPU_SET_INFORMATION entry{};
        const auto *position = storage.data() + offset;
        if (bytes - offset < sizeof(entry.Size))
            break;
        std::memcpy(&entry.Size, position, sizeof(entry.Size));
        if (!entry.Size || entry.Size > bytes - offset)
            break;
        if (entry.Size >= sizeof(entry)) {
            std::memcpy(&entry, position, sizeof(entry));
            if (entry.Type == CpuSetInformation) {
                const auto &set = entry.CpuSet;
                lowest = std::min(lowest, set.EfficiencyClass);
                highest = std::max(highest, set.EfficiencyClass);
                if (!set.Allocated || set.AllocatedToTargetProcess)
                    eligible.emplace_back(set.Id, set.EfficiencyClass);
            }
        }
        offset += entry.Size;
    }
    std::vector<ULONG> ids;
    if (lowest < highest)
        for (const auto &[id, efficiency] : eligible)
            if (efficiency == lowest)
                ids.push_back(id);
    return ids;
}
