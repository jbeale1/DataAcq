#!/usr/bin/env python3

"""
capMotion.py  -  Frame-difference motion detector for Pi camera
Runs OK on Pi 3 at 6 fps with 1280x720 resolution.

Saves a JPEG still of the peak-motion frame when significant motion is detected.
Uses dynamic threshold based on measured noise floor.
Excludes illumination-change frames (clouds) from noise floor measurement.

2026-03-26  J.Beale  v1.0
  Diagnostic printout of changed pixels and noise floor for tuning.  
  Added brightness-jump detection to exclude clouds from noise floor.  
  Tuned parameters for good sensitivity with low false triggers.  
"""

import cv2
import numpy as np
from picamera2 import Picamera2
from datetime import datetime
import time
import os

# ── tunables ──────────────────────────────────────────────────────────────────
RESOLUTION       = (1280, 720)
FRAMERATE        = 6
DIFF_THRESHOLD   = 25        # per-pixel abs difference to count as changed
COOLDOWN_SEC     = 2.0       # min seconds between saved frames
OUTPUT_DIR       = "/home/pi/motion_frames"
BLUR_KSIZE       = 5         # Gaussian blur kernel for noise reduction
SCALE            = 0.5       # analyse at half resolution
WINDOW_FRAMES    = 8         # max frames to buffer before forcing a save
BACKGROUND_ALPHA = 0.05      # background adaptation rate (~12 sec time constant @ 8fps)
NOISE_SAMPLES    = 64        # quiet frames used to measure noise floor
NOISE_MULTIPLIER = 10        # MIN_PIXELS = this * 95th-percentile of noise floor
MAX_MIN_PIXELS   = 5000      # hard ceiling on MIN_PIXELS — vehicles always exceed this
BRIGHTNESS_JUMP  = 0.05      # fraction change in mean brightness that flags illumination event
DIAGNOSTIC       = True      # print changed_px every frame — set False once tuned
# ─────────────────────────────────────────────────────────────────────────────

AW = int(RESOLUTION[0] * SCALE)
AH = int(RESOLUTION[1] * SCALE)


def normalize_brightness(gray):
    mean = np.mean(gray)
    if mean < 1:
        return gray
    return np.clip(gray * (128.0 / mean), 0, 255).astype(np.uint8)

def to_gray_blurred(frame_bgr):
    small = cv2.resize(frame_bgr, (AW, AH))
    gray  = cv2.cvtColor(small, cv2.COLOR_BGR2GRAY)
    gray  = normalize_brightness(gray)
    return cv2.GaussianBlur(gray, (BLUR_KSIZE, BLUR_KSIZE), 0)

def count_motion_pixels(background, gray):
    diff = cv2.absdiff(background, gray)
    return int(np.count_nonzero(diff > DIFF_THRESHOLD))

def capture_bgr():
    """Capture a frame and return it as a proper BGR array."""
    raw = picam2.capture_array()   # XRGB8888: 4 channels, order is B,G,R,X
    return raw[:, :, :3]           # drop the X channel — remaining is BGR

def save_best(window, label):
    best_px, best_frame = max(window, key=lambda x: x[0])
    ts    = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
    fname = os.path.join(OUTPUT_DIR, f"motion_{ts}.jpg")
    cv2.imwrite(fname, best_frame, [cv2.IMWRITE_JPEG_QUALITY, 90])
    print(f"  saved {fname}  ({label}, peak {best_px} px, {len(window)} frames)")

os.makedirs(OUTPUT_DIR, exist_ok=True)

picam2 = Picamera2()
config = picam2.create_video_configuration(
    main={"size": RESOLUTION, "format": "XRGB8888"},
    controls={"FrameRate": FRAMERATE}
)
picam2.configure(config)
picam2.start()

# Let AWB and AEC converge
print("Letting AWB/AEC settle...")
time.sleep(3)
metadata = picam2.capture_metadata()
print(f"  ColourGains: R={metadata['ColourGains'][0]:.3f}  B={metadata['ColourGains'][1]:.3f}")
print(f"  ExposureTime: {metadata.get('ExposureTime')}  AnalogueGain: {metadata.get('AnalogueGain'):.2f}")

# Flush any stale frames before building background
print("Flushing camera buffer...")
for _ in range(8):
    picam2.capture_array()
time.sleep(0.5)

# Build background reference
print("Building background reference...")
bg_frames  = [to_gray_blurred(capture_bgr()) for _ in range(16)]
background = np.median(np.stack(bg_frames), axis=0).astype(np.uint8)
print("  Background reference built.")

last_saved    = 0.0
window        = []      # list of (changed_px, frame_bgr)
in_event      = False
noise_history = []
MIN_PIXELS    = 9999    # will be set automatically after warmup
prev_mean     = None    # for brightness-jump detection

print(f"Motion detector running  →  saving to {OUTPUT_DIR}")
print(f"  Measuring noise floor for {NOISE_SAMPLES} quiet frames before arming...")

try:
    while True:
        frame = capture_bgr()
        gray  = to_gray_blurred(frame)

        changed_px = count_motion_pixels(background, gray)

        # Detect illumination jumps (clouds) by monitoring mean brightness
        curr_mean          = float(np.mean(gray))
        brightness_stable  = (prev_mean is None or
                               abs(curr_mean - prev_mean) / max(prev_mean, 1.0) < BRIGHTNESS_JUMP)
        prev_mean          = curr_mean

        # Only update noise history during quiet, brightness-stable frames
        if not in_event and brightness_stable:
            noise_history.append(changed_px)
            if len(noise_history) > NOISE_SAMPLES:
                noise_history.pop(0)
            if len(noise_history) >= NOISE_SAMPLES:
                noise_95   = np.percentile(noise_history, 95)
                MIN_PIXELS = min(MAX_MIN_PIXELS,
                                 max(200, int(noise_95 * NOISE_MULTIPLIER)))

        triggered = (len(noise_history) >= NOISE_SAMPLES) and (changed_px >= MIN_PIXELS)

        if DIAGNOSTIC:
            armed  = len(noise_history) >= NOISE_SAMPLES
            stable = '' if brightness_stable else ' ILLUM'
            print(f"  px={changed_px:6d}  min={MIN_PIXELS:6d}  "
                  f"{'ARMED' if armed else 'warming up':10s}{stable}"
                  f"  {'TRIGGERED' if triggered else ''}")

        now = time.monotonic()

        if triggered:
            in_event = True
            window.append((changed_px, frame.copy()))

        elif in_event:
            # Event just ended — save peak frame
            if window and (now - last_saved) >= COOLDOWN_SEC:
                save_best(window, "event end")
                last_saved = now
            window.clear()
            in_event = False

        # Always adapt background on brightness-stable frames, regardless of trigger state
        if brightness_stable:
            background = cv2.addWeighted(background, 1.0 - BACKGROUND_ALPHA,
                                         gray, BACKGROUND_ALPHA, 0)


        # Safety valve: flush window if event runs too long
        if in_event and len(window) >= WINDOW_FRAMES:
            if (now - last_saved) >= COOLDOWN_SEC:
                save_best(window, "window flush")
                last_saved = now
            window.clear()

except KeyboardInterrupt:
    print("Stopped.")
finally:
    picam2.stop()
