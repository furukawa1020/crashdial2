import serial
import time

try:
    ser = serial.Serial('COM7', 115200, timeout=1)
    print("Serial monitor started. Press Ctrl+C to exit.")
    print("=" * 50)
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)
        time.sleep(0.01)
        
except KeyboardInterrupt:
    print("\nMonitor stopped.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
