from PIL import Image
import os

source_img_path = r'C:\Users\Admin\.gemini\antigravity\brain\79c1013c-9241-473f-9fd8-247938a52d87\png_studio_icon_1783090100474.png'
target_dir = r'C:\Users\Admin\.gemini\antigravity\scratch\PNG-Studio-Native\icons'

img = Image.open(source_img_path)

# Generate PNGs
sizes = [16, 32, 48, 128, 256]
for size in sizes:
    resized = img.resize((size, size), Image.Resampling.LANCZOS)
    resized.save(os.path.join(target_dir, f'app_icon_{size}.png'))

# Generate ICO
icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
img.save(os.path.join(target_dir, 'app_icon.ico'), sizes=icon_sizes)

print('Icons replaced successfully.')
