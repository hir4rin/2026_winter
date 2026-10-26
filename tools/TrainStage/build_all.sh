#!/bin/bash
# 列車ステージ(見た目・当たり判定)とトンネルを作り直して mv1 にする(Git Bash で実行)
# 使い方: ./build_all.sh 素材フォルダ [--corridor]
#   素材フォルダ: 各zipを展開したフォルダ(train/, freight-shipping-container-*/, subway/, corridor/ など)
#   --corridor: 宇宙船の通路の焼き込み(prepare_corridor.py)もやり直す(数十秒かかる)
# 注意: ゲームの TrainStage.mv1 / TrainStageCol.mv1 を上書きするので、Unity(TrainStageScene)で調整した分は消える。
#       調整を残したいときは、これを実行せずに Unity の Tools > Train Stage > 2. Export To Game を使う
#       ここで作った TrainStage_Unity.fbx は Unity の Tools > Train Stage > 1. Import で取り込む
set -e
ASSETS="$1"
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
BLENDER="/c/Users/tsyts/game/steamapps/common/Blender/blender.exe"
STAGE="$REPO/2026_winter/data/Stage/TrainStage"
TUNNEL="$REPO/2026_winter/data/Stage/TrainTunnel"
TOOLS="$REPO/tools/FbxToMv1"
TUNNEL_DIF=1.0   # トンネルの色の明るさ(flat変換で 拡散×0.1・環境×0.9 になる)

if [ "$2" = "--corridor" ] || [ ! -f "$STAGE/corridor_car.blend" ]; then
	"$BLENDER" -b "$ASSETS/corridor/Spaceship Corridor_blend/Spaceship corridor.blend" --python "$HERE/prepare_corridor.py" -- "$STAGE/tex" "$STAGE/corridor_car.blend"
fi
"$BLENDER" -b --python "$HERE/build_train_stage.py" -- "$ASSETS" "$STAGE" "$STAGE/corridor_car.blend"
"$BLENDER" -b "$ASSETS/subway/source/subway.blend" --python "$REPO/tools/TrainTunnel/build_tunnel.py" -- "$TUNNEL" 11 4 2
# 使った素材をUnityの部品にする(Unity で Tools > Train Stage > 3. Setup Props を実行するとプレハブになる)
UNITY_PROPS="/c/Users/tsyts/Desktop/programm/ninja_motion/Assets/TrainStage/Props"
if [ -d "$(dirname "$UNITY_PROPS")" ]; then
	"$BLENDER" -b --python "$HERE/export_props.py" -- "$ASSETS" "$STAGE" "$(cygpath -w "$UNITY_PROPS")"
fi

"$BLENDER" -b "$STAGE/TrainStage.blend" --python "$TOOLS/export_x.py" -- "$STAGE" TrainStage TrainStageCol
"$BLENDER" -b "$TUNNEL/TrainTunnel.blend" --python "$TOOLS/export_x.py" -- "$TUNNEL" TrainTunnel TrainTunnelSection dif=$TUNNEL_DIF
w() { cygpath -w "$1"; }
cd "$TOOLS"
./convert.exe flat "$(w "$STAGE/TrainStage.x")" "$(w "$STAGE/TrainStage.mv1")" "$(w "$STAGE/TrainStageCol.x")" "$(w "$STAGE/TrainStageCol.mv1")" \
	"$(w "$TUNNEL/TrainTunnel.x")" "$(w "$TUNNEL/TrainTunnel.mv1")" "$(w "$TUNNEL/TrainTunnelSection.x")" "$(w "$TUNNEL/TrainTunnelSection.mv1")"
rm -f "$STAGE"/*.blend1 "$TUNNEL"/*.blend1 "$STAGE/TrainStage.x" "$STAGE/TrainStageCol.x" "$TUNNEL/TrainTunnel.x" "$TUNNEL/TrainTunnelSection.x"
cat convert_log.txt | grep -v "  mat"
