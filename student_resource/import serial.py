import serial
import pyautogui                                        
import time

     
bluetooth_port = "COM8 "
baud_rate = 115200

ser = serial.Serial(bluetooth_port, baud_rate)
time.sleep(2)
print("Connected to ESP32 Bluetooth!")

while True:
    if ser.in_waiting > 0:
        data = ser.readline().decode().strip()
        print("Received:", data)
        if "JUMP" in data:
            pyautogui.press('space')
            print("🦖 Jumped!")
