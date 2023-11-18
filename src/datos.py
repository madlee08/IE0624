import paho.mqtt.client as mqtt
import serial

# para saber mas como configurar mqtt, ver
# https://pypi.org/project/paho-mqtt/#usage-and-api

# para saber mas como configurar el puerto serial, ver
# https://pyserial.readthedocs.io/en/latest/pyserial.html


BROKER="iot.eie.ucr.ac.cr"
PORT = 1883
KEEPALIVE = 10
TOPIC='v1/devices/me/telemetry'
TOKEN = "2xxl57r9opxaqbe1ejpu"
DEVICE = '/dev/ttyACM0'
BAUDRATE = 115200

def on_connect(client, userdata, flags, rc) :
    if (rc==0) :
        print(f"Conexión exitosa a {BROKER}.")
    else :
        print(f"Conexión fallida, Código de referencia: {rc}")


def on_disconnect(client, userdata, rc): 
   print(f"Se ha desconectado de {BROKER} exitosamente.") 


puerto = serial.Serial(DEVICE, BAUDRATE)

client = mqtt.Client()
client.on_connect = on_connect
client.on_disconnect = on_disconnect

client.username_pw_set(TOKEN)
client.connect(BROKER, PORT, KEEPALIVE)

client.loop()

try:
    while True:
        inp = input()

        if 'lumos' in inp:
            s = '{'
            data = inp.split(":")
            s += inp
            s += ','
            for i in range(4):
                s += input()
                s += ','
            s = s[0:-2]
            s += '}'
            print(s)
            client.publish(TOPIC, s)

except KeyboardInterrupt:
    puerto.close()
    client.disconnect()
