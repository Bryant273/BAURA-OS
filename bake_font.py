import struct

with open('/mnt/c/Users/SDA/Downloads/BAURA-OS/GUI/UI_V2/inter.bgf', 'rb') as f:
    data = f.read()

faces, nsizes, nglyphs = struct.unpack('<HHH', data[4:10])
blob = struct.unpack('<I', data[12:16])[0]
cps = [struct.unpack('<I', data[16+i*4 : 16+i*4+4])[0] for i in range(nglyphs)]

def get_glyph(face_idx, size_idx, char):
    cp = ord(char)
    if cp not in cps: return None
    g_idx = cps.index(cp)
    set_idx = face_idx * nsizes + size_idx
    h_offset = 16 + nglyphs * 4 + set_idx * 16
    px, asc, desc, line = struct.unpack('<bBBB', data[h_offset:h_offset+4])
    glyphs_ptr = struct.unpack('<I', data[h_offset+4:h_offset+8])[0]
    r = data[glyphs_ptr + g_idx*10 : glyphs_ptr + (g_idx+1)*10]
    adv = struct.unpack('<h', r[0:2])[0]
    bx, by, w, h = struct.unpack('<bbBB', r[2:6])
    off = struct.unpack('<I', r[6:10])[0]
    bitmap = data[blob + off : blob + off + w * h]
    return {'char': char, 'cp': cp, 'adv': adv // 64, 'bx': bx, 'by': by, 'w': w, 'h': h, 'bitmap': bitmap, 'asc': asc, 'desc': desc, 'line': line}

def generate_header(face_idx, size_idx, name):
    struct_def = 'typedef struct {\n    int8_t bx;\n    int8_t by;\n    uint8_t w;\n    uint8_t h;\n    uint8_t adv;\n    const uint8_t *bitmap;\n} InterGlyph_' + name + ';\n'
    bitmaps = []
    glyph_defs = []
    for c in range(32, 127):
        g = get_glyph(face_idx, size_idx, chr(c))
        if g is None or g['w'] == 0 or g['h'] == 0:
            adv_val = g['adv'] if g else 8
            glyph_defs.append('{0, 0, 0, 0, ' + str(adv_val) + ', 0}')
        else:
            bm_name = 'bm_' + name + '_' + str(c)
            hex_data = ', '.join('0x' + format(b, '02X') for b in g['bitmap'])
            bitmaps.append('static const uint8_t ' + bm_name + '[] = {' + hex_data + '};')
            glyph_defs.append('{' + str(g['bx']) + ', ' + str(g['by']) + ', ' + str(g['w']) + ', ' + str(g['h']) + ', ' + str(g['adv']) + ', ' + bm_name + '}')
    
    code = struct_def + '\n' + '\n'.join(bitmaps) + '\n\nstatic const InterGlyph_' + name + ' inter_glyphs_' + name + '[95] = {\n  ' + ',\n  '.join(glyph_defs) + '\n};\n'
    return code

h28 = generate_header(1, 4, '28_semibold')
h14 = generate_header(0, 1, '14_regular')

with open('/mnt/c/Users/SDA/Downloads/BAURA-OS/BOOT/FIRMWARE/FRAMEBUFFER/baura_inter_font.h', 'w') as out:
    out.write('#ifndef BAURA_INTER_FONT_H\n#define BAURA_INTER_FONT_H\n\n#include <stdint.h>\n\n' + h28 + '\n' + h14 + '\n#endif\n')

print('Generated baura_inter_font.h successfully with struct definition!')
