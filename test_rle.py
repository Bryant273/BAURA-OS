import struct

with open("/mnt/c/Users/SDA/Downloads/bge_real_desktop.bmp", "rb") as f:
    data = f.read()[54:] # skip BMP header

pixels = [struct.unpack("<I", data[i:i+4])[0] for i in range(0, len(data), 4)]
runs = []
cur_p = pixels[0]
cur_len = 0
for p in pixels:
    if p == cur_p and cur_len < 65535:
        cur_len += 1
    else:
        runs.append((cur_p, cur_len))
        cur_p = p
        cur_len = 1
runs.append((cur_p, cur_len))
print("Total pixels:", len(pixels))
print("Total runs:", len(runs))
print("RLE size bytes:", len(runs) * 6)
print("RLE size KB:", (len(runs) * 6) / 1024)
