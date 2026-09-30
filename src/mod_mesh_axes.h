#pragma once

#include <array>

namespace modmesh {
namespace axes {

// FBX GlobalSettings use zero-based axis indices. The shipped native exports
// use +X right, +Y up, +Z front, also the MayaYUp/OpenGL FBX convention. Keep
// missing metadata at that identity convention for existing files and OBJ.
struct FbxAxes {
    int upAxis = 1, upSign = 1;
    int frontAxis = 2, frontSign = 1;
    int coordAxis = 0, coordSign = 1;
};

// Orthogonal signed permutation: game[i] = sign[i] * file[sourceAxis[i]].
// Units are deliberately separate, so existing sceneScale handling applies
// once to geometry and bind positions. Do not apply this to every local node:
// transform the hierarchy root and global bind matrices instead.
struct Transform {
    std::array<int, 3> sourceAxis{{0, 1, 2}};
    std::array<int, 3> sign{{1, 1, 1}};

    bool identity() const
    {
        return sourceAxis == std::array<int, 3>{{0, 1, 2}}
            && sign == std::array<int, 3>{{1, 1, 1}};
    }

    bool reflected() const
    {
        int det = sign[0] * sign[1] * sign[2];
        for (int i = 0; i < 3; ++i)
            for (int j = i + 1; j < 3; ++j)
                if (sourceAxis[i] > sourceAxis[j]) det = -det;
        return det < 0;
    }

    template <class T>
    std::array<T, 3> vector(const std::array<T, 3> &value) const
    {
        return {{T(sign[0]) * value[sourceAxis[0]],
                 T(sign[1]) * value[sourceAxis[1]],
                 T(sign[2]) * value[sourceAxis[2]]}};
    }

    // Row-major storage, column-vector convention, compatible with M4.
    std::array<double, 16> rowMajorMatrix() const
    {
        std::array<double, 16> result{};
        for (int row = 0; row < 3; ++row)
            result[row * 4 + sourceAxis[row]] = double(sign[row]);
        result[15] = 1.0;
        return result;
    }

    // FBX TransformLink arrays use ROW vectors: translation occupies 12..14.
    // Change only the global/output frame: B_game = B_file * transpose(A).
    // This preserves each bone's local frame, as root-only node conversion
    // does; conjugating this matrix would incorrectly rotate its local input.
    // Retain the fourth lane and all scaling/shear in the source bind.
    std::array<double, 16> convertFbxWorldMatrix(
        const std::array<double, 16> &source) const
    {
        auto result = source;
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 3; ++column)
                result[row * 4 + column] = double(sign[column])
                    * source[row * 4 + sourceAxis[column]];
        return result;
    }
};

// A malformed/partial frame must not index out of bounds or collapse a mesh.
// Report failure and leave a deterministic identity for the caller to log.
inline bool fromFbx(const FbxAxes &source, Transform &result)
{
    result = Transform{};
    const std::array<int, 3> indices{{source.coordAxis, source.upAxis, source.frontAxis}};
    const std::array<int, 3> signs{{source.coordSign, source.upSign, source.frontSign}};
    for (int i = 0; i < 3; ++i)
        if (indices[i] < 0 || indices[i] > 2 || (signs[i] != -1 && signs[i] != 1))
            return false;
    if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
        return false;
    result.sourceAxis = indices;
    result.sign = signs;
    return true;
}

} // namespace axes
} // namespace modmesh
