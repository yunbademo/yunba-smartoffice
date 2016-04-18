状态上报
--------

收到请求上报或状态变化时上报，`devid` 为设备 ID, `status` 依次为：

| 状态 | 值 |
|--------|--------|
| 开关 | *1*: 开 *2*: 关 |
| 模式 | *1*: 制冷 *2*: 制热 *3*: 抽湿 *4*: 送风 |
| 风速 | *1*: 高风 *2*: 中风 *3*: 低风 *4*: 自动 |
| 设置温度 | 设置的目标温度 |
| 当前温度 | 当前实时的温度 |

```json
{"devid":"tc_office_1","status":[1,1,1,24,0]}
```

请求上报
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"get"}
```

开关切换
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"on_off"}
```

模式切换
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"mode"}
```

风速切换
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"fan"}
```

增加温度
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"inc"}
```

降低温度
--------

`devid` 为设备 ID：
```json
{"devid":"tc_office_1","cmd":"dec"}
```