import socket
import base64
import os
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

IMAGE_PATH = "/home/pbihao/Pictures/Wallpapers/timg.jepg"

def start():
    subject = "Subject: Pbihao log in!\r\n".encode()
    contenttype = 'text/plain'
    msg = '\r\n Good morning, every one!\n'.encode()
    head_from = 'from:<1435343052@qq.com>\r\n'.encode()
    head_to = 'to:<1435343052@qq.com>\r\n'.encode()
    endmsg = '\r\n.\r\n'.encode()

    # 使用QQ邮箱进行本次试验
    mailServer = ("smtp.qq.com", 587)
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(mailServer)  # 建立链接

    recv = client.recv(1024).decode()
    print(recv)
    if recv[:3] != '220':
        print('220 reply not received from server')

    heloCommand = 'HELO Alice\r\n'
    client.send(heloCommand.encode())
    recv1 = client.recv(1024).decode()
    print(recv1)
    if recv1[:3] != '250':
        print('250 reply not received from server')

    user_name = '1435343052@qq.com'
    user_password = 'kwlxbfctfzsthhai'

    user_info = ('\x00' + user_name + '\x00' + user_password).encode()
    # 使用base64进行加密
    user_info = base64.b64encode(user_info)
    authMsg = "AUTH PLAIN ".encode()
    authMsg += user_info
    authMsg += '\r\n'.encode()

    # 发送用户验证信息
    client.send(authMsg)
    recv2 = client.recv(1024).decode()
    print(recv2)

    # 发送邮件作者
    mailFrom = 'MAIL FROM:<1435343052@qq.com>\r\n'.encode()
    client.send(mailFrom)
    recv3 = client.recv(1024).decode()
    print(recv3)

    # 发送邮件去往哪里
    recpTo = 'RCPT TO:<1435343052@qq.com>\r\n'.encode()
    client.send(recpTo)
    recv4 = client.recv(1024).decode()
    print(recv4)

    # 发送数据
    data = 'DATA\r\n'.encode()
    client.send(data)
    recv5 = client.recv(1024).decode()
    print(recv5)

    # 开始传输数据
    client.send(head_from)
    client.send(head_to)
    client.send(subject)
    client.send(msg)

    # 发送图片


    # image = "\r\n<img src='https://ss2.bdstatic.com/70cFvnSh_Q1YnxGkpoWK1HF6hhy/it/u=4273598416,2058485762&fm=26&gp=0.jpg'/>".encode()
    # client.send(image)
    with open(IMAGE_PATH, 'rb') as fp:
        data = fp.read(1024)
        client.send(data)

        fp.close()

    client.send(endmsg)
    recv6 = client.recv(1024).decode()
    print(recv6)

    # 退出链接
    cut = "QUIT\r\n".encode()
    client.send(cut)
    recv7 = client.recv(1024).decode()
    print(recv7)

    client.close()


if __name__ == "__main__":
    start()
