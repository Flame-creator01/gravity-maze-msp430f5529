> 返回 [文档索引](README.md)

# CCS 5.5.0 导入与编译

## 导入

工程根目录含完整 Eclipse/CCS 元数据，Debug 和 Release 配置均源于 TI 的 CCS5 时代示例模板并修改为本项目路径。不要把工程导入为普通 Makefile 项目。

1. `Project → Import Existing CCS Projects`，选择工程根目录。
2. 如选择 `Copy projects into workspace`，保留整个文件夹结构。禁止单独复制 `.project`。
3. 在 `Properties → General` 确认 Device 为 MSP430F5529，Connection 为 TI MSP430 USB1。
4. 工程默认声明编译器 **4.2.1**，对应 CCS 5.5 时代。若本机版本不同，在该页选择实际已安装的 TI MSP430 编译器，接受 CCS 对构建选项的迁移。没有该编译器时先使用 CCS 安装管理器添加 MSP430 支持。
5. 使用 Debug 配置执行 Build。当前 Debug 优化级别为 0，便于观察变量。

## 核对构建选项

| 项目 | 配置 |
|---|---|
| 芯片宏 | `__MSP430F5529__` |
| 指令集 | MSP430X，`--silicon_version=mspx` |
| 数据模型 | large，`--large_memory_model` |
| 目标格式 | COFF，与本工程传统 CCS5 配置一致 |
| 栈 | 768 字节，最坏占用需实测确认 |
| 堆 | 0，不使用 malloc |
| 库 | `libc.a`，由 CCS 编译器选取匹配运行库 |
| 链接脚本 | 根目录唯一的 `lnk_msp430f5529.cmd` |
| 寄存器符号 | 链接脚本末尾 `-l msp430f5529.cmd`，来自 CCS 的 `msp430/include` |

Include Options 包含 `${CCS_BASE_ROOT}/msp430/include`、`${CG_TOOL_ROOT}/include`、`${PROJECT_ROOT}/include` 和 `source` 下 board、drivers、input、game、app、ui 六个目录。

`tests`、`tools`、`docs`、`reference`、`Debug`、`Release` 已从 CCS 源文件构建中排除。主机测试的替代函数不能加入 MCU 链接。

## 元数据无法迁移时的恢复方法

### 提示 “meta-data cannot be interpreted”

这类报错来自 CCS/CDT 对工程元数据格式的解析，普通 XML 语法检查发现不了。工程已包含 TI 原版工程中的两处 XML 处理指令：

- `.cproject`：XML 声明之后、根元素之前的 `<?fileVersion 4.0.0?>`；
- `.ccsproject`：XML 声明之后、根元素之前的 `<?ccsproject version="1.0"?>`。

两条已核对存在于文件中，但尚未在 CCS 5.5 中验证导入。若遇到该提示：

1. 将工程复制到新的目录，例如 `D:\GravityMaze\MSP430_GravityMaze`，不要覆盖正在使用的工程。
2. 若 Project Explorer 已有同名工程，右键 `Delete` 将其从工作区移除，**不要勾选删除磁盘内容**，保留自己的修改。
3. 重新执行 `Project → Import Existing CCS Projects`，选择新目录。
4. 如果仍然失败，展开错误框的 `Details >>`，保留完整错误文字；必要时查看当前 CCS 工作区下 `.metadata/.log` 的末尾异常。截图里的概括提示本身不能排除其他插件或安装问题。

### 新建本机原生工程

CCS 插件随具体安装不同可能要求重新选编译器。若导入后仍报告 build definition 不存在，创建新的 **Empty CCS Project**：

1. Device 选择 MSP430F5529，Compiler 选择本机 TI MSP430 编译器，使用传统 COFF。
2. 复制 `include`、`source` 和 `targetConfigs`；添加上述 Include Options。
3. 用本工程链接脚本替换自动生成脚本，保证工程中只有一份器件链接脚本。
4. 设置 MSP430X、大数据模型、768 字节栈和 0 字节堆。
5. 编译所有 `source/**/*.c`。不要加入 `tests` 的 `.c` 文件。

## 下载

连接板载 eZ-FET USB，按实验板说明将电源选择开关置 `eZ`。保持 JP16 调试连接短接块及板载电源跳线处于正确位置；主控的 `5529 USB` 是另一接口，不用于本项目 USB 协议功能。下载将覆盖现有用户程序。

打开 `targetConfigs/MSP430F5529.ccxml`；使用 TI MSP430 USB1 连接，Build 后 Debug、Run。仿真器驱动或固件更新需求以 CCS 提示为准。下载后需要让程序自由运行（F8），暂停在 `main` 时不会扫描按键或采样。

仓库中不包含目标 `.out`：仓库维护环境没有 TI 目标编译器，构建产物需由本地 CCS 生成。Flash/RAM 实际占用以 CCS 的 map 文件为准。下一步的上板顺序见 [首次上板与联调步骤](bringup.md)。
