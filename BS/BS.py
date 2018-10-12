
from _thread import start_new_thread
import socket
import sys
import signal



MAX_BUFFER = 512
STANDART_PORTCS = 58065
STANDART_PORTBS = 59000

server_answer = ""

USERS = []


CSport = 0
BSport = 0
CSname = 0

def get_argument_type(arg):
	if (len(arg) != 2 or arg[0] != "-") :
		print("Argument flags should be 2 characters, the first being '-' and the second a letter")
	else:
		if arg[1]=="b":
			return 1
		elif arg[1]=="n":
			return 2
		elif arg[1]=="p":
			return 3
		else:
			return 0


def authenticate_usr(key):
	msg = ""
	for i in range(len(USERS)):
		if (key[4:]==USERS[i]):
			print("OK")
			msg = "AUR OK\n"
			server_msg = msg.encode()
			return server_msg
		print("NOK")
		msg = "AUR NOK\n"
		server_msg = msg.encode()
		return server_msg
				
	
def upload_files_from_dir(files):
	return "UPR NOK"

def restore_files_from_dir(dir):
	return "RBR EOF"

def read_msg(connection):
	msg = ""
	while 1:
		byte = connection.recv(1)
		byte = byte.decode()
		if byte != "\n":
			msg += byte
		elif byte == "\n":
			break
	return msg

def create_msg(key, host_ID, port):
	msg = ""
	msg = key + " " + str(host_ID) + " " + str(BSport) + "\n"
	message = msg.encode()
	return message

def thread(connection):

	server_answer = ""
	msg_request = read_msg(connection)
	

	print(msg_request)

	if (msg_request[:3] == "AUT"):
		server_answer = authenticate_usr(msg_request)
		connection.send(server_answer)


	if server_answer == "OK":

		while 1:

			msg_request = read_msg(connection)


			if len(msg_request) != 0:

				if (msg_request[:3] == "UPL"):
					server_answer = upload_files_from_dir(msg_request)
					connection.send(server_answer)
					connection.close()
					server_sock.close()
				if (msg_request[:3] == "RSB"):
					server_answer = restore_files_from_dir(msg_request)
					connection.send(server_answer)
					connection.close()
					server_sock.close()

				else:
					print("ERR")
					break	

	connection.close()
	server_sock.close()
	return 0


def thread_cs(connection):

	server_answer = ""
	msg_request = read_msg(connection)

	if (msg_request != "RGR NOK" or msg_request != "RGR OK"):
		msg = "RGR ERR\n"
		server_answer = msg.encode()
		connection.send(server_answer)
	connection.close()
	server_sock.close()
	


			
#############################################################
#----------------------------MAIN---------------------------#
#############################################################

#teste da validade dos argumentos passados e respetiva atribuicao de nomes
if len(sys.argv)==1:
	print("No arguments passed")
elif len(sys.argv)>7:
	print("Too many arguments passed")
else:
	if (len(sys.argv)%2==0):
		print("Wrong argument numbers: you should disclose the BS server port by using the flag -b and the CS server name and port by using the flags -n and -p respectively")
	else:
		for i in range(1, len(sys.argv), 2):

			numb = get_argument_type(sys.argv[i])
			if numb == 1:
				BSport = sys.argv[i+1]
			elif numb == 2:
				CSname = sys.argv[i+1]
			elif numb == 3:
				CSport = sys.argv[i+1]
			else:
				print("Flag not recognized")

if CSport == 0:
	CSport = STANDART_PORTCS
if BSport ==0:
	BSport = STANDART_PORTBS
if CSname ==0:
	CSname = socket.gethostname()


def unregisterBS(connection):
	msg = ""
	msg = "UNR " + str(host_ID) +  " " + str(BSport) + "\n"
	message = msg.encode()
	server_sock.sendto(message, (CSname, CSport))
	CS_answer = read_msg(connection)
	if (CS_answer != "UAR OK" or CS_answer != "UAR NOK"):
		msg = "UAR ERR\n"
		server_answer = msg.encode()
		connection.send(server_answer)
	connection.close()
	server_sock.close()
		


#estabelecer a ligacao com o CS

server_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
host_ID = socket.gethostbyname(socket.gethostname())
message = create_msg("REG", host_ID, BSport)
server_sock.sendto(message, (CSname, CSport))
print("Waiting for connection with CS...\n")

connection, cs_address = server_sock.recvfrom(MAX_BUFFER)
print("Connection succefully established\n")
start_new_thread(thread_cs, (connection,))

signal.signal(signal.SIGINT, unregisterBS(connection))






#estabelecer a ligacao com USER

server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host_name = socket.gethostname()
server_sock.bind((host_name, BSport))
server_sock.listen(10)

print("Waiting for connection with USER...\n")

while 1:

	connection, client_address = server_sock.accept()
	print("Connection succefully established\n")
	start_new_thread(thread, (connection,))
	




