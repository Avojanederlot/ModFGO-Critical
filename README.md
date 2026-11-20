# ModFGO 控制台

面向 **OPPO 渠道 FGO 2.129.0（245）** 的 ModFGO 兼容模块，提供暴击适配、「黑暗之衣」被动、1–30 倍速和 KernelSU WebUI。当前版本为 **0.2.0-beta.7**。

## 兼容环境

- 游戏包名：`com.bilibili.fgo.nearme.gamecenter`，Android 用户 0，arm64。
- 需要 KernelSU、已启用的 Zygisk 和配套的 ModFGO OPPO 兼容主模块。
- 安装器会校验游戏 `libil2cpp.so` 和配套主模块的 SHA256；其他版本或渠道请勿直接使用。

## 安装

从 [release-assets](release-assets/) 下载两个 ZIP：

1. 在 KernelSU 安装 `ModFGO-OPPO-2.129.0-compat-test.zip` 并启用，提供配套主模块和初始化兼容补丁。
2. 安装 `ModFGO-Critical-0.2.0-beta.7.zip` 并重启。该包更新 `fgomod_zygisk` 伴随模块。
3. 打开 **ModFGO 控制台** 的 WebUI 设置，再启动游戏。

两个 ZIP 的 SHA256 见 [SHA256SUMS.txt](release-assets/SHA256SUMS.txt)。

## 功能与验证

- **暴击适配**：对我方普通指令的伤害计算接管暴击判定，WebUI 可开关。真机曾观察到入口调用与判定计数增长，最终伤害倍率对照仍待验证。
- **黑暗之衣**：进入战斗时为我方从者添加技能 `960988`，原有被动保留。真机续打后，玛修和大流士三世的状态面板显示该技能；各项效果尚未逐一验证。
- **倍速**：WebUI 可设置 1–30 的整数倍率，1 倍恢复原速。5 倍下已实测选卡、普通攻击与换波；宝具和独立计时动画尚未全面验证。
- **统一 WebUI**：提供暴击、倍速及原 ModFGO 配置项。原项目的其他功能和所有配置组合未完成验证。

暴击、倍速设置在游戏进程中读取，原 ModFGO 配置一般需要重启游戏。暴击诊断计数表示接口活动，不能单独证明最终伤害结果。详情见 [VALIDATION.md](VALIDATION.md)。

## 构建

`module/` 是模块文件，`src/critical.c` 是本次适配载荷源码，`module/zygisk/arm64-v8a.so` 为预编译载入器。`tools/build.py` 可检查并打包现有载荷，也可以传入 Zig 和 Android arm64 系统库重新编译：

```sh
python tools/build.py
python tools/build.py --zig /path/to/zig --lib-dir /path/to/android-arm64-libs
```

依赖来源和源码边界见 [THIRD_PARTY.md](THIRD_PARTY.md)。本仓库不包含上游 ModFGO 及预编译载入器的完整源码。

## 上游

[hexstr/ModFGO](https://github.com/hexstr/ModFGO) · [KernelSU WebUI 文档](https://kernelsu.org/guide/module-webui.html)
