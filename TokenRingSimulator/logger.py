import socket
import struct
import sys
import _thread

#   224.1.1.2 5007
MCAST_GRP = '224.1.1.2'
MCAST_PORT = 5007
IS_ALL_GROUPS = True


def main():
    if (len(sys.argv) == 2):
        f= open(sys.argv[1], "w+")
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      

        if IS_ALL_GROUPS:
            # on this port, receives ALL multicast groups
            sock.bind(('', MCAST_PORT))
        else:
            # on this port, listen ONLY to MCAST_GRP
            sock.bind((MCAST_GRP, MCAST_PORT))
        mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)

        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

        try:
            while True:
                msg = sock.recv(1024)
                msg_str=msg.decode('ascii')
                f.write(msg_str + '\n')
                print (msg_str)
        except KeyboardInterrupt:
            f.close
            return


if __name__ == "__main__":
    #_thread.start_new_thread(main, ())
    main()
    
    while(1):
        pass

      
  
  

