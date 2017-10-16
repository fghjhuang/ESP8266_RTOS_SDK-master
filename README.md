# 基于ESP8266_RTOS_SDK的项目 #
主要功能
## 初始化检查有无保存ap的参数，没有就进行smartconfig进程
## smartconfig之后连接ap，进行udp广播获取设备ip
## 获取设备ip后进行websocket连接，连接成功发送数据，获取json数据并进行解析