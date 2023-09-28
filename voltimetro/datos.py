import serial

puerto = serial.Serial('COM2', 115200)

try:
    while True:
        datos = puerto.readline().decode('utf-8').strip()

        if datos:
            print(datos)
except KeyboardInterrupt:
    puerto.close()