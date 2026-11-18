import struct

def ppm_to_bmp(ppm_path, bmp_path):
    with open(ppm_path, "rb") as f:
        line1 = f.readline().strip()
        assert line1 == b"P6"
        line2 = f.readline().strip()
        while line2.startswith(b"#"):
            line2 = f.readline().strip()
        w, h = map(int, line2.split())
        max_val = int(f.readline().strip())
        raw = f.read()

    file_size = 54 + w * h * 4
    neg_h = -h
    header = bytearray(54)
    header[0:2] = b"BM"
    struct.pack_into("<I", header, 2, file_size)
    struct.pack_into("<I", header, 10, 54)
    struct.pack_into("<I", header, 14, 40)
    struct.pack_into("<i", header, 18, w)
    struct.pack_into("<i", header, 22, neg_h)
    struct.pack_into("<H", header, 26, 1)
    struct.pack_into("<H", header, 28, 32)

    pixels = bytearray(w * h * 4)
    for i in range(w * h):
        r = raw[i * 3]
        g = raw[i * 3 + 1]
        b = raw[i * 3 + 2]
        pixels[i * 4] = b
        pixels[i * 4 + 1] = g
        pixels[i * 4 + 2] = r
        pixels[i * 4 + 3] = 0xFF

    with open(bmp_path, "wb") as f:
        f.write(header)
        f.write(pixels)
    print(f"Wrote {bmp_path} ({w}x{h})")

ppm_to_bmp("/home/ahmed/BAURA-OS/GUI/Styles_composants_ui/baura-os-docs/06-DEMARRAGE/code/splash_final.ppm",
           "/mnt/c/Users/SDA/Downloads/splash_final.bmp")
ppm_to_bmp("/home/ahmed/BAURA-OS/GUI/Styles_composants_ui/baura-os-docs/06-DEMARRAGE/code/splash_strip.ppm",
           "/mnt/c/Users/SDA/Downloads/splash_strip.bmp")
