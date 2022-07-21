#!/bin/bash
# Get BLE devices information

if [ $# -ne 1 ]; then
  echo "指定された引数は$#個です。" 1>&2
  echo "実行するには1個の引数(デバイスリストファイル名)が必要です。" 1>&2
  exit 1
fi

# Please follow these steps to get token.
# 1. Download the SwitchBot app on App Store or Google Play Store
# 2. Register a SwitchBot account and log in into your account
# 3. Generate an Open Token within the app 
#  a) Go to Profile > Preference
#  b) Tap App Version 10 times. Developer Options will show up
#  c) Tap Developer Options
#  d) Tap Get Token and save to 'switchbot_open_token.txt'
DATA=`cat switchbot_open_token.txt`

# Get device list in json format
curl -X GET "https://api.switch-bot.com/v1.0/devices"  -H "Authorization:$DATA" > device_list.json
cat device_list.json | python3 -m json.tool > $1