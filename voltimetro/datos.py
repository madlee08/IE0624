import serial

puerto = serial.Serial("/tmp/ttyS1", 115200)

file = open('datos.csv', "a")
file.write("V0,V1,V2,V3" + "\n")

try:
    while True:
        datos = puerto.readline().decode('utf-8').strip()

        if datos:
            print(datos)           
            datos = datos.split("\t")
            datos = [valor[3:] + ',' for valor in datos]
            file.write(''.join(datos) + "\n")
        else: file.close()
except KeyboardInterrupt:
    puerto.close()
    file.close()