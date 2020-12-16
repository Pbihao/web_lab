import socket

# 创建对象
# 使用ipv4协议族和字节流
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = "127.0.0.1"
port = 9000

server.bind((host, port))

server.listen(5)

client_socket, addr = server.accept()


while True:
    msg = client_socket.recv(512)
    data = msg.decode("utf-8")
    print("received message: \n%s\n" % data)

    msg = input("Please input message you want to send:\n")

    if msg == "q":
        break

    client_socket.send(msg.encode("utf-8"))

server.close()
