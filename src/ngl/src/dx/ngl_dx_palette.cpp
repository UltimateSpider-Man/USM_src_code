#include "ngl_dx_palette.h"

#include "ngl.h"
#include "variables.h"

nglPalette *nglCreatePalette([[maybe_unused]] int a1, uint32_t entries, const void *a3)
{
    auto *v3 = static_cast<nglPalette *>(tlMemAlloc(sizeof(nglPalette), 8, 0x1000000));
    v3->m_palette_idx = 0;
    v3->m_palette_entries = nullptr;
    v3->m_size = 0;
    v3->m_size = entries;

    v3->m_palette_entries = static_cast<PALETTEENTRY *>(
        tlMemAlloc(sizeof(PALETTEENTRY) * entries, 8, 0x1000000));
    v3->m_palette_idx = dword_975BE8()[dword_975BE0()--];
    if (a3 != nullptr) {
        std::memcpy(v3->m_palette_entries, a3, 4 * ((4 * entries) >> 2));

#ifdef OPENUSM_XBPACK_V10
        // Xbox v10 DDSM palettes sometimes carry RGB only: every fourth byte
        // is zero. D3D9 interprets PALETTEENTRY::peFlags as alpha, making the
        // otherwise-white VENOM_EYE entries fully transparent. Only repair a
        // palette whose complete alpha channel is absent; authored alpha ramps
        // (mouth, tongue, particles, city textures) remain byte-for-byte intact.
        bool hasAuthoredAlpha = false;
        for (uint32_t i = 0; i < entries; ++i)
            hasAuthoredAlpha |= v3->m_palette_entries[i].peFlags != 0;

        if (!hasAuthoredAlpha) {
            for (uint32_t i = 0; i < entries; ++i) {
                PALETTEENTRY &entry = v3->m_palette_entries[i];
                // Preserve the zero/black color-key entry as transparent.
                if (entry.peRed != 0 || entry.peGreen != 0 || entry.peBlue != 0)
                    entry.peFlags = 0xFF;
            }
        }
#endif
    }

    if (!g_valid_texture_format()) {
        g_Direct3DDevice()->lpVtbl->SetPaletteEntries(g_Direct3DDevice(),
                                                      v3->m_palette_idx,
                                                      v3->m_palette_entries);
    }

    return v3;
}
