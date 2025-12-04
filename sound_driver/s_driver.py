import pygame
import numpy as np
import serial
import threading
import time
import math

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
SAMPLE_RATE = 44100 
DURATION = 2.0     
BUFFER_SIZE = 4096 

MIDI_TO_FREQ = {
    41: 110.0, 43: 123.47, 45: 146.83,
    36: 65.41, 38: 82.41, 42: 110.0,
    46: 164.81, 49: 196.0, 51: 246.94
}

NOTE_TO_INDEX = {
    41: 2, 43: 0, 45: 1,
    36: 4, 38: 3, 42: 5,
    46: 6, 49: 7, 51: 8
}

SENSOR_COLORS = {
    0: (0, 255, 255),   1: (50, 200, 255),  2: (100, 150, 255),
    3: (50, 255, 50),   4: (150, 255, 50),  5: (255, 255, 50),
    6: (255, 150, 0),   7: (255, 50, 50),   8: (255, 50, 150)
}

SENSOR_POSITIONS = {
    0: (-35, -55), 1: (-35, 65), 2: (65, 5),
    8: (0, -260), 7: (230, -140), 6: (230, 140),
    5: (0, 260), 4: (-230, 140), 3: (-230, -140)
}

sensor_last_trigger = {i: 0 for i in range(100)}
sensor_values = [0.0] * 9
sensor_lock = threading.Lock()
synthesis_mode = 0 

pygame.mixer.pre_init(frequency=SAMPLE_RATE, size=-16, channels=2, buffer=BUFFER_SIZE)
pygame.init()
pygame.mixer.set_num_channels(64)

screen = pygame.display.set_mode((800, 800))
pygame.display.set_caption("🎵 HANG DRUM (NOISE FREE)")
clock = pygame.time.Clock()
font = pygame.font.Font(None, 36)

def generate_hang_sound(note, velocity, mode):
    freq = MIDI_TO_FREQ.get(note, 440)
    
    with sensor_lock:
        reverb_mix = sensor_values[3] * 0.3
        brightness = sensor_values[1]
        resonance = sensor_values[0]
    
    samples = int(SAMPLE_RATE * DURATION)
    t = np.linspace(0, DURATION, samples)
    
    if mode == 0:
        decay = 2.5 - resonance
        osc_fund = np.sin(2 * np.pi * freq * t) * np.exp(-decay * t)
        osc_oct = 0.5 * np.sin(2 * np.pi * (freq * 2) * t) * np.exp(-(decay+1) * t)
        osc_fifth = 0.3 * np.sin(2 * np.pi * (freq * 3) * t) * np.exp(-(decay+2) * t)
        
        slap = 0.1 * np.random.normal(0, 1, samples) * np.exp(-50 * t)
        waveform = osc_fund + osc_oct + osc_fifth + slap

    elif mode == 1:
        osc1 = np.sin(2 * np.pi * freq * t)
        osc2 = 0.6 * np.sin(2 * np.pi * (freq * 2.02) * t)
        osc3 = 0.4 * np.sin(2 * np.pi * (freq * 3.98) * t)
        waveform = (osc1 + osc2 + osc3) * np.exp(-4.0 * t)

    else:
        osc = np.sin(2 * np.pi * freq * t)
        lfo = 0.9 + 0.1 * np.sin(2 * np.pi * 5.0 * t) 
        waveform = osc * lfo * np.exp(-1.5 * t)

    fade_in = int(0.01 * SAMPLE_RATE)
    waveform[:fade_in] *= np.linspace(0, 1, fade_in)
    
    wave_l = waveform
    wave_r = waveform
    
    if reverb_mix > 0.05:
        delay_s = int(0.1 * SAMPLE_RATE)
        echo = np.zeros_like(waveform)
        echo[delay_s:] = waveform[:-delay_s] * reverb_mix
        wave_l += echo
        wave_r += echo * 0.9 

    vol = (velocity / 127.0) * 0.6
    wave_l *= vol
    wave_r *= vol
    
    wave_l = np.tanh(wave_l)
    wave_r = np.tanh(wave_r)
    
    out_l = (wave_l * 32000).astype(np.int16)
    out_r = (wave_r * 32000).astype(np.int16)
    
    return np.column_stack((out_l, out_r))

def play_sound(note, velocity, idx):
    if time.time() - sensor_last_trigger.get(idx, 0) < 0.15: return

    try:
        sensor_last_trigger[idx] = time.time()
        with sensor_lock:
            sensor_values[idx] = velocity / 127.0
            
        audio = generate_hang_sound(note, velocity, synthesis_mode)
        pygame.sndarray.make_sound(audio).play()
    except Exception as e:
        print(f"Err: {e}")

def serial_thread():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
        print("✅ Connected")
        while True:
            if ser.in_waiting:
                line = ser.readline().decode(errors='ignore').strip()
                if line.startswith("NOTE:"):
                    p = line.replace("NOTE:", "").split(":")
                    if len(p) == 2:
                        note = int(p[0])
                        vel = int(p[1])
                        idx = NOTE_TO_INDEX.get(note, 0)
                        play_sound(note, vel, idx)
            time.sleep(0.001)
    except: pass

def draw():
    screen.fill((20, 20, 30))
    CX, CY = 400, 400
    
    pygame.draw.circle(screen, (40, 40, 50), (CX, CY), 320)
    pygame.draw.circle(screen, (80, 80, 100), (CX, CY), 320, 2)
    
    modes = ["Classical Hang", "Steel Pan", "Space Drum"]
    txt = font.render(f"Mode: {modes[synthesis_mode]}", True, (150, 150, 150))
    screen.blit(txt, (20, 20))
    
    for idx, pos in SENSOR_POSITIONS.items():
        x = CX + pos[0]
        y = CY + pos[1]
        
        hit_time = time.time() - sensor_last_trigger.get(idx, 0)
        is_active = hit_time < 0.4
        
        base_col = (60, 60, 70)
        target_col = SENSOR_COLORS.get(idx, (255,255,255))
        
        if is_active:
            fade = 1.0 - (hit_time / 0.4)
            col = (
                int(base_col[0] + (target_col[0]-base_col[0])*fade),
                int(base_col[1] + (target_col[1]-base_col[1])*fade),
                int(base_col[2] + (target_col[2]-base_col[2])*fade)
            )
            r = 35 + int(15 * fade)
        else:
            col = base_col
            r = 35
            
        pygame.draw.circle(screen, col, (x, y), r)
        pygame.draw.circle(screen, (140, 140, 140), (x, y), r, 2)
        
        txt = font.render(str(idx), True, (200, 200, 200))
        screen.blit(txt, (x-8, y-10))

    pygame.display.flip()

def check_click(pos):
    CX, CY = 400, 400
    mx, my = pos
    for idx, (px, py) in SENSOR_POSITIONS.items():
        if math.hypot(mx - (CX+px), my - (CY+py)) < 40:
            note = [k for k, v in NOTE_TO_INDEX.items() if v == idx][0]
            play_sound(note, 100, idx)
            return

def main():
    global synthesis_mode
    threading.Thread(target=serial_thread, daemon=True).start()
    
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT: running = False
            if event.type == pygame.KEYDOWN:
                if pygame.K_0 <= event.key <= pygame.K_2:
                    synthesis_mode = event.key - pygame.K_0
            if event.type == pygame.MOUSEBUTTONDOWN:
                check_click(event.pos)
                
        draw()
        clock.tick(60)
    pygame.quit()

if __name__ == "__main__":
    main()
