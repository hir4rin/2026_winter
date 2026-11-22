#!/bin/bash
# Unity で書き出し → ビューアで mv1 → 今の mv1 にアニメだけ入れ替え → 比べる
# run_all.sh 作業フォルダ [noexport]
# 作業フォルダ: ビューアのコピー(DxLibModelViewer_64bit.exe と LoadAndSave=1 の Setting.ini)、前の mv1(Enemy_old.mv1 / EnemyGun_old.mv1)
# ビューアの変換は同じ FBX でもときどき壊れた値になるので、今のアニメと比べて差が大きければ変換し直す
W="$1"
T="$(cd "$(dirname "$0")" && pwd)"
P="C:/Users/tsyts/Desktop/programm/ninja_motion"
D="$T/../../2026_winter/data/Enemy"
if [ "$2" != "noexport" ]; then
	"/c/Program Files/Unity/Hub/Editor/2022.3.62f3/Editor/Unity.exe" -batchmode -projectPath "$P" -executeMethod EnemyAnimTool.Export -quit -logFile "$W/unity_export.log"
	cat "$P/Library/EnemyAnimTool.result"
	cp "$P"/Library/EnemyAnimExport/*.fbx "$W/"
fi
for p in "Swordman Enemy" "Gunner EnemyGun"; do
	set -- $p
	ok=0
	for try in 1 2 3 4 5; do
		cd "$W"; rm -f $1.mv1
		(./DxLibModelViewer_64bit.exe "$W/$1.fbx" &)
		prev=-1; same=0
		# 大きさが12秒変わらなくなるまで待つ
		for i in $(seq 1 150); do
			sleep 3
			if [ -f $1.mv1 ]; then s=$(stat -c %s $1.mv1); if [ "$s" = "$prev" ]; then same=$((same+1)); else same=0; fi; prev=$s; [ $same -ge 4 ] && break; fi
		done
		taskkill //F //IM DxLibModelViewer_64bit.exe >/dev/null 2>&1; sleep 1
		"$T/animmerge.exe" "$W/$2_old.mv1" "$W/$1.mv1" "$W/$2_new.mv1" "$W/merge_$1.txt"
		cd "$T"; ./animcmp.exe "$W/cmp_$1.txt" "$W/$2_old.mv1" "$W/$2_new.mv1"
		r=$(grep -o "^[^ ]*.*maxDiff=[0-9.]*" "$W/cmp_$1.txt" | awk '{split($NF,a,"="); c++; if (a[2]+0>m) {m=a[2]+0; w=$1}} END {print m+0, c, w}')
		miss=$(grep -c "B none" "$W/cmp_$1.txt")
		echo "$1 try$try size=$prev max=$r missing=$miss"
		if awk -v m="${r%% *}" 'BEGIN{exit !(m < 0.5)}' && [ "$miss" = "0" ]; then ok=1; break; fi
	done
	if [ $ok = 1 ]; then cp "$W/$2_new.mv1" "$D/$2.mv1"; grep -hE "differs|saved|tris" "$W/merge_$1.txt"; echo "$1 -> $D/$2.mv1"; else echo "$1 FAILED"; fi
done
echo ALLDONE
