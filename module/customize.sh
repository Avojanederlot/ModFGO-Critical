SKIPUNZIP=0
PKG=com.bilibili.fgo.nearme.gamecenter
APK=$(pm path "$PKG" | sed -n 's/^package://p' | head -n 1)
[ -n "$APK" ] || abort "OPPO FGO must be installed for user 0"
[ "$ARCH" = arm64 ] || abort "Only arm64 is supported"
HASH=$(sha256sum "${APK%/*}/lib/arm64/libil2cpp.so" | awk '{print $1}')
[ "$HASH" = ce605921ad42c6a2b0dc8dc8743387442bff22204d05d520ea2a6955ad7304e6 ] || abort "Only OPPO 2.129.0 (245) is supported"
HASH=$(sha256sum /data/adb/modules/zygisk-ModFGO/zygisk/arm64-v8a.so 2>/dev/null | awk '{print $1}')
[ "$HASH" = 77f6017fd802c30d403dcbd50038c46f99914f6196a1b4ce3ad80ed023932756 ] || abort "Install the matching ModFGO OPPO compatibility module first"
[ ! -f /data/adb/modules/zygisk-ModFGO/disable ] || abort "Enable the matching ModFGO module first"
set_perm "$MODPATH/ctl.sh" 0 0 0755
set_perm "$MODPATH/zygisk/arm64-v8a.so" 0 0 0644
set_perm "$MODPATH/payload/arm64-v8a/libfgomod_payload.so" 0 0 0644
ui_print "ModFGO 控制台 0.2.0-beta.7"
ui_print "Reboot after installation, then open this module WebUI."
ui_print "All features configurable via unified WebUI."
