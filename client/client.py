import asyncio
from bleak import BleakClient
from pynput import keyboard
import time

import sys, termios, tty   # <-- added

ESP32_ADDRESS = "EE45AFF1-8222-AB43-B495-4F92BD6EEDAD"
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# Disable terminal echo --------------
fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(fd)
tty.setcbreak(fd)
# ------------------------------------

# Global "current key"
current_key = 'n'

def on_press(key):
    global current_key
    try:
        match key.char:
            case 'w':
                current_key = 'w'
            case 's':
                current_key = 's'
    except AttributeError:
        if key == keyboard.Key.up: 
            current_key = 'u'
        elif key == keyboard.Key.down: 
            current_key = 'd'
        elif key == keyboard.Key.enter:
            current_key = 'e'

def on_release(key):
    global current_key
    current_key = 'n'

listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

async def main():
    print("Connecting...")
    async with BleakClient(ESP32_ADDRESS) as client:
        print("Connected!")

        while True:
            data = bytearray([ord(current_key)])
            await client.write_gatt_char(CHAR_UUID, data, response=False)
            await asyncio.sleep(0.01)  # 10 ms loop

    # Restore terminal settings on exit
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

asyncio.run(main())

