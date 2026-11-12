param([string]$BuildDirectory = '', [string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$taskProjectRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $env:TEMP 'MSP430_GravityMaze_host_tests' }
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
$taskSources = @(
    'tests/test_core.c','tests/save_port_stub.c',
    'source/game/game.c','source/game/game_map.c',
    'source/input/motion_input.c','source/input/key_filter.c',
    'source/input/tuning.c',
    'source/drivers/lcd.c','source/drivers/save.c',
    'source/app/app.c','source/ui/ui.c'
)
$taskIncludes = @('include','source/game','source/input','source/drivers','source/app','source/ui')
$taskArguments = @('-std=c89','-Wall','-Wextra','-Werror','-pedantic','-g')
foreach ($taskInclude in $taskIncludes) { $taskArguments += '-I' + (Join-Path $taskProjectRoot $taskInclude) }
foreach ($taskSource in $taskSources) { $taskArguments += Join-Path $taskProjectRoot $taskSource }
$taskExecutable = Join-Path $BuildDirectory 'test_core.exe'
$taskArguments += @('-o',$taskExecutable)
& $Compiler @taskArguments
if ($LASTEXITCODE -ne 0) { throw 'Host compilation failed.' }
& $taskExecutable
if ($LASTEXITCODE -ne 0) { throw 'Host tests failed.' }

# Exercise the production key driver with simulated GPIO and timer ISR calls.
$taskKeyArguments = @('-std=c89','-Wall','-Wextra','-Werror','-pedantic','-g')
foreach ($taskInclude in @('tests/stubs','include','source/drivers','source/input')) {
    $taskKeyArguments += '-I' + (Join-Path $taskProjectRoot $taskInclude)
}
foreach ($taskSource in @('tests/test_key_driver.c','source/drivers/key.c','source/input/key_filter.c')) {
    $taskKeyArguments += Join-Path $taskProjectRoot $taskSource
}
$taskKeyExecutable = Join-Path $BuildDirectory 'test_key_driver.exe'
$taskKeyArguments += @('-o',$taskKeyExecutable)
& $Compiler @taskKeyArguments
if ($LASTEXITCODE -ne 0) { throw 'Key driver host compilation failed.' }
& $taskKeyExecutable
if ($LASTEXITCODE -ne 0) { throw 'Key driver host tests failed.' }

# Compile the actual SPI sensor driver against a register/transaction model.
$taskAccelArguments = @('-std=c89','-Wall','-Wextra','-Werror','-pedantic','-g')
foreach ($taskInclude in @('tests/accel_stubs','include','source/board','source/drivers')) {
    $taskAccelArguments += '-I' + (Join-Path $taskProjectRoot $taskInclude)
}
foreach ($taskSource in @('tests/test_accel_driver.c','source/drivers/accel_port.c','source/drivers/accel.c')) {
    $taskAccelArguments += Join-Path $taskProjectRoot $taskSource
}
$taskAccelExecutable = Join-Path $BuildDirectory 'test_accel_driver.exe'
$taskAccelArguments += @('-o',$taskAccelExecutable)
& $Compiler @taskAccelArguments
if ($LASTEXITCODE -ne 0) { throw 'Accelerometer host compilation failed.' }
& $taskAccelExecutable
if ($LASTEXITCODE -ne 0) { throw 'Accelerometer host tests failed.' }

# Compile the info-flash port against host register stubs (syntax/C89 only).
# -Wno-int-to-pointer-cast: the host has 64-bit pointers; the target MSP430
# address space is 16-bit, so this diagnostic does not apply there.
$taskFlashArguments = @('-std=c89','-Wall','-Wextra','-Werror','-pedantic',
                        '-Wno-int-to-pointer-cast','-fsyntax-only')
foreach ($taskInclude in @('tests/flash_stubs','include','source/drivers')) {
    $taskFlashArguments += '-I' + (Join-Path $taskProjectRoot $taskInclude)
}
$taskFlashArguments += Join-Path $taskProjectRoot 'source/drivers/save_port.c'
& $Compiler @taskFlashArguments
if ($LASTEXITCODE -ne 0) { throw 'Info-flash port host compilation failed.' }
Write-Host 'info-flash port: compiled against host register stubs (syntax/C89 only)'

# Compile the thumb wheel driver against host ADC/port stubs (syntax/C89 only).
$taskWheelArguments = @('-std=c89','-Wall','-Wextra','-Werror','-pedantic','-fsyntax-only')
foreach ($taskInclude in @('tests/wheel_stubs','include','source/drivers')) {
    $taskWheelArguments += '-I' + (Join-Path $taskProjectRoot $taskInclude)
}
$taskWheelArguments += Join-Path $taskProjectRoot 'source/drivers/wheel.c'
& $Compiler @taskWheelArguments
if ($LASTEXITCODE -ne 0) { throw 'Thumb wheel host compilation failed.' }
Write-Host 'thumb wheel: compiled against host ADC stubs (syntax/C89 only)'
