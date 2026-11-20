# 依赖与来源

- 上游 ModFGO：<https://github.com/hexstr/ModFGO>。本 beta 依赖外部已安装的兼容主模块，不包含上游完整 C++ 源码，不重新许可其二进制。
- `module/zygisk/arm64-v8a.so`：来自本次设备上现有的 `fgomod_zygisk` Zygisk 载入器，作为预编译依赖沿用；其完整源码未在当前工作目录提供。
- `src/critical.c`、`src/test_critical.c`、`module/webroot/*`、`module/ctl.sh`、安装与打包脚本：本次适配新增实现。
- WebUI 调用遵循 KernelSU 公开 exec 桥接协议：<https://github.com/tiann/KernelSU/blob/main/js/index.js>。页面资源全部本地加载，不依赖 CDN。

本说明用于区分来源，不替代各依赖自身的授权条款，也不把未提供源码的载入器声明为完整开源实现。
