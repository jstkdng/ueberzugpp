import socket
import time
import os

cmd1 = """
{"action":"add","identifier":"preview","max_height":21,"max_width":118,"path":"/tmp/img1.png","x":10,"y":15}
"""

cmd2 = """
{"action":"add","identifier":"preview","max_height":47,"max_width":118,"path":"/tmp/img2.png","x":10,"y":15}
"""

cmd_exit = """
{"action":"remove","identifier":"preview"}
"""

s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
connected = False
while not connected:
    try:
        s.connect(f"/tmp/ueberzug_{os.getenv('USER')}.sock")
        connected = True
    except Exception:
        time.sleep(0.05)

s.sendall(str.encode(cmd1 + '\n'))
time.sleep(2)
s.sendall(str.encode(cmd_exit + '\n'))
s.sendall(str.encode(cmd2 + '\n'))
time.sleep(2)
s.sendall(str.encode(cmd_exit + '\n'))
s.close()
