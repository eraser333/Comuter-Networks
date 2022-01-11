import socket 
import sys
import re

class Client:
    def __init__(self):
        self.connectionFd = socket.socket()
        self.dataFd = socket.socket()
        self.listenFd = socket.socket()
        self.dataPort = -100
        self.connType = "NORMAL"
        self.isConnected = False
        # 设置服务器的IP地址
        self.serverIP = "166.111.80.66" #"166.111.80.66" 
        self.serverPort = 21
        # client
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        self.clientIP =ip
        # username
        self.username = "ssast2021"
        # self.username = "anonymous"
        self.password = "%SSAST!Fall42"
        # file
        self.fileStartPoint = 0

    def get_statuscode(self, return_string):
        try:
            status_code = int(return_string.split(" ")[0])
            return status_code
        except:
            return 1000

    def basic_send_receive(self, request):
        try:
            self.connectionFd.send(request.encode("UTF-8", "strict"))
            return_string = self.connectionFd.recv(8192).decode("UTF-8", "strict")
            return return_string
        except:
            print("receive error")
            return "error"


    def login(self):
        # 初始登录的时候默认为普通登录模式
        try:
            self.connectionFd.connect((self.serverIP, self.serverPort))
        except:
            sys.stdout.write("connect error1")
            return False

        return_step1 = self.connectionFd.recv(8192).decode("UTF-8", "strict")


        request_user = "USER " + self.username +"\r\n"
        return_string1 = self.basic_send_receive(request_user)
        if self.get_statuscode(return_string1) not in range(200,400):
            sys.stdout.write("unexpect status code")
            return False


        requestPassword = "PASS " + self.password + "\r\n"
        return_string2 = self.basic_send_receive(requestPassword)
        if self.get_statuscode(return_string2) not in range(200,400):
            sys.stdout.write("unexpect status code")
            return False
        
        return True, return_step1, return_string1, return_string2
    

    def quit(self):
        return_string = self.basic_send_receive("QUIT\r\n")
        return return_string

    def abor(self):
        return_string = self.basic_send_receive("ABOR\r\n")
        return return_string


    def pasv(self):
        requestString = "PASV \r\n"
        return_string = self.basic_send_receive(requestString)
        if self.get_statuscode(return_string) not in range(200,400):
            sys.stdout.write("PASV error1")
            return False
        
        try:
            self.listenFd.close()
            self.listenFd = socket.socket()
            self.dataFd.close()
            self.dataFd = socket.socket()
            tempList =  re.split(r"[(,)]", return_string)
            self.dataPort = int(tempList[5])*256 + int(tempList[6])
            
            self.connType = "PASV"
        except:
            sys.stdout.write("PASV error2")
            return False
        
        return True, return_string


    def port(self, inputDataPort):
        inputDataPort = int(inputDataPort)
        if inputDataPort not in range(20000, 65536):
            sys.stdout.write("inputDataPort error")
            return False
        port0 = int(inputDataPort / 256)
        port1 = inputDataPort % 256

        ipList = self.clientIP.replace(".", ",")
        request = "PORT " + ipList + "," + str(port0) +","+ str(port1) + "\r\n"
        
        self.listenFd.close()
        self.listenFd = None
        self.listenFd = socket.socket()
        self.dataFd.close()
        self.dataFd = None
        self.dataFd = socket.socket() 
        try:
            self.listenFd.bind((self.clientIP, inputDataPort))
            self.listenFd.listen(5)
        except:
            return False

        self.connType = "PORT"
        self.isConnected = False
        return_string = self.basic_send_receive(request)    

        if self.get_statuscode(return_string) not in range(200, 400):
            sys.stdout.write("PORT error2")
            self.return_normal()
            return False

        return True, return_string


    def retr(self, filePath, local_filepath):
        request = "RETR " + filePath + "\r\n"
        self.connectionFd.send(request.encode("UTF-8", "strict"))

        if self.connType == "PORT":
            if self.isConnected == False:
                self.dataFd, Addr = self.listenFd.accept()
                self.isConnected = True
        elif self.connType == "PASV":
            if self.isConnected == False:
                self.dataFd.connect((self.serverIP, self.dataPort))
                self.isConnected = True
        else:
            return_string = self.connectionFd.recv(8192).decode('UTF-8','strict')
            sys.stdout.write(return_string)
            print("connType error")
            self.return_normal()
            return
        
        return_string1 = self.connectionFd.recv(8192).decode('UTF-8','strict')
        if self.get_statuscode(return_string1) != 150:
            print("retr error1")
            return

        filename = filePath.split("/")[-1]
        try:
            if self.fileStartPoint == 0:
                File = open(local_filepath + "/" + filename, "wb+")
            else:
                File = open(local_filepath + "/" + filename, "rb+")
            File.seek(self.fileStartPoint, 0)
            self.fileStartPoint = 0
        except:
            print("retr error2")
            return
        
        while(1):
            receive_file_data = self.dataFd.recv(8192)
            if len(receive_file_data) == 0 or not receive_file_data or receive_file_data == None:
                break
            else:
                try:
                    File.write(receive_file_data)
                except:
                    print("retr error4")
                    return
                
        File.close()
        self.return_normal()
        return_string2 = self.connectionFd.recv(8192).decode("UTF-8", "strict")

        return return_string1, return_string2
        

    def stor(self, filepath):
        filename = filepath.split("/")[-1]
        request = "STOR " + filename + "\r\n"
        self.connectionFd.send(request.encode("UTF-8", "strict"))
        

        if self.connType == "PORT":
            if self.isConnected == False:
                self.dataFd, Addr = self.listenFd.accept()
                self.isConnected = True
        elif self.connType == "PASV":
            if self.isConnected == False:
                self.dataFd.connect((self.serverIP, self.dataPort))
                self.isConnected = True
        else:
            return_string = self.connectionFd.recv(8192).decode('UTF-8','strict')
            sys.stdout.write(return_string)
            print("connType error")
            self.return_normal()
            return
        
        return_string1 = self.connectionFd.recv(8192).decode('UTF-8','strict')
        if self.get_statuscode(return_string1) != 150:
            print("stor error1")
            self.return_normal()
            return
        
        file = open(filepath, "rb")
        file.seek(self.fileStartPoint, 0)
        self.fileStartPoint = 0

        dataSend = file.read()
        dataSend += b'\0'

        self.dataFd.sendall(dataSend)

        self.return_normal()

        return_string2 = self.connectionFd.recv(8192).decode("UTF-8", "strict")

        return return_string1, return_string2
        

    def list(self, path):
        request = "LIST " + path + "\r\n"
        self.connectionFd.send(request.encode("UTF-8", "strict"))

        if self.connType == "PORT":
            if self.isConnected == False:
                self.dataFd, Addr= self.listenFd.accept()
                self.isConnected = True
        elif self.connType == "PASV":
            if self.isConnected == False:
                self.dataFd.connect((self.serverIP, self.dataPort))
                self.isConnected = True
        else:
            return_string1 = self.connectionFd.recv(8192).decode('UTF-8','strict')
            sys.stdout.write(return_string1)
            print("connType error")
            return False
        
        return_string1 = self.connectionFd.recv(8192).decode('UTF-8','strict')
        if self.get_statuscode(return_string1) != 150:
            print("list error1")
            return False
        
        receive_data = self.dataFd.recv(8192).decode('UTF-8','strict')

        return_string2 = self.connectionFd.recv(8192).decode('UTF-8','strict')

        self.return_normal()
        return True, return_string1, return_string2, receive_data

    def return_normal(self):
        self.dataFd.close()
        self.dataFd = socket.socket()

        self.listenFd.close()
        self.listenFd = socket.socket()

        self.connType = "NORMAL"
        self.isConnected = False

    def syst(self):
        return_string = self.basic_send_receive("SYST\r\n")
        return return_string

    def cdup(self):
        return_string = self.basic_send_receive("CDUP\r\n")
        return return_string

    def pwd(self):
        return_string = self.basic_send_receive("PWD\r\n")
        return return_string

    def cwd(self, path):
        request = "CWD " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def rmd(self, path):
        request = "RMD " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def mkd(self, path):
        request = "MKD " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def rnfr(self, path):
        request = "RNFR " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def rnto(self, path):
        request = "RNTO " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def type(self, chosen_type):
        request = "TYPE " + chosen_type + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string


    def dele(self, path):
        request = "DELE " + path + "\r\n"
        return_string = self.basic_send_receive(request)
        return return_string

    def modify_basic(self, IP, port, username, password):
        self.serverIP = IP
        self.serverPort = port
        self.username = username
        self.password = password

if __name__ == "__main__":
    client = Client()
    client.login()
    while 1:
        sys.stdout.write("Please input your command\n")
        command = sys.stdin.readline()[:-1]

        justOneCommand = {"QUIT": client.quit, 
                          "ABOR":client.abor, 
                          "PASV":client.pasv, 
                          "SYST":client.syst, 
                          "CDUP":client.cdup, 
                          "PWD":client.pwd}

        needTwoCommand = {"PORT":client.port, 
                          "STOR":client.stor, 
                          "RETR":client.retr, 
                          "LIST":client.list, 
                          "CWD":client.cwd, 
                          "RMD":client.rmd, 
                          "MKD":client.mkd, 
                          "RNFR":client.rnfr, 
                          "RNTO":client.rnto, 
                          "TYPE":client.type,
                          "DELE":client.dele}

        if command in justOneCommand:
            func = justOneCommand[command]
            func()
        elif command in needTwoCommand:
            sys.stdout.write("Next parameter\n")
            parameter = sys.stdin.readline()[:-1]

            func = needTwoCommand[command]
            func(parameter)
        else:
            sys.stdout.write("No such command!\n")
        
