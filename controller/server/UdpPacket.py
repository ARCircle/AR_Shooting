import socket
import struct
import time
import numpy as np
import imufusion
from typing import List


HOST = ''
PORT = 10503  # *toko*
BUFF_SIZE = 4096
GYRO_OFFSET = [2.876148, -7.005847, 8.739294] # X,Y,Z
ACC_OFFSET = [0.003080, 0.021831, 0.091680] # X,Y,Z


class UdpPacket:
    def __init__(self, sock: socket.socket):
        # recieve packet
        # wait untill packet comes
        self._data, self._fromaddr = sock.recvfrom(BUFF_SIZE)
        # packet recieved time
        self._time = time.time()
        # fire detect bool
        self._fire = b'\x00'
        # gyroXYZ
        self._gyro = np.zeros(3)
        # accelXYZ
        self._acc = np.zeros(3)
        self.parse()
        
    def parse(self):
        # 書式定義文字: https://docs.python.org/ja/3/library/struct.html#format-characters
        # fire, gyroX, gyroY, gyroZ, accX, accY, accZ
        # parse
        msg = list(struct.unpack('=cffffff', self._data))
        # set values
        self._fire = msg[0]
        if self._fire == b'\x00':
            self._fire = None
        if self._fire == b'f':
            self._fire = 'f'
        if self._fire == b'c':
            self._fire = 'c'
        for i in range(3):
            self._gyro[i] = msg[i+1]
            self._acc[i] = msg[i+4]
        # print(msg)
        
    def offset(self, GYRO: List[float], ACC: List[float]):
        # offset
        for i in range(3):
            self._gyro[i] -= GYRO[i]
            self._acc[i] -= ACC[i]
    
    def fire_valid_update(self, last_fire_time: int, fire_inteval_ms: int):
        # check fire has valid interval and update the bool
        # if self has valid interval, return self._time(latest fire time)
        if self._fire == None:
            pass
        elif self._time - last_fire_time >= fire_inteval_ms / 1000:
            return self._time
        else:
            self._fire = None
        
        return last_fire_time
            
        
        