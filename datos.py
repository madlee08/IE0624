import serial

# para saber mas como configurar el puerto serial, ver
# https://pyserial.readthedocs.io/en/latest/pyserial.html

puerto = serial.Serial('/dev/ttyACM0', 115200)

try:
    while True:
        datos = puerto.read().decode('utf-8', 'ignore')

        if datos:
            print(datos, end='')

except KeyboardInterrupt:
    puerto.close()
