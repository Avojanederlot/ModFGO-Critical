#!/system/bin/sh
set -eu
PKG=com.bilibili.fgo.nearme.gamecenter
DIR=/sdcard/Android/data/$PKG/files/Mod
CONF=$DIR/CriticalBeta.conf
STATUS=$DIR/CriticalBeta.status.json
MODCFG=$DIR/ModConfig.yaml
SPEED=$DIR/UnitySpeed.conf

# ── helpers ──────────────────────────────────────────────────
yaml_get() {
  # yaml_get <key> → value (or empty)
  if [ -f "$MODCFG" ]; then
    sed -n "s/^${1}: *//p" "$MODCFG" | tail -n 1
  fi
}

yaml_set() {
  # yaml_set <key> <value>
  mkdir -p "$DIR"
  if [ -f "$MODCFG" ]; then
    cp "$MODCFG" "$MODCFG.bak"
    if grep -q "^${1}:" "$MODCFG"; then
      sed "s/^${1}:.*/${1}: ${2}/" "$MODCFG" > "$MODCFG.$$.tmp"
    else
      cp "$MODCFG" "$MODCFG.$$.tmp"
      printf '%s: %s\n' "$1" "$2" >> "$MODCFG.$$.tmp"
    fi
  else
    printf '%s: %s\n' "$1" "$2" > "$MODCFG.$$.tmp"
  fi
  mv -f "$MODCFG.$$.tmp" "$MODCFG"
  echo saved
}

# ── commands ─────────────────────────────────────────────────
case "${1:-}" in
  speed_get)
    VALUE=1
    if [ -f "$SPEED" ]; then VALUE=$(cat "$SPEED"); fi
    case "$VALUE" in [1-9]|[12][0-9]|30) ;; *) VALUE=1;; esac
    printf '%s\n' "$VALUE"
    ;;
  speed_set)
    [ "$#" = 2 ] || exit 2
    case "$2" in [1-9]|[12][0-9]|30) ;; *) exit 2;; esac
    [ -d /data/user/0/$PKG ] || exit 3
    mkdir -p "$DIR"
    TMP=$SPEED.$$.tmp
    trap 'rm -f "$TMP"' EXIT
    printf '%s\n' "$2" > "$TMP"
    mv -f "$TMP" "$SPEED"
    echo saved
    ;;

  # ── Critical Beta config ───────────────────────────────────
  get)
    VALUE=1
    if [ -f "$CONF" ]; then VALUE=$(sed -n 's/^enabled=\([01]\)$/\1/p' "$CONF" | tail -n 1); fi
    case "$VALUE" in 0|1) ;; *) echo 'Invalid configuration' >&2; exit 2;; esac
    printf '%s\n' "$VALUE"
    ;;

  set)
    [ "$#" = 2 ] || exit 2
    case "$2" in 0|1) ;; *) exit 2;; esac
    [ -d /data/user/0/$PKG ] || { echo 'OPPO FGO is not installed for user 0' >&2; exit 3; }
    mkdir -p "$DIR"
    TMP=$CONF.$$.tmp
    trap 'rm -f "$TMP"' EXIT
    [ ! -f "$CONF" ] || cp "$CONF" "$CONF.bak"
    printf 'enabled=%s\n' "$2" > "$TMP"
    mv -f "$TMP" "$CONF"
    echo saved
    ;;

  status)
    if [ -f "$STATUS" ]; then cat "$STATUS"; else printf '{"stage":"not_started","installed":false,"calls":0,"forced":0,"enemySkipped":0}\n'; fi
    ;;

  pid) pidof "$PKG" || true ;;

  # ── ModConfig.yaml full config ─────────────────────────────
  modcfg_get)
    if [ ! -f "$MODCFG" ]; then
      printf '{"EnableOneHitKill":true,"EnableDematerializer":true,"EnableReplaceSvt":false,"EnableAutoSelectCard":true,"EnablePrintAssetName":false,"CardColor":2,"SetFPS":120,"SvtId":"[]"}\n'
    else
      # Build JSON from yaml keys
      ohk=$(yaml_get EnableOneHitKill)
      dem=$(yaml_get EnableDematerializer)
      rep=$(yaml_get EnableReplaceSvt)
      asc=$(yaml_get EnableAutoSelectCard)
      pan=$(yaml_get EnablePrintAssetName)
      cc=$(yaml_get CardColor)
      fps=$(yaml_get SetFPS)
      svt=$(yaml_get SvtId)
      # Normalize booleans
      case "$ohk" in true) ohk=true;; *) ohk=false;; esac
      case "$dem" in true) dem=true;; *) dem=false;; esac
      case "$rep" in true) rep=true;; *) rep=false;; esac
      case "$asc" in true) asc=true;; *) asc=false;; esac
      case "$pan" in true) pan=true;; *) pan=false;; esac
      [ -n "$cc" ] || cc=2
      [ -n "$fps" ] || fps=120
      [ -n "$svt" ] || svt="[]"
      printf '{"EnableOneHitKill":%s,"EnableDematerializer":%s,"EnableReplaceSvt":%s,"EnableAutoSelectCard":%s,"EnablePrintAssetName":%s,"CardColor":%s,"SetFPS":%s,"SvtId":"%s"}\n' \
        "$ohk" "$dem" "$rep" "$asc" "$pan" "$cc" "$fps" "$svt"
    fi
    ;;

  modcfg_set)
    # modcfg_set <key> <value>
    [ "$#" = 3 ] || { echo 'Usage: ctl.sh modcfg_set <key> <value>' >&2; exit 2; }
    KEY=$2
    VAL=$3
    case "$KEY" in
      EnableOneHitKill|EnableDematerializer|EnableReplaceSvt|EnableAutoSelectCard|EnablePrintAssetName)
        case "$VAL" in true|false) ;; *) echo "Boolean value required (true/false)" >&2; exit 2;; esac
        ;;
      CardColor)
        case "$VAL" in 0|1|2) ;; *) echo "CardColor must be 0, 1 or 2" >&2; exit 2;; esac
        ;;
      SetFPS)
        case "$VAL" in ''|*[!0-9]*) echo "SetFPS must be a number" >&2; exit 2;; esac
        [ "$VAL" -ge 30 ] && [ "$VAL" -le 240 ] || { echo "SetFPS range: 30-240" >&2; exit 2; }
        ;;
      SvtId)
        # Accept as-is (array string)
        ;;
      *)
        echo "Unknown key: $KEY" >&2; exit 2
        ;;
    esac
    yaml_set "$KEY" "$VAL"
    ;;

  modcfg_reset)
    # Reset ModConfig.yaml to defaults
    mkdir -p "$DIR"
    [ ! -f "$MODCFG" ] || cp "$MODCFG" "$MODCFG.bak"
    cat > "$MODCFG.$$.tmp" << 'DEFAULTS'
EnableOneHitKill: true
EnableDematerializer: true
EnableReplaceSvt: false
EnableAutoSelectCard: true
EnablePrintAssetName: false
CardColor: 2
SetFPS: 120
SvtId: []
DEFAULTS
    mv -f "$MODCFG.$$.tmp" "$MODCFG"
    echo saved
    ;;

  *) echo 'Usage: ctl.sh get|set 0|1|status|pid|modcfg_get|modcfg_set <key> <value>|modcfg_reset' >&2; exit 2 ;;
esac
