> 返回 [文档索引](README.md)

# 资料来源与兼容性依据

核对日期：2026-11-05。

1. [TI MSP-EXP430F5529 用户指南 SLAU330B](https://www.ti.com/lit/ug/slau330b/slau330b.pdf)：第19页按键表，第20页加速度计说明，第22、24、27页原理图，第28页Rev2.2变更记录。注意正文仍描述CMA3000，而Rev2.2原理图/BOM已改为LIS3DH，应结合修订记录阅读。
2. [TI MSP-EXP430F5529 User Experience 1.31 下载页](https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSPEXP430F5529UE/latest/index_FDS.html)：核对原版 `HAL_Cma3000.c`、`HAL_Dogs102x6.c`、`HAL_Wheel.c`、CCS元数据与F5529链接脚本。安装包仅解包读取，未执行安装程序。
3. [ST LIS3DH 产品与数据手册](https://www.st.com/en/mems-and-sensors/lis3dh.html)：身份寄存器、SPI协议、量程、采样率与HR输出格式依据。官方数据手册下载未成功；当前LIS3DH寄存器实现尚待正式手册复核和实物验证，不能视为已核验的硬件兼容性。
4. VTI **CMA3000-D01，8277800A.03，Rev. A.03，2011-04-01，共4页**（《三坐标轴加速度计数据手册.pdf》）。第2页确认 ±2g/400Hz 为56count/g，±8g 为14count/g，灵敏度校准误差±10%，SPI最高500kHz，400Hz启动时间典型10ms；脚注说明100Hz/±2g下灵敏度为50.5count/g。这份4页手册没有寄存器表、详细SPI命令或复位序列，不能用来证明这些实现细节。文件未在仓库中分发。
5. [Linux CMA3000 输入驱动](https://github.com/torvalds/linux/blob/master/drivers/input/misc/cma3000_d0x.c)：核对寄存器地址、RSTR写入0x02/0x0A/0x04及STATUS bit0校验错误的含义。仅作为协议事实参考，没有复制该 GPL 驱动代码；当前 MSP430 SPI 操作由本工程实现，原 TI HAL 许可继续保留。详细寄存器资料仍可在后续继续核对。
6. **信息 Flash 存档依据**：TI `MSP430x5xx and MSP430x6xx Family User's Guide`（SLAU208，Rev. Q）第 7 章与 MSP430F5529 器件头文件（`msp430f5529.h`）。核对结论：F5xx 的 **FCTL2 未实现**（头文件中被注释），不需要配置 Flash 时序分频；写/擦信息存储器前需清 `FCTL4.LOCKINFO`（SLAU208 7.2.1）、清 `FCTL3.LOCK`，并临时把 **`FCTL3.LOCKA` 翻转为 0**（LOCKA 复位为 1，LOCKA=1 会禁止所有信息段的段擦除），完成后恢复三处保护。`FCTL1` 的 ERASE/WRT 位与 `FCTL3` 的 BUSY/LOCK/LOCKA 位由器件头文件提供。擦除/写入序列参考 TI `HAL_FLASH.c/.h`（F5xx_F6xx_Core_Lib）。INFO C/D 地址取自工程内 `lnk_msp430f5529.cmd`（INFOC=0x1880、INFOD=0x1800）。INFO A 存放出厂 TLV/校准数据，工程不访问。该实现尚未在实物上验证。
7. **六关地图**：由离线脚本生成并逐一验证“收集全部能量点后到出口且不经过陷阱”，生成结果固化在 `source/game/game_map.c`；`tools/test_host.ps1` 在每次回归时用状态 BFS 和真实游戏模块重新验证，不依赖脚本运行。地图为本项目自建，没有引用外部地图数据。

## 现场观测（非资料依据）

实物丝印记录为 SEEED-EXP430F5529 V1.0，不能套用 TI Rev 编号判断其芯片。现场观测：TYPE=1、侧立 X≈−1008mg、正反面 Z≈±162mg。它们是现场观察，尚不是测量准确性的认证；排查方法见 [传感器诊断与 Z 轴排查](sensor_diagnostics.md)。

## 第三方代码与许可

LCD 字库、相关初始化序列、滚轮参考实现和元数据来源于 TI BSD 许可示例；原始许可声明保留在对应文件与根目录 `NOTICE_TI.txt` 中。主机测试不依赖 TI 例程运行库。

本工程不包含 CCS 软件、TI 编译器、仿真器驱动或传感器供应商 SDK，也没有声称已通过目标编译。
