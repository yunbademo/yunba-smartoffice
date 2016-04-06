这里介绍 [Zigbee][1] 智能开关制作过程。

简介
--------

我们将一个 [Arduino Pro Mini][2] 和一个基于 Zigbee 的串口模块，集成到一个普通遥控开关内部，制作成一个智能开关；将另一个基于 Zigbee 的串口模块作为网关，与智能开关通讯，并通过 [云巴实时消息服务][3] 接入互联网；从而达到可以使用 Web 或 App 实时控制智能开关的目的。

下图为制作完成的智能开关：

![3.jpg](../images/3.jpg)

这是拆开后的内部图：

![4.jpg](../images/4.jpg)

需要的元件
--------

1. 普通遥控开关 1 个，有 3V 直流输出，可以给 Arduino Pro Mini 及 Zigbee 模块供电，[购买链接][4]。
2. Arduino Pro Mini 1 个，作为控制器，[购买链接][5]。
3. Zigbee 模块两个，一个集成在开关内部，另一个接入互联网，[购买链接][6]。
4. Arduino Pro Mini 专用 miniUSB 接口，用于烧写程序，[购买链接][7]。
5. USB 转串口线，用于连接 Zigbee 模块和 PC 机作为网关，[购买链接][13]。
6. 各种导线若干，见智能开关内部图。

代码
--------

将智能开关的 [开源代码][8] 使用 [Arduino IDE][9] 编译并烧录到 Arduino Pro Mini 上。还需要将另一个 Zigbee 模块通过 USB 转串线接在安装了 Python 的 PC 机上，并运行 [串口网关代码][10]。

硬件连接
--------

用开关的直流输出分别给 Arduino Pro Mini 和 Zigbee 模块供电，Arduino Pro Mini 输出控制信号到开关，总的连线如下：

| Arduino Pro Mini | Zigbee 模块 | 开关 |
|--------|--------|--------|
| VCC | VCC | VCC |
| GND | GND | GND |

安装及测试
------

安装之前需要先配置 Zigbee 模块，我们配置成：波特率 115200，广播模式。使两个模块能正常通讯，参考模块厂家的 [配置视频][11]。

配置好后，将各元件连接，并安装好。然后将开关接入交流电路中。并将另一个 Zigbee 模块通过 USB 转串线接在安装了 Python 的 PC 机上，并运行串口网关代码。正常情况下就可以通过 [Web][12] 端查看智能开关的状态并对其进行控制了。需要注意的是要保证开关代码和串口网关代码中的 `Appkey`，`Devid`，`Topic` 等参数与 Web 中的一致。Web 端如下图：

![5.png](../images/5.png)

[1]: http://baike.baidu.com/link?url=Kcwx8ighfWCVc23x2V7q3uK0NhGk4vNAUnnUN4zYJFWbWpq68GvjoJHRJlOZsVZILpR_RJcBoes6-WNrCVW0Mq
[2]: https://www.arduino.cc/en/Main/ArduinoBoardProMini
[3]: http://yunba.io
[4]: http://item.jd.com/1462873148.html
[5]: https://item.taobao.com/item.htm?id=521709260567
[6]: https://item.taobao.com/item.htm?id=520850867141
[7]: https://item.taobao.com/item.htm?id=521709808584
[8]: https://github.com/shdxiang/yunba-smartoffice/blob/master/arduino/sketch_switch/sketch_switch.ino
[9]: https://www.arduino.cc/en/Main/Software
[10]: https://github.com/shdxiang/yunba-smartoffice/blob/master/python/serial_gateway.py
[11]: http://www.tudou.com/programs/view/gJOcbx7MX4w/
[12]: #
[13]: https://item.taobao.com/item.htm?id=45811340839
