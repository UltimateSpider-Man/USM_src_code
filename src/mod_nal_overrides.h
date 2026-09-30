#pragma once

#include <cstdint>
#include <filesystem>

// Configured sources are private bank/skeleton inputs, never global clip mods.
bool modNalConfiguredSource(const std::filesystem::path &path);
bool modNalHasAnimationSwap(std::uint32_t bankHash);
std::uint8_t *modNalConfiguredAnimation(std::uint32_t bankHash, int *sizeOut,
    const std::uint8_t *original, int originalSize, bool *handled);
void *modNalConfiguredSkeleton(void *original, bool *handled);
struct tlresource_location;
bool modNalHandleSkeletonResource(void *handler, int behavior, tlresource_location *location);
