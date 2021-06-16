'''
Authored by Pose AI Ltd, 2021

Simple demo of a UDP server for Pose Camera in python.
Once connected will receive stream of poses from the app

Apache License 2.0
'''


import socket
import json

PORT_NUM = 8080


''' Prints your local IP address.  Configure this in the App.
  Make sure your router and firewall do not block the port '''
def show_my_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('10.255.255.255', 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    print("Connect the app to IP:", ip, "Port:", PORT_NUM)


# The handshake configures the app
handshake = json.dumps(
    {
        'HANDSHAKE': {
            'name': 'PythonDemo',  # will be displayed in the app
            'rig': 'UE4',  # Options: 'UE4', 'Mixamo'
            'mode': 'Desktop',  # Options: 'Room', 'Desktop', 'Portrait', 'RoomBodyOnly', 'PortraitBodyOnly'
            'mirror': 'YES',  # Options: 'YES', 'NO'
            'syncFPS': 60, # App smooths processed frames to constant FPS.  0 for async mode (will still smooth joints between processed frames)
            'cameraFPS': 60, # Tries to set camera to this speed. 30 and 60 work on most iphone cameras
        }
    }
).encode()

show_my_ip()

connections = {}
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('0.0.0.0', PORT_NUM))

while True:
    message, address = server_socket.recvfrom(65304)
    message = message.decode('utf-8')

    decoded_message = json.loads(message)

    if 'sessionUUID' in decoded_message.keys():
        # is initial connection, send it the handshake
        print(decoded_message, address)
        connections[decoded_message['sessionUUID']] = address
        server_socket.sendto(handshake, address)
    else:
        # message holds pose data, do what you want with it
        print(decoded_message)
