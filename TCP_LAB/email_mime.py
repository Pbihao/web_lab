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

    msgtype = b"Content-Type:multipart/mixed;boundary='BOUNDARY'\r\n\r\n"  # 设置邮件不同部分的分隔标志即'boundary='\r\n\r\n''，不清楚需要几个\r\n，保险起见，我加了俩
    msgboundary = b'--BOUNDARY\r\n'  # 邮件不同部分要设置分隔标志，需要在前面设置的boundary之前加--
    msgmailer = b'X-Mailer:mengqi\'s mailer\r\n'  # 设置的邮件发送端，大概这个意思
    msgMIMI = b'MIME-Version:1.0\r\n'  # MIMEvision设置，无论是什么版本，这里都写1.0
    msgfileType = b"Content-type:image/gif;\r\n"  # 传送的文件类型是图片
    msgfilename = b"Content-Disposition: attachment; filename='%s'\r\n" % "timg.jpeg".encode('utf-8')  # 字符串需要encode，因为socket只能传送byte（字节码）
    msgfileID = b'Content-ID:<image1>\r\n'  # 给传送的文件定义的id，在邮件里插入图片时会用到

    # 开始传输数据
    client.send(head_from)
    client.send(head_to)
    client.send(subject)
    client.send(msgtype)
    client.send(b'Content-Transfer-Encoding:7bit\r\n\r\n')

    # 发送图片

    client.send(b'\r\n\r\n' + msgboundary)  # 发送分割标志
    client.send(b'Content-Type:text/html;\r\n')  # text/html类型的，可以插入图片
    client.send(b'Content-Transfer-Encoding:7bit\r\n\r\n')  # 需要7bit格式传输
    client.send(b'test for sending image')
    client.send(b'<img src="cid:image1">')  # 插入图片，html代码一样，只不过这个cid必须跟下面那个msgfileID一样

    client.send(b'\r\n\r\n' + msgboundary)  # 发送分割标志
    client.send(msgfileType)  # 发送要插入的图片
    client.send(msgfileID)
    client.send(msgfilename)
    client.send(b'Content-Transfer-Encoding:base64\r\n\r\n')  # 这里必须base64编码
    client.send(b'\r\n')

    with open(IMAGE_PATH, 'rb') as fp:
        while True:
            data = fp.read(1024)
            if not data:
                break
            client.send(base64.b64encode(data))

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
