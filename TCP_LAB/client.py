import socket

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

host = "127.0.0.1"
port = 9000

client.connect((host, port))

while True:
    msg = input("Please input what you want to send:\n")

    if msg == "q":
        break

    client.send(msg.encode("utf-8"))
    msg = client.recv(512)
    print("received message:\n%s\n" % msg.decode("utf-8"))


client.close()