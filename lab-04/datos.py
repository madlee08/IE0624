import paho.mqtt.client as mqtt
import serial

BROKER="iot.eie.ucr.ac.cr"
PORT = 1883
KEEPALIVE = 10
TOPIC='v1/devices/me/telemetry'
TOKEN = "4wr1d4p30ozyeirt5srr"
DEVICE = '/dev/ttyACM0'
BAUDRATE = 115200

def on_connect(client, userdata, flags, rc) :
    if (rc==0) :
        print(f"Conexión exitosa a {BROKER}.")
    else :
        print(f"Conexión fallida, Código de referencia: {rc}")


def on_disconnect(client, userdata, rc): 
   print(f"Se ha desconectado de {BROKER} exitosamente.") 


client = mqtt.Client()
client.on_connect = on_connect
client.on_disconnect = on_disconnect

client.username_pw_set(TOKEN)
client.connect(BROKER, PORT, KEEPALIVE)

client.loop()

# para saber mas como configurar el puerto serial, ver
# https://pyserial.readthedocs.io/en/latest/pyserial.html

puerto = serial.Serial(DEVICE, BAUDRATE)

try:
    while True:
        datos = puerto.readline().decode('utf-8', 'ignore')
        
        if datos and datos.startswith('Bateria'):
            print(datos, end='')
            client.publish(TOPIC, '{' + datos.replace('\t', ',').removesuffix(',\r\n') + '}')

except KeyboardInterrupt:
    puerto.close()
    client.disconnect()

