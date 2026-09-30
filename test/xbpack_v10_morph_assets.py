"""Extract read-only v10 morph position fixtures; no game assets are copied into tests.
Usage: python test/xbpack_v10_morph_assets.py <xbox-pack-directory> <output.bin>
"""
import pathlib
import struct
import sys

records = []
images = frames = sections = 0
for path in pathlib.Path(sys.argv[1]).glob('*.XBPACK'):
    data = path.read_bytes()
    pos = 0
    while True:
        image = data.find(b'XBXM', pos)
        if image < 0:
            break
        pos = image + 4
        def u32(offset):
            return struct.unpack_from('<I', data, image + offset)[0]
        if u32(4) != 0x1601:
            continue
        count, directory, base = u32(8), u32(12), u32(16)
        directory -= base
        if count > 1000 or directory < 0 or image + directory + count*12 > len(data):
            continue
        found = False
        for entry in range(count):
            entry = directory + entry*12
            if data[image+entry+3] != 3:
                continue
            found = True
            morph = u32(entry+4)-base
            for frame in range(u32(morph+4)):
                frame = u32(morph+8)-base+frame*12
                frames += 1
                for section in range(u32(frame+4)):
                    section = u32(frame+8)-base+section*136
                    vertices, mask = u32(section), u32(section+4)
                    sections += 1
                    assert mask in (0, 1), (path, section, mask)
                    assert all(u32(section+8+4*s) == 0 for s in range(1,32))
                    pointer = u32(section+8)
                    assert bool(pointer) == bool(mask)
                    if not pointer:
                        continue
                    start = pointer-base
                    cursor, vertex = start, 0
                    while True:
                        count, skip = struct.unpack_from('<HH', data, image+cursor)
                        cursor += 4 + count*12
                        vertex += count
                        assert vertex <= vertices
                        if not skip:
                            break
                        vertex += skip
                        assert vertex <= vertices
                    records.append((vertices, data[image+start:image+cursor]))
        images += found
with pathlib.Path(sys.argv[2]).open('wb') as out:
    out.write(struct.pack('<II', 0x31464d58, len(records)))
    for vertices, stream in records:
        out.write(struct.pack('<II', vertices, len(stream)))
        out.write(stream)
print(f'{images} images, {frames} frames, {sections} sections, {len(records)} position streams; all masks/pointers and runs valid')
