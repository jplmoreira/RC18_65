
from _thread import start_new_thread
import socket
import sys



MAX_BUFFER = 512
STANDART_PORTCS = 58065
STANDART_PORTBS = 59000
TASK = 0
server_answer = ""

USERS = ["99999 zzzzzzzz"]


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
	i = 0
	for i in range(len(USERS)):
		if (key[4:]==USERS[i]):
			print(key[4:])
			msg = "AUR OK"
			server_msg = msg.encode()
			return server_msg
		msg = "AUR NOK\n"
		server_msg = msg.encode()
		return server_msg
				
	
#def upload_files_to_dir(files):




def thread(connection):

	server_answer = ""

	user_msg = connection.recv(19)
	request_task = user_msg.decode()
	#request_task = msg_request[TASK]
	print(request_task)

	if (request_task[:3] == "AUT"):
		server_answer = authenticate_usr(request_task)
		connection.send(server_answer)


	if server_answer == "OK":

		while 1:

			user_msg = connection.recv(MAX_BUFFER)
			msg_request = user_msg.decode().split()


			if len(msg_request) != 0:

				request_task = msg_request[TASK]

				if (request_task[:3] == "UPL"):
					server_answer = upload_files_to_dir(request_task)

				else:
					print("ERR")
					break	
	connection.close()
	server_sock.close()
	return 0

			


			
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


#estabelecer a ligacao
server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host_name = socket.gethostname()
server_sock.bind((host_name, BSport))
server_sock.listen(10)

print("Waiting for connection...\n")

while 1:

	connection, client_address = server_sock.accept()
	print("Connection succefully established\n")
	start_new_thread(thread, (connection,))
	




