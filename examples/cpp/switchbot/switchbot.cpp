#include <boost/foreach.hpp>
#include <boost/python.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "../common/utils.hpp"
#include "argparse.hpp"
#include "simpleble/SimpleBLE.h"

using namespace std;
using std::cin;
using std::cout;
using std::endl;
using std::string;

const int _Bot_manufacturer_id = 89;
const std::string _Bot_services_id1 = "cba20d00-224d-11e6-9fb8-0002a5d5c51b";
const std::string _Bot_characteristics_id1 = "cba20003-224d-11e6-9fb8-0002a5d5c51b";
const std::string _Bot_characteristics_id2 = "cba20002-224d-11e6-9fb8-0002a5d5c51b";
std::vector<SimpleBLE::Peripheral> _Peripherals;
std::map<std::string, std::string> _Commands;

/**
 * @brief 文字列の分割
 * 
 * @param str 対象文字列
 * @param del デリミタ
 * @return std::vector<std::string> 分割された文字列ベクター
 */
std::vector<std::string> split(std::string str, char del) {
    std::vector<std::string> result;
    std::string subStr;

    for (const char c : str) {
        if (c == del) {
            result.push_back(subStr);
            subStr.clear();
        } else {
            subStr += c;
        }
    }
    result.push_back(subStr);
    return result;
}

/**
 * @brief スキャン結果から端末を選択
 * 
 * @param devicesList 端末リスト
 * @return std::string 選択した端末のMACアドレス
 */
std::string selectDevice(boost::python::list devicesList) {
    typedef boost::python::stl_input_iterator<string> iterator_type;
    std::vector<std::string> result;
    char del = ',';
    int i;
    i = 1;
    BOOST_FOREACH (const iterator_type::value_type& device,
                   std::make_pair(iterator_type(devicesList),  // begin
                                  iterator_type()))            // end
    {
        result = split(device, del);
        std::size_t size = result.size();
        if (size == 1) {
            cout << i << " " << result[0] << endl;
        } else if (size == 2) {
            cout << i << " " << result[0] << " " << result[1] << endl;
        }
        i = i + 1;
    }

    cout << "Enter the number of the device you want to control:" << endl;
    std::string bt_addr = "";
    int deviceNo = boost::python::len(devicesList);
    int idx;
    cin >> idx;
    if (idx < 1 || idx > deviceNo) {
        cout << "Out of index range" << endl;
        return bt_addr;
    }
    string device_str = boost::python::extract<string>(devicesList[idx - 1]);
    std::vector<std::string> bt_addr_str = split(device_str, del);
    bt_addr = bt_addr_str[0];
    return bt_addr;
}

/**
 * @brief 端末のスキャン
 * 
 * @param scan_timeout_msec スキャンタイムアウト（秒）
 * @return boost::python::list 端末MACアドレスのpython形式のリスト
 */
boost::python::list scanDevices(int scan_timeout_msec) {
    boost::python::list deviceList = {};
    auto adapter_optional = Utils::getAdapter();

    if (!adapter_optional.has_value()) {
        // std::cout << "error at getAdapter" << std::endl;
        return deviceList;
    }

    auto adapter = adapter_optional.value();

    adapter.set_callback_on_scan_found([](SimpleBLE::Peripheral peripheral) {
        // std::cout << "Found device: " << peripheral.identifier() << " [" << peripheral.address() << "] "
        //           << peripheral.rssi() << " dBm" << std::endl;
    });

    adapter.set_callback_on_scan_updated([](SimpleBLE::Peripheral peripheral) {
        // std::cout << "Updated device: " << peripheral.identifier() << " [" << peripheral.address() << "] "
        //           << peripheral.rssi() << " dBm" << std::endl;
    });

    adapter.set_callback_on_scan_start([]() {
        // std::cout << "Scan started." << std::endl;
    });
    adapter.set_callback_on_scan_stop([]() {
        // std::cout << "Scan stopped." << std::endl;
    });
    adapter.scan_for(scan_timeout_msec);

    _Peripherals = adapter.scan_get_results();
    for (size_t i = 0; i < _Peripherals.size(); i++) {
        std::map<uint16_t, SimpleBLE::ByteArray> manufacturer_data = _Peripherals[i].manufacturer_data();
        bool is_switchbot = false;
        for (auto& [_Bot_manufacturer_id, data] : manufacturer_data) {
            if (_Bot_manufacturer_id == 89) {  // manufacturer_id is a search key (89 in decimal)
                is_switchbot = true;
                break;
            }
        }
        if (is_switchbot && _Peripherals[i].is_connectable()) {
            // std::cout << "[" << count << "] " << _Peripherals[i].identifier() << " [" << _Peripherals[i].address() <<
            // "]" << _Peripherals[i].rssi() << " dBm" << std::endl;
            deviceList.append(_Peripherals[i].address());
        }
    }
    return deviceList;
}

/**
 * @brief 16進からバイナリへの変換
 * 
 * @param s 16進文字列
 * @return std::string バイナリ文字列
 */
std::string hex2bin(std::string s) {
    std::string rc;
    int nLen = s.length();
    int tmp;
    for (int i(0); i + 1 < nLen; i += 2) {
        if (std::istringstream(s.substr(i, 2)) >> std::hex >> tmp) {
            rc.push_back(tmp);
        }
    }
    return rc;
}

/**
 * @brief コマンド書き込み
 * 
 * @param bt_addr 端末MACアドレス
 * @param command コマンド
 * @param scan_timeout_msec スキャンタイムアウト（秒）
 */
void writeRequest(std::string bt_addr, std::string command, int scan_timeout_msec) {
    std::vector<SimpleBLE::Peripheral> peripherals;
    SimpleBLE::Peripheral peripheral;

    auto adapter_optional = Utils::getAdapter();
    if (!adapter_optional.has_value()) {
        return;
    }
    auto adapter = adapter_optional.value();
    
    adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {
        // std::cout << "Found device: " << peripheral.identifier() << " [" << peripheral.address() << "]" << std::endl;
        peripherals.push_back(peripheral);
    });
    adapter.set_callback_on_scan_start([]() { 
        // std::cout << "Scan started." << std::endl; 
    });
    adapter.set_callback_on_scan_stop([]() { 
        // std::cout << "Scan stopped." << std::endl; 
    });
    adapter.scan_for(scan_timeout_msec);
   
    for (size_t i = 0; i < peripherals.size(); i++) {
        if (peripherals[i].address() == bt_addr) {
            peripheral = peripherals[i];
            break;
        }
    }

    peripheral.connect();

    // `write_request` is for unacknowledged writes. `write_command` is for acknowledged writes.
    std::string command_bin = hex2bin(_Commands[command]);  // press: "571000"
    peripheral.write_request(_Bot_services_id1, _Bot_characteristics_id2, command_bin);

    if (command == "settings") { // Read battery usage
        SimpleBLE::ByteArray rx_data = peripheral.read(_Bot_services_id1, _Bot_characteristics_id1);
        std::cout << "battery (%) is: " << int(rx_data[1]) << std::endl;
        // Utils::print_byte_array(rx_data);
    }
    peripheral.disconnect();
    // std::cout << "Command executed and terminal disconnected." << std::endl;
}

/**
 * @brief メイン
 * 引数の一覧は以下のコマンドで確認
 *   switchbot --help
 * 
 * @param argc 引数
 * @param argv 引数
 * @return int 戻り値
 */
int main(int argc, char* argv[]) {

    argparse::ArgumentParser parser("switchbot", "switchbot control program", "SwitchBot");
    parser.addArgument({"--scan", "-s"}, "Run Switchbot in scan mode - scan devices to control",
                       argparse::ArgumentType::StoreTrue);
    parser.addArgument({"--scan-timeout"}, "Device scan timeout (default: 2 seconds)");
    parser.addArgument({"--connect-timeout"}, "Device connection timeout (default: 2 seconds)");
    parser.addArgument({"--device", "-d"}, "Specify the address of a device to control");
    parser.addArgument({"--interface", "-i"},
                       "Name of the bluetooth adapter (default: hci0 or whichever is the default)");
    parser.addArgument({"--command", "-c"}, "Specify the command to run (default: press). commands are press,on,off,open,close,pause,settings.");
    parser.addArgument({"--refresh", "-r"}, "Refresh local device list", argparse::ArgumentType::StoreTrue);

    auto args = parser.parseArgs(argc, argv);
    int scan = args.has("scan");
    int scan_timeout = std::stoi(args.safeGet<std::string>("scan-timeout", "2"));
    float connect_timeout = std::stof(args.safeGet<std::string>("connect-timeout", "2"));
    string bt_addr = args.safeGet<std::string>("device", "");
    string bt_interface = args.safeGet<std::string>("interface", "hci0");
    string command = args.safeGet<std::string>("command", "press");
    int refresh = args.has("refresh");

    _Commands["press"] = "570100";
    _Commands["on"] = "570101";
    _Commands["off"] = "570102";
    _Commands["open"] = "570F450105FF00";
    _Commands["close"] = "570F450105FF64";
    _Commands["pause"] = "570F450100FF";
    _Commands["settings"] = "5702";

    // check ARGs
     if (_Commands.find(command) == _Commands.end()) {
        std::cout << "Invalid command. See --help." << std::endl;
        return EXIT_FAILURE;
    }

    namespace python = boost::python;
    // Allow Python to load modules from the current directory.
    setenv("PYTHONPATH", ".", 1);
    Py_Initialize();

    if (scan == 1) {
        // 1.1 scan device to get addr
        try {
            boost::python::list devicesList = scanDevices(int(scan_timeout * 1000));
            bt_addr = selectDevice(devicesList);
        } catch (const python::error_already_set&) {
            PyErr_Print();
            return EXIT_FAILURE;
        }
            
    } else if (bt_addr == "") {
        // 1.2 get device addr from Home otherwise from ARGs
        try {
            python::object my_python_class_module = python::import("switchbot_py3");
            python::object home = my_python_class_module.attr("Home")("switchbot_open_token.txt", "device_list_lint.json");
            if (refresh == 1) {
                // refresh Home information
                home.attr("refresh")();
            }
            python::object py_homeDevicesList = home.attr("get_device_list")();
            boost::python::list homeDevicesList = python::extract<boost::python::list>(py_homeDevicesList);
            bt_addr = selectDevice(homeDevicesList);
        } catch (const python::error_already_set&) {
            PyErr_Print();
            return EXIT_FAILURE;
        }
    }

    // 2 send command to terminal and read from terminal
    if (bt_addr == "") {
        return EXIT_FAILURE;
    }
    writeRequest(bt_addr, command, int(scan_timeout * 1000));

    // Do not call Py_Finalize() with Boost.Python.
    return EXIT_SUCCESS;
}
