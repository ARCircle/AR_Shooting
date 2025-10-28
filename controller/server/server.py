import socket
import struct
from typing import List
import time
import keyboard
import math
import mouse
import numpy as np
import imufusion
# udp packet class
from UdpPacket import UdpPacket


HOST = ''
PORT = 10503  # *toko*
BUFF_SIZE = 4096
GYRO_OFFSET = [0, 0, 0] # X,Y,Z
ACC_OFFSET = [0, 0, 0] # X,Y,Z

VERTICAL_AXIS = 2
CENTER_YAW = 0
OFFSET_RANGE = 10000
MONITOR_DISTANCE = 2500 # mm
MONITOR_SIZE = [1920, 1080] # X,Y mm
MONITOR_PPMM = 10 # 82A100GLJP:(157ppi, 6ppmm), 32MP58HQ:(apx4ppmm)
Z_COEF = 190
X_COEF = 220
MEDGWICK_SAMPLE_RATE = 500 # ms

GUN_FIRE_RATE = 250 # ms


def sigmoid_abs(coef:int, shift: int, input: float):
    gain = 1 / (1 + math.exp(-coef * abs(input) + shift))
    return gain
    
def calc_mouseXY(yaw: float, pitch: float):
    aim_z = MONITOR_DISTANCE * math.tan(math.radians(pitch))
    aim_x = MONITOR_DISTANCE * math.tan(math.radians(yaw))
    aim_z_dot = aim_z * MONITOR_PPMM
    aim_x_dot = aim_x * MONITOR_PPMM
    aim_z_dot = int(Z_COEF * math.sin(pitch))
    aim_x_dot = int(X_COEF * math.sin(yaw))
    # aim_z_dot += MONITOR_SIZE[1] / 2
    # aim_x_dot += MONITOR_SIZE[0] / 2
    return (aim_x_dot, aim_z_dot)

def get_offset_gyro(sock:socket.socket) -> List[float]:
    print(f"START GYRO OFFSET.\nPRESS e TO FINISH, PRESS r TO RESET.")
    count = 1
    sum = np.zeros(3)
    mean = np.zeros(3)
    while True:
        # 'e'押下でオフセット終了
        if keyboard.is_pressed('e'):
            break
        if keyboard.is_pressed('r'):
            count = 1
            sum = np.zeros(3)
            mean = np.zeros(3)
        packet = UdpPacket(sock)
        sum += packet._gyro
        mean = sum / count
        print(*[f"{abs(i):.3f}" for i in (packet._gyro - mean)],sep=', ', end='          \r')
        count += 1
    print(f"OFFSET_GYRO: {mean}")
    return mean
        
if __name__ == "__main__":
    # Madgwickフィルタによる姿勢角計算
    # https://github.com/xioTechnologies/Fusion/tree/main
    ahrs = imufusion.Ahrs()
    # パラメータ設定
    # https://github.com/xioTechnologies/Fusion#alegorithm-settings
    ahrs.settings = imufusion.Settings(imufusion.CONVENTION_NED,  # convention
                                   0.15,  # gain
                                   2000,  # gyroscope range
                                   10,  # acceleration rejection
                                   10,  # magnetic rejection
                                   MEDGWICK_SAMPLE_RATE * 10)  # every 5 sec
    # offset調整後の微ズレ補正のためのimufusion.Offset()
    active_offset = imufusion.Offset(MEDGWICK_SAMPLE_RATE)
    
    # UDP通信用ソケット
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # bindする．
    # https://docs.python.org/ja/3/library/socket.html#socket.socket.listen
    sock.bind((HOST, PORT))
    # sock.recvfrom(BUFF_SIZE)でデータ長BUFF_SIZEのUDPパケットを受信, @UdpPacket.py

    # ofxfsetを取得
    GYRO_OFFSET = get_offset_gyro(sock)
    input("Press any key to start program.\n")
    
    # 角度変位算出のため，1つ前の状態を記憶しておく
    prev_angles = np.zeros(3)
    
    # 発射レート制御のため，最後に発射された時刻を記録しておく
    last_fire_time = time.time()
    
    while True:
        # 'x'押下で処理終了
        if keyboard.is_pressed('x'):
            break
        
        # パケットクラスを生成，データ受信，オフセットの反映
        packet = UdpPacket(sock)
        packet.offset(GYRO_OFFSET, ACC_OFFSET)
        # さらにオフセット微調整
        packet._gyro = active_offset.update(packet._gyro)
        # 発射レートの有効性を確認
        last_fire_time = packet.fire_valid_update(last_fire_time, GUN_FIRE_RATE)
        
        # MadgwickのQuaternionを更新
        ahrs.update_no_magnetometer(packet._gyro, packet._acc, 1 / MEDGWICK_SAMPLE_RATE)
        # Quaternionから姿勢角を求める
        new_angles = ahrs.quaternion.to_euler()
        
        # 発射コードがfならクリック
        if packet._fire == 'f':
            mouse.click("left")
        # コードcはセンター出し
        if packet._fire == 'c':
            # サブモニターにゲームを表示するので，mouse_x = -center
            mouse.move(-MONITOR_SIZE[0]/2, MONITOR_SIZE[1]/2, absolute=True)
            mouse.click("left")
            
            
        # 変化分だけマウスを移動させる
        delta_angles = new_angles - prev_angles
        mouse_dx, mouse_dy = calc_mouseXY(delta_angles[2], -delta_angles[0])
        mouse.move(mouse_dx, mouse_dy, absolute=False)
        

        
        # 世代交代
        prev_angles = new_angles
        
        # デバッグ出力
        # print(f"{packet._fire}\t{mouse_dx:.3f}\t{mouse_dy:.3f}")
        if(packet._fire != None):
            print(f"{packet._fire}")
        # print(*packet._gyro, sep="\t")
        

    # 通信処理が終了したらソケットを閉じる
    sock.close()
            
            
        
