#include "../src/mod_pcmesh_archive.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: mod_pcmesh_archive_probe <PCMESH.zip> <mesh-stem>\n");
        return 2;
    }

    std::string why;
    modmesh::pcmeshzip::Archive archive;
    if (!archive.open(argv[1], &why)) {
        std::fprintf(stderr, "archive open failed: %s\n", why.c_str());
        return 1;
    }

    std::vector<uint8_t> bytes;
    std::string member;
    if (!archive.extract(argv[2], bytes, &member, &why)) {
        std::fprintf(stderr, "extract failed: %s\n", why.c_str());
        return 1;
    }

    if (bytes.size() < 8u || std::memcmp(bytes.data(), "PCM ", 4u) != 0) {
        std::fprintf(stderr, "extracted member is not a PCMESH image\n");
        return 1;
    }
    uint32_t version = 0;
    std::memcpy(&version, bytes.data() + 4u, sizeof(version));
    std::printf("archive_entries=%zu member=%s bytes=%zu version=0x%X\n",
                archive.size(), member.c_str(), bytes.size(), version);
    return version == 0x601u ? 0 : 1;
}
