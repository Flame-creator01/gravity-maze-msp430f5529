> 返回 [文档索引](README.md)

# 验证记录（主机测试）

本文记录主机侧已经执行的测试与结果。结论范围仅限主机：**没有**执行 CCS 目标编译、下载或实物验证。

## 对应当前代码的修订

当前代码包含以下变更，主机回归在每次变更后重跑并通过：

- 玩法：6 关、能量点开门、陷阱罚时、倾角调速与信息 Flash 存档；校准判据未改。
- 构建：`save_port.c` 曾因 F5529 无 `FCTL2` 编译失败，已按 SLAU208 的 INFO 解锁序列修正——清 `FCTL4.LOCKINFO`、清 `FCTL3.LOCK`，并临时翻转复位值为 1 的 `FCTL3.LOCKA`（LOCKA=1 会禁止所有信息段擦除），操作后恢复保护、操作前查 BUSY；主机寄存器桩同步为真实位值并移除 FCTL2，回归结果不变。
- 响应：触发阈值 220→130mg、最快倾角 800→450mg、每格 260/120→190/90ms，测试按新参数更新并通过。
- TUNE 灵敏度调试台（参数存 INFO B）；换向/后摇修订：低通权重 1/4→2/4（TUNE 可调 1～3）、释放阈值 80→100mg、轴余量 45→35mg。
- TUNE 调值改为**转动板载模拟拇指轮**（P8.0 使能、P6.5/ADC12-A5，`wheel.c`），S1/S2 只负责选参数，主机测试用滚轮桩覆盖调值流程。

## 执行方式

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\test_host.ps1 -BuildDirectory .\work\host_build
```

测试环境：Windows 主机 GCC，`-std=c89 -Wall -Wextra -Werror -pedantic`。测试直接编译生产代码，只有硬件访问替换为测试桩。

## 实际输出

```text
level 1: 1 items, 1 traps, 26 steps solved
level 2: 2 items, 1 traps, 26 steps solved
level 3: 2 items, 2 traps, 30 steps solved
level 4: 3 items, 2 traps, 44 steps solved
level 5: 3 items, 3 traps, 54 steps solved
level 6: 4 items, 3 traps, 66 steps solved
timing: pause/resume, restart, tilt speed, rate limit and tick rollover passed
motion: negative offsets, tilt magnitude, calibration and directions passed
calibration: settling ignores samples, bounded noise, motion/gravity/tilt reasons and full-span arithmetic passed
keys: bounce rejection, one short event, one long event, no duplicate passed
LCD: ASCII, bottom-right pixel, clipping and full frame passed
application: calibration/menu/play/pause, sensor failure and stale-data handling passed
application calibration: one-second settling, repeated short press ignored, cancellation and tick rollover passed
application calibration: gravity failure is reported, retry clears failure and reaches menu passed
application diagnostics: raw/register snapshot, S1 reinitialization, failure and recovery passed
gameplay: trap teleport + time penalty and locked exit notice passed
save: defaults, double-buffer store/load, checksum fallback, apply-result and erase failure passed
menu: locked level refused, saved unlock allows selection passed
tuning: defaults, interval, clamp, filter weight, INFO B save/load and corrupt fallback passed
tuning app: diagnostics->TUNE, wheel adjust, S1/S2 select, hold-S2 save and reload passed
screens: gameplay/menu/clear/paused/calibration/diagnostics/tuning/error frames rendered
ALL HOST TESTS PASSED (no physical hardware is exercised)
key driver: 50ms taps, delayed consumption, both keys, queue wrap/full, bounce, long press and tick rollover passed
ALL KEY DRIVER HOST TESTS PASSED (timer cadence and GPIO are simulated)
accelerometer: reset sequence, register selection, signed XYZ, raw diagnostics, config/parity rejection, bounded RX/TX/busy timeouts and LIS3DH regression passed
ALL ACCELEROMETER HOST TESTS PASSED (SPI/device responses are simulated)
info-flash port: compiled against host register stubs (syntax/C89 only)
thumb wheel: compiled against host ADC stubs (syntax/C89 only)
```

## 额外覆盖的内容

- 六关地图：行宽 15、四周封闭、起点/出口合法、每关至少 1 个能量点与 1 个陷阱；状态 BFS（位置 × 已收集掩码）验证“收集全部能量点后到出口”且路线不经过陷阱；随后用真实 `game.c` 沿该路线逐格运行，校验每一步位置、步数、收集计数、出口开启与通关冻结。
- 倾角调速：轻倾 190ms、重倾 90ms、中等倾角约 137ms；停止/重新开始不能绕过限速；时基回绕。
- 陷阱：踩中后退回起点、`trap_hits` 增加、罚时计入用时、提示为 `TRAP`。
- 出口锁定：能量点未收集完时进入出口被拒绝、位置不变、提示为 `LOCKED`。
- 存档：默认值、双槽存储/读取、校验和损坏时回退旧记录、双槽损坏回退默认、`Save_ApplyResult` 的最佳用时/步数/解锁合并、擦除失败不崩溃。
- 菜单锁定：未解锁关卡按 S1 不进入游戏；保存的解锁进度载入后可开始。
- TUNE 调试台：7 个参数默认值与范围、`Tuning_Interval` 在触发/满速处的端点与中点、参数钳制（释放<触发、快速<慢速、满速>触发、滤波 1～3）、INFO B 保存/读取、记录损坏回退默认；应用级“诊断页长按 S2 进入 → 滚轮调值 → S1/S2 切换 → 长按 S2 保存回菜单 → 重新载入”的完整流程。
- 滚轮驱动：`wheel.c` 通过主机 ADC/端口桩的 C89 语法编译（ADC12 读取与 P8.0 使能未做实机验证）。
- 信息 Flash 驱动：`save_port.c` 通过主机寄存器桩的 C89 语法编译（不等价于实物擦写验证）。
- 界面冒烟：6 关的游戏中/菜单/通关/暂停，以及校准、诊断、TUNE、错误页均完整渲染一帧，无越界或断言。
- 校准：静置期丢弃样本、按键后暂时晃动但静置结束后稳定可通过、等待期重复短按不重置、长按取消、静置计时回绕、三类失败原因、全量程峰峰差计算、失败后重试清除错误并进入菜单。校准阈值未改。
- 按键驱动：直接编译 `key.c`，通过测试专用 `msp430.h` 替代 GPIO 和 GIE，模拟 5ms 中断调用；覆盖延迟取事件期间的 50ms 短按、多键事件、队列回绕/满及溢出后恢复。不执行真实 Timer_A0 ISR，不代表 GPIO 实测。
- 传感器：直接编译生产 SPI 驱动，通过寄存器模型模拟 CMA3000/LIS3DH 和超时；应用诊断重初始化成功、失败、恢复也已覆盖。模型未执行物理器件，不可据此判断 Z 轴硬件正常。

## 尚未验证

传感器侧结论保持：TYPE=1、侧立 X≈−1008mg、正反面 Z≈±162mg；校准判据未改，Z 问题需另行处理。以下项目均未验证：

- CCS 5.5.0 中的 TI MSP430 目标编译和链接（新增 `save.c/save_port.c/tuning.c/wheel.c` 及 INFO B/C/D 访问）；
- eZ-FET 下载；
- INFO Flash 实际写入与断电保持（通关一关后断电重上电看 `PROGRESS/BEST`，以及 TUNE 参数是否保留）；
- TUNE 调试台的实际显示、滚轮转动方向与调值手感、保存结果；
- 滚轮 ADC（P6.5/A5、P8.0 使能）的实际读数与旋转方向；
- 6 关实际手感、倾角调速与陷阱罚时的体感；
- DOGS102 LCD 的实际显示方向、对比度和新增图形（能量点/陷阱/出口锁定/倾角条）；
- 加速度轴方向、采样频率与 30 分钟连续运行。

上板测试的方法与顺序见 [测试计划](test_plan.md)，问题记录见 [调试与问题排查记录](debug_log.md)。
