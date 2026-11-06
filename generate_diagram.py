from PIL import Image, ImageDraw, ImageFont
import os

width, height = 1200, 750
img = Image.new('RGB', (width, height), color=(26, 32, 44))
draw = ImageDraw.Draw(img)

try:
    font_large = ImageFont.truetype('arial.ttf', 24)
    font_med = ImageFont.truetype('arial.ttf', 16)
    font_small = ImageFont.truetype('arial.ttf', 13)
    font_bold = ImageFont.truetype('arialbd.ttf', 16)
    font_title = ImageFont.truetype('arialbd.ttf', 26)
except:
    font_large = ImageFont.load_default()
    font_med = font_large
    font_small = font_large
    font_bold = font_large
    font_title = font_large

# Title
draw.text((width//2, 35), 'LilyGO T-Display v1.1 - Morse Trainer Verkabelung', fill=(255, 255, 255), font=font_title, anchor='mm')
draw.text((width//2, 68), 'Exakter 1:1 Anschlussplan (Paddle, Rotary Encoder, Buzzer)', fill=(160, 174, 192), font=font_med, anchor='mm')

# 1. ESP32 Board (Center)
esp_x, esp_y, esp_w, esp_h = 440, 105, 320, 565
draw.rectangle([esp_x, esp_y, esp_x+esp_w, esp_y+esp_h], fill=(18, 22, 28), outline=(74, 85, 104), width=3)

# TFT Screen
tft_x, tft_y, tft_w, tft_h = esp_x + 70, esp_y + 110, 180, 250
draw.rectangle([tft_x, tft_y, tft_x+tft_w, tft_y+tft_h], fill=(10, 30, 60), outline=(0, 180, 216), width=2)
draw.text((tft_x + tft_w//2, tft_y + 30), 'TFT 1.14"', fill=(0, 180, 216), font=font_bold, anchor='mm')
draw.text((tft_x + tft_w//2, tft_y + 55), '135 x 240', fill=(160, 174, 192), font=font_small, anchor='mm')
draw.text((tft_x + tft_w//2, tft_y + 125), 'DECODE: 20 WPM', fill=(255, 255, 255), font=font_med, anchor='mm')
draw.text((tft_x + tft_w//2, tft_y + 155), 'AUTO READY', fill=(72, 187, 120), font=font_small, anchor='mm')

# USB-C Port at bottom
draw.rectangle([esp_x + 110, esp_y + esp_h - 18, esp_x + 210, esp_y + esp_h + 10], fill=(100, 110, 125), outline=(200, 200, 200), width=1)
draw.text((esp_x + 160, esp_y + esp_h - 4), 'USB-C', fill=(255, 255, 255), font=font_small, anchor='mm')

# Board Labels
draw.text((esp_x + 160, esp_y + 40), 'LilyGO T-Display v1.1', fill=(255, 255, 255), font=font_bold, anchor='mm')
draw.text((esp_x + 160, esp_y + 65), '(ESP32)', fill=(160, 174, 192), font=font_small, anchor='mm')

# Pin header definitions (official TTGO T-Display layout)
left_pins = [
    ('3V3', True, '3.3V Power Out', (229, 62, 62)),
    ('GND', True, 'Ground', (113, 128, 150)),
    ('36', False, 'VP (Input only)', (74, 85, 104)),
    ('37', False, 'ADC (Input only)', (74, 85, 104)),
    ('38', False, 'ADC (Input only)', (74, 85, 104)),
    ('39', False, 'VN (Input only)', (74, 85, 104)),
    ('32', True, 'Paddle DIT', (72, 187, 120)),
    ('33', True, 'Paddle DAH', (236, 201, 75)),
    ('25', True, 'Encoder DT', (66, 153, 225)),
    ('26', True, 'Encoder CLK', (237, 137, 54)),
    ('27', True, 'Encoder SW', (159, 122, 234)),
]

right_pins = [
    ('GND', True, 'Ground (Buzzer -)', (113, 128, 150)),
    ('3V3', False, '3.3V Power', (74, 85, 104)),
    ('21', False, 'I2C SDA', (74, 85, 104)),
    ('22', True, 'Buzzer Plus', (183, 148, 244)),
    ('17', False, 'TX', (74, 85, 104)),
    ('2', False, 'Strapping', (74, 85, 104)),
    ('15', False, 'ADC', (74, 85, 104)),
    ('12', False, 'Avoid (Flash VDD)', (74, 85, 104)),
    ('GND', False, 'Ground', (74, 85, 104)),
    ('GND', False, 'Ground', (74, 85, 104)),
    ('5V', False, '5V (from USB)', (74, 85, 104)),
]

pin_y_start = esp_y + 110
pin_spacing = 39

left_coords = {}
right_coords = {}

# Draw Left Header
for i, (name, active, desc, col) in enumerate(left_pins):
    py = pin_y_start + i * pin_spacing
    px = esp_x + 18
    draw.rectangle([px - 10, py - 9, px + 10, py + 9], fill=(45, 55, 72), outline=col if active else (74, 85, 104), width=2 if active else 1)
    draw.ellipse([px - 3, py - 3, px + 3, py + 3], fill=(200, 200, 200))
    draw.text((px + 20, py), f'GPIO {name}' if name.isdigit() else name, fill=(255, 255, 255) if active else (113, 128, 150), font=font_bold if active else font_small, anchor='lm')
    left_coords[name] = (px - 10, py)

# Draw Right Header
for i, (name, active, desc, col) in enumerate(right_pins):
    py = pin_y_start + i * pin_spacing
    px = esp_x + esp_w - 18
    draw.rectangle([px - 10, py - 9, px + 10, py + 9], fill=(45, 55, 72), outline=col if active else (74, 85, 104), width=2 if active else 1)
    draw.ellipse([px - 3, py - 3, px + 3, py + 3], fill=(200, 200, 200))
    draw.text((px - 20, py), f'GPIO {name}' if name.isdigit() else name, fill=(255, 255, 255) if active else (113, 128, 150), font=font_bold if active else font_small, anchor='rm')
    right_coords[f'{name}_{i}'] = (px + 10, py)

# 2. Rotary Encoder KY-040 (Left side box)
enc_x, enc_y, enc_w, enc_h = 50, 350, 250, 260
draw.rectangle([enc_x, enc_y, enc_x+enc_w, enc_y+enc_h], fill=(23, 28, 38), outline=(66, 153, 225), width=2)
draw.text((enc_x + enc_w//2, enc_y + 25), 'Rotary Encoder (KY-040)', fill=(255, 255, 255), font=font_bold, anchor='mm')
draw.text((enc_x + enc_w//2, enc_y + 45), 'Menue & Speed (5-35 WPM)', fill=(160, 174, 192), font=font_small, anchor='mm')

enc_pins = [
    ('CLK', 'Orange', (237, 137, 54), 'GPIO 26'),
    ('DT',  'Blau',   (66, 153, 225),  'GPIO 25'),
    ('SW',  'Lila',   (159, 122, 234), 'GPIO 27'),
    ('+',   'Rot',    (229, 62, 62),   '3V3 (NICHT 5V!)'),
    ('GND', 'Grau',   (160, 174, 192), 'GND'),
]

enc_coords = {}
for i, (pname, color_name, pcol, esp_target) in enumerate(enc_pins):
    py = enc_y + 75 + i * 36
    draw.rectangle([enc_x + enc_w - 20, py - 8, enc_x + enc_w - 4, py + 8], fill=pcol)
    draw.text((enc_x + 12, py), f'{pname}', fill=(255, 255, 255), font=font_bold, anchor='lm')
    draw.text((enc_x + 60, py), f'-> {esp_target}', fill=pcol, font=font_small, anchor='lm')
    enc_coords[pname] = (enc_x + enc_w - 4, py)

# 3. Morse Paddle (Top Left)
pad_x, pad_y, pad_w, pad_h = 50, 105, 250, 220
draw.rectangle([pad_x, pad_y, pad_x+pad_w, pad_y+pad_h], fill=(23, 28, 38), outline=(72, 187, 120), width=2)
draw.text((pad_x + pad_w//2, pad_y + 25), 'Morse Paddle (Iambic)', fill=(255, 255, 255), font=font_bold, anchor='mm')
draw.text((pad_x + pad_w//2, pad_y + 45), '3.5mm Stereo Klinke (TRS)', fill=(160, 174, 192), font=font_small, anchor='mm')

pad_pins = [
    ('Tip (Spitze)', 'DIT', 'Gruen', (72, 187, 120), 'GPIO 32'),
    ('Ring',         'DAH', 'Gelb',  (236, 201, 75),  'GPIO 33'),
    ('Sleeve (GND)', 'GND', 'Grau',  (160, 174, 192), 'GND'),
]
pad_coords = {}
for i, (pname, func, color_name, pcol, esp_target) in enumerate(pad_pins):
    py = pad_y + 80 + i * 42
    draw.rectangle([pad_x + pad_w - 20, py - 8, pad_x + pad_w - 4, py + 8], fill=pcol)
    draw.text((pad_x + 12, py - 8), f'{pname} = {func}', fill=(255, 255, 255), font=font_bold, anchor='lm')
    draw.text((pad_x + 12, py + 10), f'({color_name}) -> {esp_target}', fill=pcol, font=font_small, anchor='lm')
    pad_coords[func] = (pad_x + pad_w - 4, py)

# 4. Buzzer (Right side box)
buz_x, buz_y, buz_w, buz_h = 880, 180, 270, 200
draw.rectangle([buz_x, buz_y, buz_x+buz_w, buz_y+buz_h], fill=(23, 28, 38), outline=(183, 148, 244), width=2)
draw.text((buz_x + buz_w//2, buz_y + 25), 'Piezo Buzzer (Passiv)', fill=(255, 255, 255), font=font_bold, anchor='mm')
draw.text((buz_x + buz_w//2, buz_y + 48), '700 Hz PWM Sidetone', fill=(160, 174, 192), font=font_small, anchor='mm')

draw.text((buz_x + 40, buz_y + 90), '(+) Plus', fill=(255, 255, 255), font=font_bold, anchor='lm')
draw.text((buz_x + 40, buz_y + 110), '-> ESP GPIO 22 (Lila)', fill=(183, 148, 244), font=font_small, anchor='lm')
draw.rectangle([buz_x + 10, buz_y + 95, buz_x + 28, buz_y + 107], fill=(183, 148, 244))

draw.text((buz_x + 40, buz_y + 145), '(-) Minus / GND', fill=(255, 255, 255), font=font_bold, anchor='lm')
draw.text((buz_x + 40, buz_y + 165), '-> ESP GND (Grau)', fill=(160, 174, 192), font=font_small, anchor='lm')
draw.rectangle([buz_x + 10, buz_y + 150, buz_x + 28, buz_y + 162], fill=(160, 174, 192))

buz_plus_coord = (buz_x + 10, buz_y + 101)
buz_gnd_coord = (buz_x + 10, buz_y + 156)

# 5. Draw Clean Orthogonal Lines
def draw_trace(p1, p2, col, width=3):
    x1, y1 = p1
    x2, y2 = p2
    mid_x = (x1 + x2) // 2
    draw.line([(x1, y1), (mid_x, y1), (mid_x, y2), (x2, y2)], fill=col, width=width)
    draw.ellipse([x1-3, y1-3, x1+3, y1+3], fill=col)
    draw.ellipse([x2-3, y2-3, x2+3, y2+3], fill=col)

# Paddle connections
draw_trace(pad_coords['DIT'], left_coords['32'], (72, 187, 120), 3)
draw_trace(pad_coords['DAH'], left_coords['33'], (236, 201, 75), 3)
draw_trace(pad_coords['GND'], (left_coords['GND'][0], left_coords['GND'][1]-3), (160, 174, 192), 2)

# Encoder connections
# CLK -> GPIO 26
draw_trace(enc_coords['CLK'], left_coords['26'], (237, 137, 54), 3)
# DT -> GPIO 25 (EINZIG und ALLEIN an GPIO 25!)
draw_trace(enc_coords['DT'], left_coords['25'], (66, 153, 225), 3)
# SW -> GPIO 27 (EINZIG und ALLEIN an GPIO 27!)
draw_trace(enc_coords['SW'], left_coords['27'], (159, 122, 234), 3)
# + -> 3V3
draw_trace(enc_coords['+'], left_coords['3V3'], (229, 62, 62), 3)
# GND -> GND
draw_trace(enc_coords['GND'], (left_coords['GND'][0], left_coords['GND'][1]+3), (160, 174, 192), 2)

# Buzzer connections
draw_trace(buz_plus_coord, right_coords['22_3'], (183, 148, 244), 3)
draw_trace(buz_gnd_coord, right_coords['GND_0'], (160, 174, 192), 2)

# Bottom Legend Bar
leg_y = 680
draw.rectangle([50, leg_y - 10, width - 50, leg_y + 50], fill=(18, 22, 28), outline=(74, 85, 104), width=1)
draw.text((70, leg_y + 8), 'KLARSTELLUNG:', fill=(236, 201, 75), font=font_bold, anchor='lm')
draw.text((215, leg_y + 8), '• DT geht NUR an GPIO 25    • CLK geht NUR an GPIO 26    • SW geht NUR an GPIO 27', fill=(255, 255, 255), font=font_med, anchor='lm')
draw.text((215, leg_y + 30), '• Encoder VCC an 3.3V (NIEMALS 5V!)    • Paddle DIT an GPIO 32, DAH an GPIO 33', fill=(160, 174, 192), font=font_small, anchor='lm')

out_path = r'C:\Users\Michael\.gemini\antigravity\scratch\morse-trainer\docs\wiring_diagram_exact.png'
img.save(out_path, 'PNG')
brain_path = r'C:\Users\Michael\.gemini\antigravity\brain\5bf3cb09-6b7d-4832-b75f-e18a9e63cd50\wiring_diagram_exact.png'
img.save(brain_path, 'PNG')
print('DONE')
