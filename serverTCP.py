#!/usr/bin/env python3
# from https://www.geeksforgeeks.org/socket-programming-multi-threading-python/


import socket 
import os	

# import thread module 
from _thread import *
import threading 
  

# thread function 
def threaded(c):
 
    dev = os.open('/dev/ttyOP_hat', os.O_RDONLY)
    try:
        while True:
            data= os.read(dev,900)
            # send back reversed string to client
            if data :
                #print(data) 
                c.send(data) 
    finally:
        print("disconnected client")
        c.close()
        os.close(dev)
  
def Main(): 
    host = "" 
  
    # reverse a port on your computer 
    # in our case it is 12345 but it 
    # can be anything 
    port = 10111 
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
    s.bind((host, port)) 
    print("socket binded to port", port) 
  
    # put the socket into listening mode 
    s.listen(5) 
    print("socket is listening") 
  
    # a forever loop until client wants to exit 
    try:
        while True: 
        # establish connection with client
            c, addr = s.accept() 
            print('Connected to :', addr[0], ':', addr[1]) 
            # Start a new thread and return its identifier 
            start_new_thread(threaded, (c,)) 
    finally:
            print("disconnected server")
            s.close()

  
  
if __name__ == '__main__': 
    Main() 
