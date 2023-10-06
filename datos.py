import serial

# para saber mas como configurar el puerto serial, ver
# https://pyserial.readthedocs.io/en/latest/pyserial.html

puerto = serial.Serial('/tmp/ttyS1', 115200)
file = open('datos.csv', 'w')
file.write('V0,V1,V2,V3\n')

try:
    while True:
        datos = puerto.readline().decode('utf-8').strip()

        if datos:
            print(datos)
            datos = datos.split('\t')
            datos = [valor[3:] + ',' for valor in datos]
            file.write(''.join(datos) + '\n')

except KeyboardInterrupt:
    puerto.close()
    file.close()