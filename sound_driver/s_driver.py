import pygame
import numpy as np
import serial
import threading
import time

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
SAMPLE_RATE = 44100
DURATION = 1.0
BUFFER_SIZE = 2048

MIDI_TO_FREQ = {
    41: 110.0, 43: 123.47, 45: 146.83,
    36: 65.41, 38: 82.41, 42: 110.0,
    46: 164.81, 49: 196.0, 51: 246.94
}

SENSOR_COLORS = {
    0: (100, 100, 255), 1: (100, 200, 255), 2: (200, 100, 255),
    3: (50, 255, 50),   4: (150, 255, 50),  5: (255, 255, 50),
    6: (255, 150, 50),  7: (255, 50, 50),   8: (255, 50, 150)
}

sensor_last_trigger = {i: 0 for i in range(100)}
sensor_values = [0.0] * 9
sensor_lock = threading.Lock()
synthesis_mode = 0

pygame.mixer.pre_init(SAMPLE_RATE, -16, 2, BUFFER_SIZE)
pygame.init()
pygame.mixer.set_num_channels(32)

screen = pygame.display.set_mode((1000, 700))
pygame.display.set_caption("🎹 LITE AUDIO ENGINE (Anti-Lag)")
clock = pygame.time.Clock()
font = pygame.font.Font(None, 36)

def generate_sound_lite(note, velocity, mode):
    freq = MIDI_TO_FREQ.get(note, 440)
    
    with sensor_lock:
        reverb_mix = sensor_values[3] * 0.3
        delay_mix = sensor_values[4] * 0.3
        brightness = sensor_values[1]
    
    samples = int(SAMPLE_RATE * DURATION)
    t = np.linspace(0, DURATION, samples)
    
    if mode == 0:
        osc = np.sin(2 * np.pi * freq * t)
        osc += 0.3 * np.sin(2 * np.pi * (freq * 2) * t)
        
        if brightness > 0.1:
            osc += brightness * 0.2 * np.sin(2 * np.pi * (freq * 3) * t)
        
        env = np.exp(-3.0 * t)
        waveform = osc * env
        
    elif mode == 1:
        mod = np.sin(2 * np.pi * (freq * 2) * t) * np.exp(-5*t)
        waveform = np.sin(2 * np.pi * freq * t + mod) * np.exp(-2*t)
        
    else:
        waveform = np.sin(2 * np.pi * freq * t) * 0.5
        waveform *= np.linspace(1, 0, samples)

    wave_l = waveform
    wave_r = waveform
    
    if reverb_mix > 0.05:
        delay_s = int(0.05 * SAMPLE_RATE)
        reverb_sig = np.zeros_like(waveform)
        reverb_sig[delay_s:] = waveform[:-delay_s] * reverb_mix
        wave_l += reverb_sig
        wave_r += reverb_sig

    if delay_mix > 0.05:
        delay_s = int(0.25 * SAMPLE_RATE)
        delay_sig = np.zeros_like(waveform)
        delay_sig[delay_s:] = waveform[:-delay_s] * delay_mix
        wave_r += delay_sig
        wave_l += delay_sig * 0.5

    vol = (velocity / 127.0) * 0.8
    wave_l *= vol
    wave_r *= vol
    
    np.clip(wave_l, -0.99, 0.99, out=wave_l)
    np.clip(wave_r, -0.99, 0.99, out=wave_r)
    
    out_l = (wave_l * 32767).astype(np.int16)
    out_r = (wave_r * 32767).astype(np.int16)
    
    return np.column_stack((out_l, out_r))

def play_sound(note, velocity, idx):
    current_time = time.time()
    
    if current_time - sensor_last_trigger.get(idx, 0) < 0.15:
        return

    try:
        sensor_last_trigger[idx] = current_time
        
        val = (velocity / 127.0)
        with sensor_lock:
            sensor_values[idx] = val

        audio = generate_sound_lite(note, velocity, synthesis_mode)
        pygame.sndarray.make_sound(audio).play()
        
    except Exception as e:
        print(f"Err: {e}")

def serial_thread():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
        print("✅ Serial OK")
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("NOTE:"):
                    p = line.replace("NOTE:", "").split(":")
                    if len(p) == 2:
                        play_sound(int(p[0]), int(p[1]), 0)
            time.sleep(0.001)
    except:
        pass

def draw():
    screen.fill((30, 30, 30))
    
    title = font.render("🎹 ANTI-CRACKLE ENGINE", True, (200, 200, 200))
    screen.blit(title, (350, 20))
    
    with sensor_lock:
        vals = list(sensor_values)
        
    for i in range(9):
        x = 50 + i * 100
        h = int(vals[i] * 300)
        col = SENSOR_COLORS[i]
        
        pygame.draw.rect(screen, (50, 50, 50), (x, 500-300, 80, 300))
        pygame.draw.rect(screen, col, (x, 500-h, 80, h))
        
        lbl = font.render(str(i), True, (255,255,255))
        screen.blit(lbl, (x+30, 510))

    pygame.display.flip()

def main():
    global synthesis_mode
    threading.Thread(target=serial_thread, daemon=True).start()
    
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN:
                if pygame.K_0 <= event.key <= pygame.K_2:
                    synthesis_mode = event.key - pygame.K_0
            if event.type == pygame.MOUSEBUTTONDOWN:
                play_sound(60 + int(event.pos[0]/50), 100, 0)
                
        draw()
        clock.tick(30)
    pygame.quit()

if __name__ == "__main__":
    main()