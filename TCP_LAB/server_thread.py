import socket
import socketserver
from threading import Thread

# 创建对象
# 使用ipv4协议族和字节流
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = "127.0.0.1"
port = 9000

# 绑定地址
# 设置监听
server.bind((host, port))
server.listen(5)

pool = [] # 用于暂时存放需要处理的客户端


def handle_client():
    # 每次收到一个客户端有请求，九单独开一个线程
    while True:
        client, addr = server.accept()
        pool.append(client)
        t = Thread(target=handle_msg, args=(client, ))
        t.setDaemon(True)
        t.start()


def handle_msg(client):
    while True:
        data = client.recv(512).decode("utf-8")
        print("received msg:\n %s\n"% data)
        if data == 'q':
            pool.remove(client)
            client.send("Bye".encode('utf-8'))
            break
        client.send("I got it".encode('utf-8'))


t = Thread(target=handle_client)
t.setDaemon(True)
t.start()
while True:
    cmd = input("Please input cmd:")
    if cmd == 'q':
        break
    pass

server.close()
