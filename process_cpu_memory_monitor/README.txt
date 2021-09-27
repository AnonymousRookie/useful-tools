1. 配置cfg.yaml

process: 配置需要监视的进程名称。(建议把本进程zpsutil.exe也添加进去)
interval: 配置秒数，每隔多少秒获取并记录一次进程的CPU、内存使用情况。（不宜太小，太小的话本进程zpsutil.exe也会占用较高的CPU）

2. 运行zpsutil.exe

3. 查看日志

日志保存在zpsutil.exe所在目录中，文件名格式为zpsutil年-月-日.log，每天生成一个新文件。