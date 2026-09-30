#pragma once

#include <cstdint>

struct CharEntropyQuantConverter {
    struct EncTrackData {
        float field_0[4][4];
    };
};

namespace CharEntropyDecoder {
    struct CharChannelDecoder {
        const void *field_0;
        bool field_4;
        char field_5;
        uint16_t field_6;

        CharChannelDecoder(
            const void *a2,
            bool a3) : field_0(a2), field_4(a3), field_5(-1), field_6(0)
        {}
    };
}

