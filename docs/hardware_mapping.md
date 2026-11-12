> 返回 [文档索引](README.md)

# 硬件资源与引脚映射

实物照片丝印为 **SEEED-EXP430F5529 V1.0**。下表依据 TI **SLAU330B** 用户指南第 22、24、27、28 页及原版 HAL；SEEED V1.0 不等同于 TI Rev2.0/2.1/2.2，不能套用 TI Rev 编号判断其芯片。

| 资源 | MSP430F5529 端口 | 软件配置 |
|---|---|---|
| S1 | P1.7 | 输入，上拉，低电平按下 |
| S2 | P2.2 | 输入，上拉，低电平按下 |
| LED1 | P1.0 | 高电平点亮；JP3 需短接 |
| 加速度计供电 | P3.6 | 输出高电平供电 |
| 加速度计 CS | P3.5 | 低电平选择 |
| 加速度计 MOSI | P3.3 / UCA0SIMO | UCA0 SPI |
| 加速度计 MISO | P3.4 / UCA0SOMI | UCA0 SPI |
| 加速度计 SCLK | P2.7 / UCA0CLK | UCA0 SPI |
| 加速度计 INT1 | P2.5 | 配置为输入，当前不使用中断 |
| 滚轮使能 | P8.0 | GPIO 高电平使能模拟拇指轮 |
| 滚轮滑臂 | P6.5 / ADC12 A5 | ADC12 单通道单次转换 |
| LCD MOSI | P4.1 / UCB1SIMO | UCB1 SPI |
| LCD SCLK | P4.3 / UCB1CLK | UCB1 SPI |
| LCD CS | P7.4 | 低电平选择 |
| LCD RESET | P5.7 | 低电平复位 |
| LCD D/C | P5.6 | 低命令，高数据 |
| LCD 背光使能 | P7.6 | GPIO 高电平使能，不占用 Timer_B |
| SD CS | P3.7 | 保持高电平，避免干扰共享 LCD 总线 |
| 系统时基 | Timer_A0 CCR0 | 约 1 ms 中断 |

加速度计使用 UCA0、LCD 使用 UCB1，两条总线独立。硬件无新增电路：LCD、按键、LED、加速度计、滚轮和调试器均为板载。

## 板卡版本与加速度计

- TI Rev 2.0 / 2.1 参考板：CMA3000-D01。按 TI HAL 通过 UCA0、SPI mode 0 设置 CTRL=0x94（±2g、400Hz、禁用 I²C），主循环每 20ms 读取一次最新轴值。8 位轴值按标称 18mg/LSB 换算。
- TI Rev 2.2 参考板：LIS3DH。WHO_AM_I=0x33；驱动以 SPI mode 3、CTRL_REG1=0x47 设置 50Hz，CTRL_REG4=0x88 设置 BDU、12位高分辨率和 ±2g。使用 1mg/LSB 标称换算。

默认先检测 LIS3DH 身份寄存器，再尝试 CMA3000 控制寄存器回读；自动识别不是板卡版本的最终证明，需要结合丝印和数据变化确认。可在 `include/board_config.h` 中将 `ACCEL_FORCE_TYPE` 改为 1 或 2 强制指定型号，当前保留自动识别。

## 时钟

使用内部 REFO/FLL，不依赖外部晶振起振。名义 SMCLK/MCLK 为 1.048576MHz；Timer_A0 每 1049 个时钟触发一次（约 1.0004ms），计时还受到内部参考源误差影响，不作为精密秒表。

## CMA3000 驱动细节

初始化按 RSTR 写 0x02、0x0A、0x04 执行软件复位，等待 30ms，再读取 REVID/STATUS、配置并回读 CTRL。每次采样检查 CTRL 和 STATUS bit0（校验错误），通过后读取 DOUTX/Y/Z。SPI 约 131kHz，事务前留约 50us 间隔，结束前有界等待 UCBUSY 清零再释放 CS。

VTI 手册在 ±2g/400Hz 下给出 56count/g（约 17.86mg/count），因此保留 18mg/count 近似。I 字段为 CMA3000 的 REVID，不能当成 LIS3DH 的 WHO_AM_I；TYPE=1 也不能证明各轴读数正常。

这些复位与通信改进尚未证明能修复实物 Z 幅度异常，排查进展见 [传感器诊断与 Z 轴排查](sensor_diagnostics.md)。

## 滚轮

板载滚轮是模拟拇指轮（电位器），不是正交编码器：P8.0 高电平使能，滑臂接 P6.5/ADC12 A5（对应 TI 参考设计 `HAL_Wheel.c`）。`source/drivers/wheel.c` 按相对位移每 `WHEEL_STEP_ADC`（默认 96）个计数产生一个调值 tick，因此不需要绝对位置校准；详细用法见 [系统架构与模块接口](architecture.md) 与 [游戏玩法与 TUNE 调参](gameplay.md)。

## BOM

SEEED-EXP430F5529 V1.0 开发板 1 块、匹配板载调试接口的 USB 数据线 1 根、开发电脑 1 台。LCD、按键、LED、加速度计和调试器均为板载，无新增电子元件。

资料出处与核对范围见 [资料来源与兼容性依据](sources.md)。
