# -*- coding: utf-8 -*-
import argparse
import struct
import sys
import time
import os
import subprocess
import json
from contextlib import contextmanager
import bluetooth
from bluetooth.ble import DiscoveryService, GATTRequester
import gattlib
# from get_device_list import get_device_list


@contextmanager
def connect(device: str, bt_interface: str, timeout: float):
    """Terminalに接続

    Args:
        device (str):  Terminal MAC address
        bt_interface (str): Bluetooth インターフェース (eg. hci0)
        timeout (float): 接続タイムアウト(秒)

    Raises:
        ConnectionError: 接続エラー例外

    Yields:
        object: 接続ハンドル
    """

    if bt_interface:
        req = GATTRequester(device, False, bt_interface)
    else:
        req = GATTRequester(device, False)

    req.connect(False, 'random')
    connect_start_time = time.time()

    while not req.is_connected():
        if time.time() - connect_start_time >= timeout:
            raise ConnectionError('Connection to {} timed out after {} seconds'.
                                  format(device, timeout))
        time.sleep(0.1)

    yield req

    if req.is_connected():
        req.disconnect()


class Scanner(object):
    """スキャナークラス

    Args:
        object (Scanner): Scannerインスタンス

    Returns:
        Scanner: Scannerインスタンス
    """

    service_uuid = 'cba20002-224d-11e6-9fb8-0002a5d5c51b'
    _default_scan_timeout = 8
    _default_connect_timeout = 5.0

    def __init__(self, bt_interface: str = None, scan_timeout: int = None,
                 connect_timeout: float = None):
        """初期化

        Args:
            bt_interface (str, optional): Bluetoothインターフェース. Defaults to None.
            scan_timeout (int, optional): スキャンタイムアウト(秒). Defaults to None.
            connect_timeout (float, optional): 接続タイムアウト(秒). Defaults to None.
        """

        self.bt_interface = bt_interface
        self.connect_timeout = connect_timeout or self._default_connect_timeout
        self.scan_timeout = scan_timeout or self._default_scan_timeout

    @classmethod
    def is_switchbot(cls, device: str, bt_interface: str, timeout: float) -> bool:
        """Terminalがスイッチボットのインスタンスか

        Args:
            device (str): Terminal MAC address
            bt_interface (str): Bluetoothインターフェース
            timeout (float): タイムアウト(秒)

        Returns:
            bool: スイッチボットならTrue
        """

        try:
            with connect(device, bt_interface, timeout) as req:
                for chrc in req.discover_characteristics():
                    if chrc.get('uuid') == cls.service_uuid:
                        print(' * Found Switchbot service on device {} handle {}'.
                              format(device, chrc.get('value_handle')))
                        return True
        except ConnectionError:
            return False

    def scan(self):
        """Terminalをスキャン

        Returns:
            [str]: MACアドレスのリスト
        """

        if self.bt_interface:
            service = DiscoveryService(self.bt_interface)
        else:
            service = DiscoveryService()

        print('Scanning for bluetooth low-energy devices')
        devices = list(service.discover(self.scan_timeout).keys())
        print('Discovering Switchbot services')
        return [dev for dev in devices
                if self.is_switchbot(dev, self.bt_interface, self.connect_timeout)]


class Home(object):
    """ホームクラス
    SwitchBotアプリでホームに登録した端末情報

    Args:
        object (Home): Homeインスタンス
    """

    def __init__(self, token_file: str = None, device_list_file: str =None):
        """初期化

        Args:
            token_file (str, optional): switchbot開発者トークンファイル名. Defaults to None.
            device_list_file (str, optional): スキャン端末情報jsonファイル名. Defaults to None.
        """

        self.token_file = token_file
        self.device_list_file = device_list_file
        is_file = os.path.isfile(self.device_list_file)
        if not is_file:
            subprocess.run(["./get_device_list.sh", self.device_list_file])

    def refresh(self):
        """Switchbot APIによりローカル端末情報を更新
        """

        subprocess.run(["./get_device_list.sh", self.device_list_file])

    def get_device_list(self):
        """ローカル端末情報から得たMACアドレスのリスト取得

        Returns:
            [str]: 端末MACアドレスのリスト
        """

        json_open = open(self.device_list_file, 'r')
        json_load = json.load(json_open)
        self.deviceList = json_load['body']['deviceList']
        deviceIds = []
        for d in self.deviceList:
            device_Id_Name = d['deviceId'][0:2] +':'+ d['deviceId'][2:4] +':'+ d['deviceId'][4:6] \
            +':'+ d['deviceId'][6:8] +':'+ d['deviceId'][8:10] +':'+ d['deviceId'][10:12] + ',' + d['deviceName']
            deviceIds.append(device_Id_Name)
        return deviceIds


class Driver(object):
    """Bluetoothにアクセスするためのドライバークラス

    Args:
        object (Driver): Driverインスタンス

    Returns:
        Driver: Driverインスタンス
    """

    handles = {
        'press': 0x16,
        'on': 0x16,
        'off': 0x16,
        'open': 0x0D,
        'close': 0x0D,
        'pause': 0x0D,
        'settings': 0x16,
    }
    notif_handle = 0x13
    commands = {
        'press': b'\x57\x01\x00',
        'on': b'\x57\x01\x01',
        'off': b'\x57\x01\x02',
        'open': b'\x57\x0F\x45\x01\x05\xFF\x00',
        'close': b'\x57\x0F\x45\x01\x05\xFF\x64',
        'pause': b'\x57\x0F\x45\x01\x00\xFF',
        'settings': b'\x57\x02',
    }
    _default_bt_interface = 'hci0'
    _default_timeout_secs = 5

    def __init__(self, device: str, bt_interface : str =None, timeout_secs : int =None):
        """初期化

        Args:
            device (str): Terminal MACアドレス
            bt_interface (str, optional): Bluetoothインターフェース. Defaults to None.
            timeout_secs (int, optional): タイムアウト(秒). Defaults to None.
        """

        self.device = device
        self.bt_interface = bt_interface if bt_interface else self._default_bt_interface
        self.timeout_secs = timeout_secs if timeout_secs else self._default_timeout_secs
        self.req = None

    def run_command(self, command : str):
        """_summary_

        Args:
            command (str): コマンド(レスポンスなし)

        Returns:
            object: 接続ハンドル
        """

        with connect(self.device, self.bt_interface, self.timeout_secs) as req:
            print('Connected!')
            return req.write_by_handle(self.handles[command], self.commands[command])

    def run_and_res_command(self, command: str ):
        """_summary_

        Args:
            command (str): コマンド(レスポンスあり)

        Returns:
            object: 接続ハンドル
        """

        with connect(self.device, self.bt_interface, self.timeout_secs) as req:
            print('Connected!')
            req.write_by_handle(self.handles[command], self.commands[command])
            return req.read_by_handle(self.notif_handle)

    def get_settings_value(self, value) -> int:
        """レスポンスからバッテリー残量を抽出

        Args:
            value (bytes): レスポンス

        Returns:
            int: バッテリー残量(%)
        """

        value = struct.unpack('B' * len(value[0]), value[0])
        # settings = {}
        # settings["battery"] = value[1]
        # settings["firmware"] = value[2] / 10.0
        # settings["n_timers"] = value[8]
        # settings["dual_state_mode"] = bool(value[9] & 16)
        # settings["inverse_direction"] = bool(value[9] & 1)
        # settings["hold_seconds"] = value[10]
        return value[1]

def main():
    """メイン関数
    """

    parser = argparse.ArgumentParser()
    parser.add_argument('--scan', '-s', dest='scan', required=False, default=False, action='store_true',
                        help="Run Switchbot in scan mode - scan devices to control")

    parser.add_argument('--scan-timeout', dest='scan_timeout', type=int,
                        required=False, default=8,
                        help="Device scan timeout (default: 8 seconds)")

    parser.add_argument('--connect-timeout', dest='connect_timeout', type=float, required=False, default=5,
                        help="Device connection timeout (default: 5 seconds)")

    parser.add_argument('--device', '-d', dest='device', required=False, default=None,
                        help="Specify the address of a device to control")

    parser.add_argument('--interface', '-i', dest='interface', required=False, default='hci0',
                        help="Name of the bluetooth adapter (default: hci0 or whichever is the default)")

    parser.add_argument('--command', '-c', dest='command', required=False, default='press',
                        help="Command to be sent to device. \
                            Noted that press/on/off for Bot and open/close for Curtain. \
                            Required if the controlled device is Curtain (default: %(default)s)")

    parser.add_argument('--refresh', '-r', dest='refresh', required=False, default=False, action='store_true',
                        help="Refresh local device list")


    opts, args = parser.parse_known_args(sys.argv[1:])
    
    if opts.scan:
        scanner = Scanner(opts.interface, opts.scan_timeout, opts.connect_timeout)
        devices = scanner.scan()
        
        if not devices:
            print('No Switchbots found')
            sys.exit(1)

        print('Found {} devices:'.format(len(devices)))
        print('Enter the number of the device you want to control:')

        for i in range(0, len(devices)):
            print('\t{}\t{}'.format(i, devices[i]))

        i = int(input())
        if i < 0 or i >= len(devices):
            print('Out of index range')
            sys.exit(1)
        bt_addr = devices[i]

    elif opts.device:
        bt_addr = opts.device
        
    else:
        home = Home("switchbot_open_token.txt", "device_list_lint.json")
        if opts.refresh:
            home.refresh()
        
        deviceIds = home.get_device_list()
        if (not deviceIds) or (len(deviceIds) == 0):
            print('No Switchbots found')
            sys.exit(1)
        
        print('Found {} devices:'.format(len(deviceIds)))
        print('Enter the number of the device you want to control:')
        for i, d in enumerate(deviceIds):
            dl = d.split(',')
            print('\t{}\t{}\t{}'.format(i, dl[0], dl[1]))

        i = int(input())
        if i < 0 or i >= len(deviceIds):
            print('Out of index range')
            sys.exit(1)
        
        bt_addr = deviceIds[i].split(',')[0]

    driver = Driver(device=bt_addr, bt_interface=opts.interface, timeout_secs=opts.connect_timeout)
    
    if opts.command in ['settings']:
        value = driver.run_and_res_command(opts.command)
        settings = driver.get_settings_value(value)
        # https://github.com/OpenWonderLabs/SwitchBotAPI-BLE/blob/latest/devicetypes/bot.md
        print('battery(%):' + str(settings))
    elif opts.command in driver.commands:
        driver.run_command(opts.command)
        print('Command execution successful')   
    else:
        print('invalid commands.')

if __name__ == '__main__':
    main()


# vim:sw=4:ts=4:et:
