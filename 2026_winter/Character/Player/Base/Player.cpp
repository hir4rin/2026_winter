#include "Player.h"
#include "PlayerState.h"
#include "PlayerStateIdle.h"
#include "PlayerStateMove.h"
#include "../Weapon.h"
#include "HitCol.h"
#include "../../../Camera/CameraManager.h"
#include "../../../Camera/LockOnManager.h"
#include "../../../DataLoader/DataManager.h"
#include "Enemy/EnemyBase.h"
#include "../../../System.h"
#include "../../../Math/Matrix4x4.h"
#include "../../../Camera/Camera.h"
#include "../../../Collider/SphereShape.h"
#include "../../../Collider/CapsuleShape.h"
//#include "../../../Stage/Stage.h"
#include "../../../Input.h"
#include "EffekseerForDXLib.h"
//#include "../Effect/EffectManager.h"
#include <cmath>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>

namespace
{
	constexpr float kPlayerOffset = 40.0f;//かプセルの足のオフセット
	constexpr float kPlayerHead = 120.0f;//カプセルの頭の高さ

	constexpr float kArea1MaxZ = 1197;//エリア1のz座標の範囲//上側
	constexpr float kArea2MaxZ = 5275;//エリア2のz座標の範囲//右側

	const Vector3 kArea1EfPos = Vector3(105.0f, 0.0f, 1197.0f);

	constexpr float kRadius = 40.0f;
	constexpr float kRadiusHit = 50.0f;

	constexpr float kCameraShakePower = 2.5f;//カメラの揺れの強さ
	constexpr float kCameraShakeTime = 5.0f;//カメラの揺れの時間

	constexpr float kWingScale = 0.35f;//鴉の羽モデルのスケール
	const Vector3 kWingOffset = Vector3(0.0f, -10.0f, -40.0f);//鴉の羽モデルのオフセット

	constexpr float kRotationLerpFactor = 0.1f;//モデルの向きを目標角度に近づける割合(ほぼlerp)

	constexpr float kGroundCheckCapsuleTopOffset = 100.0f;//地面判定用カプセルの上端のy軸オフセット
	constexpr float kGroundCheckCapsuleBottomOffset = -20.0f;//地面判定用カプセルの下端のy軸オフセット
	constexpr float kGroundCheckCapsuleRadius = 40.0f;//地面判定用カプセルの半径

	constexpr float kSoftTargetKeepFrame = 90.0f;//攻撃をやめてから内部ターゲットを保持するフレーム数
	constexpr float kSoftTargetRangeMult = 3.0f;//これ以上離れたら内部ターゲットを消す(ロックオン範囲に対する倍率)
}


Player::Player()
{
	m_hp = kInitialHp;
	m_comboInfo.SkillGauge = kMaxGaugeValue;
	m_comboInfo.UltGauge = kMaxGaugeValue;
	//m_modelHandle = MV1LoadModel("data/Player/Player.mv1");
	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerModel));
	m_waistFrame = MV1SearchFrame(m_modelHandle, "pelvis");//腰のボーン
	//m_modelHandle = MV1LoadModel("data/Player/Player_true.mv1");
	//m_modelHandle = MV1LoadModel("data/Player/1danme.mv1");

	//攻撃のモデルの読み込み
	m_attackModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerAttackModel));
	m_whiteHandle = LoadGraph("data/UI/white.png");

	m_rb.m_pos = Vector3(0, 0, 0);

	//モデルの初期位置を設定する//前を向いているようにする
	Matrix4x4 rotY = Matrix4x4::MakeRotationY(DX_PI_F);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);
	Matrix4x4 mtx = trans * rotY;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));

	//コンボチェーンの初期化
	InitializeComboChain();
	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetPlayerAnimData();
	m_animNames = animData.animNames;

	//鴉の羽のモデルの読み込み
	m_wingModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerWingModel));
	WingUpdate();//鴉状態の羽の更新
}

Player::~Player()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	MV1DeleteModel(m_modelHandle);
	MV1DeleteModel(m_attackModelHandle);
	MV1DeleteModel(m_wingModelHandle);
	DeleteGraph(m_whiteHandle);
}

void Player::Init()
{
	//向いている方向
	m_targetVec = Vector3(0, 0, 1);//最初は前を向いているようにする
	//初期状態をIdleにする//アニメーションの初期化
	m_anim.Init(m_modelHandle, GetAnimName("Idle"), true);
	//weak_from_this()は、shared_ptrを作成,
	//Playerクラスのインスタンスから、Playerクラスのshared_ptrを取得できるようになる
	m_currentState = std::make_shared<PlayerStateIdle>(GetWeakPtr());

	//IDの取得
	SetID();

	//当たり判定の初期化
	//中心点、オフセット、半径、当たり判定のタイプ、タグ、当たり判定が有効かどうか
	ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, kPlayerOffset, 0),
		.shape = std::make_unique<CapsuleShape>(Vector3(0,kPlayerHead,0),kRadius),
		.tag = {Collider::Faction::Player, Collider::ColRole::None},
		.isActive = true
		});

	//地面への吸着を行う
	SetUseGroundSnap(true);

	//やられ判定の初期化
	InitHitCol(GetWeakPtr());
	m_hitCol->ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, 100.0f, 0),
		.shape = std::make_unique<SphereShape>(kRadiusHit),
		.tag = {Collider::Faction::Player, Collider::ColRole::Hit},
		.isActive = true,
		.isTrigger = true
		});

	CharacterBase::ApplyPos();//座標の更新//モデルの座標を更新する
	ChangeState(m_currentState);//初期化

	//武器の生成
	m_weapon = std::make_shared<Weapon>(GetWeakPtr());//武器の生成//Playerクラスのインスタンスから、Playerクラスのshared_ptrを取得できるようになる
	//effectの生成
	m_efHandle = System::GetInstance().GetHandle(AsyncData::PlayerEffectSkill);
	for (auto& handle : m_efAreaMaxHandle)
	{
		handle = System::GetInstance().GetHandle(AsyncData::AreaWallEffect);
	}
	for (auto& handle : m_efAreaMinHandle)
	{
		handle = System::GetInstance().GetHandle(AsyncData::AreaWallEffect);
	}
	m_lockOnManager = std::make_shared<LockOnManager>();
	m_lockOnManager->Init(GetWeakPtr(), m_cameraManager, m_enemyManager);
}

void Player::Update(Camera& camera)
{
	//ライト(雑実装)
	auto& input = Input::GetInstance();

	//被ダメ後の無敵時間の更新
	if (m_damageInfo.damageTimer > 0.0f)
	{
		m_damageInfo.damageTimer -= 1.0f * System::GetInstance().GetTimeScale();//被ダメ後の無敵時間を減らす
	}

#ifdef _DEBUG
	{
		if (input.IsTriggered("Start"))
		{
			AddSkillGauge(20);
			AddUltGauge(20);
		}
		
	}
#endif


	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(Jump,Fall,Attack)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);

	//ロックオンと内部ターゲットの更新
	m_lockOnManager->Update();
	UpdateSoftTarget();
	


	if (m_currentState)
	{
		m_currentState->Update();//状態の更新
	}


	//これらは押し戻しの時に呼ばれないのでずれる→そこでも呼ぶ必要あり
	WingUpdate();
	m_weapon->Update();//武器の更新

	//回転処理//座標も行列で更新
	//UpdateAngle();
	//UpdateAngleAndPos();

	//-----------------------変わらなかった；；-----------------------------------------
	////ルートモーションONの場合、
	////アニメーションが適用された後のモデルの行列を取得
	//MATRIX modelMat = MV1GetMatrix(m_modelHandle);

	////モデルの行列から、移動量を取得する
	//m_rb.m_pos.x = modelMat.m[3][0];
	//m_rb.m_pos.y = modelMat.m[3][1];
	//m_rb.m_pos.z = modelMat.m[3][2];
	////Y軸回転角度を合わせる
	//m_rotAngleY = atan2f(modelMat.m[0][2], modelMat.m[2][2]);//モデルの行列から、Y軸の回転角度を取得する

	////方向ベクトルを同期
	//m_targetVec.x = sinf(m_rotAngleY + DX_PI_F);
	//m_targetVec.z = cosf(m_rotAngleY + DX_PI_F);
	//m_targetVec.y = 0.0f;
	//m_targetVec = m_targetVec.Normalize();
	//----------------------------------------------------------------------------------------
}
void Player::Draw()
{
	//モデルの描画
	if (m_anim.GetModelHandleForCheck() == m_modelHandle)
	{
		MV1DrawModel(m_modelHandle);
	}
	else
	{
		MV1DrawModel(m_attackModelHandle);
	}

	//鴉状態のときのみ描画
	if (m_isRaven)MV1DrawModel(m_wingModelHandle);
	m_weapon->Draw();//武器
	//コンボチェーンの描画
	for (int i = 0; i < m_comboChain.size(); ++i)
	{
		//std::string text = std::to_string(i) + ": " + m_comboChain[i].animName;
	}
#ifdef _DEBUG
	if (m_currentState)
	{
		m_currentState->DebugDraw();//デバッグ描画
	}
	//DrawSphere3D(m_rb.m_pos.ToDxLibVector(), kPlayerRockOnRange * 5, 16, GetColor(255, 0, 0), GetColor(255, 0, 0), false);
#endif

}

void Player::EffectDraw()
{
	//Vector3 offset = m_targetVec * 50.0f;
	// 再生中のエフェクトを移動する。

	
}

void Player::OnCollision(Collider& other)
{
}

void Player::OnDamage(Collider& other, AttackData& data)
{
#ifdef _DEBUG
	//return;
#endif
	//ダメージを受けたときの処理
	//敵の攻撃データをもらい、ダメージを減らし、体力を減らす、場合によってはプレイヤーを吹き飛ばす
	//DrawFormatString(0, 0, GetColor(255, 0, 0), "Player: OnDamage");
	//ダメージを受けたときの処理
	if (m_damageInfo.damageTimer <= 0.0f)//無敵時間が終わっている場合のみダメージを受ける
	{
		//体力を減らす
		m_hp -= static_cast<int>(data.attackPower);
		//ダメージが入った瞬間にSEとエフェクトを出す
		//System::GetInstance().GetSoundManager().PlaySE("PlayerHit");
		//EffectManager::GetInstance().Play(AsyncData::PlayerDamageEffect, m_rb.m_pos + Vector3(0, kPlayerCenter, 0));
		//無敵時間を設定する
		m_damageInfo.damageTimer = m_damageInfo.kDamageTime;
		//リザルト集計用//被弾回数を加算する
		m_damageTakenCount++;
		if (m_hp <= 0)
		{
			//死亡ステートにする
			ChangeState(std::make_shared<PlayerStateDie>(GetWeakPtr(), data));
			return;
		}
	}
	//無敵時間中はダメージを受けない
	else
	{
		return;
	}
	//被ダメdataを更新する
	m_attackData = data;

	//スキル攻撃中、必殺技中は被弾アニメーションに遷移しない(ダメージ自体は通常通り受ける)
	bool isSkillAttacking = m_comboInfo.currentComboIndex == ComboIndex::SkillAttack1 ||
		m_comboInfo.currentComboIndex == ComboIndex::SkillAttack2 ||
		m_comboInfo.currentComboIndex == ComboIndex::SkillAttack3;
	bool isUltAttacking = std::dynamic_pointer_cast<PlayerStateUlt>(m_currentState) != nullptr;
	if (isSkillAttacking || isUltAttacking)return;

	//stateをhitStateにする
	ChangeState(std::make_shared<PlayerStateHit>(GetWeakPtr(), data));
}

void Player::ForceIdleState()
{
	ChangeState(std::make_shared<PlayerStateIdle>(GetWeakPtr()));


	//IsHitStateに変える//後でする



}

void Player::OnAttackHit(int otherId)
{

	auto cameraManager = GetCameraManager().lock();

	////ownerに当たったことを連絡->AttackMoveを止める
	//bool isLockOn = mainCamera->GetIsLockOn();
	//if (isLockOn)
	//{
	//	//ロックオンしている敵がいて、そいつに当たったら攻撃の移動を止める
	//	auto playerTarget = GetTargetEnemy();
	//	auto targetEnemy = playerTarget.lock();
	//	if (!targetEnemy)
	//	{
	//		assert(false && "PlayerAttackOnCollision:ターゲットしている敵がいません");
	//	}
	//	if (otherId == targetEnemy->GetId())
	//	{
	//		//攻撃の移動を止める
	//		auto& comboInfo = GetComboInfo();
	//		comboInfo.isHit = true;//攻撃が当たったことを通知する//これで、攻撃の移動を止める
	//	}
	//}
	////ロックオンしていない場合
	//else
	//{
	//	//内部ターゲットにセットする
	//	lockOnManager->SetTargetEnemy(otherId);

	//}
}

void Player::SetResultUp()
{

	int texNum = MV1GetTextureNum(m_modelHandle);
	for (int i = 0; i < texNum; i++) {
		MV1SetTextureGraphHandle(m_modelHandle, i, m_whiteHandle, FALSE);
	}

	int matNum = MV1GetMaterialNum(m_modelHandle);
	const COLOR_F kResultMatColorOff = GetColorF(0.0f, 0.0f, 0.0f, 1.0f);//反射・環境光を消す色
	const COLOR_F kResultEmissiveColor = GetColorF(1.0f, 0.0f, 0.0f, 1.0f);//発光色(赤)
	for (int i = 0; i < matNum; i++) {
		MV1SetMaterialDifColor(m_modelHandle, i, kResultMatColorOff); // 拡散反射は消す
		MV1SetMaterialSpcColor(m_modelHandle, i, kResultMatColorOff); // 鏡面ハイライトも消す
		MV1SetMaterialAmbColor(m_modelHandle, i, kResultMatColorOff); // 環境光も消す
		MV1SetMaterialEmiColor(m_modelHandle, i, kResultEmissiveColor); // 発光色だけ赤に
	}
}

void Player::ChangeResultMove()
{
	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}

	//newStateに更新
	m_prevState = m_currentState;
	std::shared_ptr<PlayerState> state = std::make_shared<PlayerStateResultMove>(GetWeakPtr());
	m_currentState = state;
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}

}

void Player::ChangeState(std::shared_ptr<PlayerState> newState)
{
	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}
	//newStateに更新
	m_prevState = m_currentState;
	m_currentState = newState;
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}
}

void Player::AddSkillGauge(int value)
{
	m_comboInfo.SkillGauge += value;
	if (m_comboInfo.SkillGauge > kMaxGaugeValue)
	{
		m_comboInfo.SkillGauge = kMaxGaugeValue;
	}
}

void Player::AddUltGauge(int value)
{
	m_comboInfo.UltGauge += value;
	if (m_comboInfo.UltGauge > kMaxGaugeValue)
	{
		m_comboInfo.UltGauge = kMaxGaugeValue;
	}
}

bool Player::IsLockOn()const
{
	//trueでロックオン中
	return m_lockOnManager && m_lockOnManager->IsLockOn();
}

void Player::SetSoftTarget(std::shared_ptr<EnemyBase> target)
{
	m_softTarget = target;
	//制限時間をセット
	m_softTargetKeepTimer = kSoftTargetKeepFrame;
}

std::shared_ptr<EnemyBase> Player::GetSoftTarget() const
{
	auto target = m_softTarget.lock();
	if (!target || target->GetIsLifeZero())return nullptr;
	return target;
}

std::shared_ptr<EnemyBase> Player::GetAttackTarget() const
{
	//ロックオンしているならロックオンの敵を返す

	if (IsLockOn())
	{
		return m_lockOnManager->GetLockTarget();
	}
	//違うなら内部ターゲットを返す
	return GetSoftTarget();
}

std::shared_ptr<EnemyBase> Player::GetAssasinTarget() const
{
	return m_assasinTarget.lock();
}

const bool Player::CanAssasin() const
{
	auto assasinTarget = GetAssasinTarget();

	return assasinTarget ? true : false;
}

std::shared_ptr<EnemyBase> Player::GetPartBrokenTarget() const
{
	return m_partBrokenTarget.lock();
}

const bool Player::CanPartBrokenFinish() const
{
	auto partBrokenTarget = GetPartBrokenTarget();

	return partBrokenTarget ? true : false;
}




void Player::InitializeComboChain()
{
	//コンボチェーンの初期化
	//音の出すタイミングは、攻撃の当たり判定を有効にするタイミングと同じにする予定なので、コンボノードの中に入れる

	//CSVファイルを読み込む
	const auto& rawData = DataManager::GetInstance().GetComboRawData();

	for (const auto& tokens : rawData)
	{
		//列数チェック//tokensはvector<string>で、1行分のデータが入っている//横
		if (tokens.size() < ComboNodeType::Size)//vectorのサイズなので、push_backした行の数
		{
			assert(false && "ComboChain.csvの列数が不足しています");
			continue;
		}
		ComboNode node;
		node.animName = tokens[ComboNodeType::AnimName];
		node.modelType = std::stoi(tokens[ComboNodeType::Model]);
		node.type = static_cast<AttackType>(std::stoi(tokens[ComboNodeType::Type]));
		node.index = std::stoi(tokens[ComboNodeType::Index]);
		node.attackPower = std::stof(tokens[ComboNodeType::AttackPower]);
		node.moveFrame = std::stof(tokens[ComboNodeType::MoveTimeRate]);
		node.moveSpeedX = std::stof(tokens[ComboNodeType::MoveSpeedX]);
		node.moveSpeedY = std::stof(tokens[ComboNodeType::MoveSpeedY]);
		//nextWeakAttack(空なら空vector)
		if (!tokens[ComboNodeType::NextLightAttack].empty())
		{
			std::istringstream weakSS(tokens[ComboNodeType::NextLightAttack]);
			std::string idx;
			while (std::getline(weakSS, idx, ';'))
			{
				node.nextWeakAttack.push_back(std::stoi(idx));
			}
		}

		//nextHeavyAttack(空なら空vector)
		if (!tokens[ComboNodeType::NextHeavyAttack].empty())
		{
			std::istringstream heavySS(tokens[ComboNodeType::NextHeavyAttack]);
			std::string idx;
			while (std::getline(heavySS, idx, ';'))
			{
				node.nextHeavyAttack.push_back(std::stoi(idx));
			}
		}
		node.knockBackXZ = std::stof(tokens[ComboNodeType::knockBackXZ]);
		node.knockBackY = std::stof(tokens[ComboNodeType::knockBackY]);
		node.isKirimomi = tokens[ComboNodeType::IsKirimomi] == "1" ? true : false;//CSVの値が1ならtrue、0ならfalse
		node.seFrameRate = std::stof(tokens[ComboNodeType::SeFrameRate]);
		node.seName = tokens[ComboNodeType::SeName];
		node.attackColStartRate = std::stof(tokens[ComboNodeType::AttackColStartRate]);
		node.attackColEndRate = std::stof(tokens[ComboNodeType::AttackColEndRate]);
		//空欄なら-1(総フレーム数を使う)
		node.endFrame = tokens[ComboNodeType::EndFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::EndFrame]);
		m_comboChain.push_back(node);
	}

}

void Player::UpdateAngle()
{
	//float targetAngle = 0.0f;//目標の角度
	//if (m_targetVec.Magnitude() > 0.0f)//最初の入力されないとき以外、ここを通り、モデルの向きを変える
	//{
	//	//プレイヤーの移動方向にモデルの方向を近づける
	//	targetAngle = atan2f(m_targetVec.x, m_targetVec.z);//移動ベクトルのx成分とz成分から、プレイヤーの向きたい方向の角度を求める
	//	// Y軸回転行列を作成する//この工程は毎フレーム、原点からプレイヤーの位置に移動してから、回転する行列を作成している
	//	//180度ずれてたので、回転角度を180度ずらす
	//	/* m_rotAngle = targetAngle - DX_PI_F;*/

	//	 //角度の差分を計算//回転角度を-90から90にするため(最短経路を選択)
	//	float difference = targetAngle - m_rotAngleY - DX_PI_F;
	//	while (difference > DX_PI_F) difference -= 2.0f * DX_PI_F;
	//	while (difference < -DX_PI_F) difference += 2.0f * DX_PI_F;
	//	//targetAngle + DX_PI_F
	//	m_rotAngleY += difference * 0.1f;//回転角度を少しずつ目標の角度に近づける//ほぼlerp
	//}	
	//	Matrix4x4 rotY = Matrix4x4::MakeRotationY(m_rotAngleY);
	//	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	//	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);

	//	Matrix4x4 mtx = trans * rotY;
	//	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));
}

void Player::WingUpdate()
{
	//モデルハンドルの取得
	int modelHandle = -1;
	if(m_anim.GetModelHandleForCheck() == m_modelHandle)
	{
		modelHandle = m_modelHandle;
	}
	else
	{
		modelHandle = m_attackModelHandle;
	}


	//モデルフレームのローカルワールド行列を取得
	MATRIX mat = MV1GetFrameLocalWorldMatrix(modelHandle, kPlayerNeckBoneIndex);//モデルフレームのローカルワールド行列を取得

	////武器の位置を取得
	//Vector3 weaponPos = MV1GetFramePosition(m_ownerHandle, slotIndex);

	//MATRIX transmat = MGetTranslate(weaponPos.ToDxLibVector());
	//90度回転させる
	MATRIX rotmat = MGetRotY(DX_PI_F / 2.0f);//回転行列を作成する//90度回転させる
	MATRIX rotXmat = MGetRotX(DX_PI_F / 2.0f);//回転行列を作成する//90度回転させる
	MATRIX rotZmat = MGetRotZ(DX_PI_F / 2.0f);//回転行列を作成する//90度回転させる

	rotmat = MMult(rotXmat, rotmat);//回転行列を掛ける//90度回転させる
	rotmat = MMult(rotZmat, rotmat);//回転行列を掛ける//90度回転させる
	mat = MMult(rotmat, mat);//回転行列を掛ける//90度回転させる

	MATRIX scale = MGetScale(VGet(kWingScale, kWingScale, kWingScale));//スケーリング行列を作成する//モデルの大きさを半分にする

	mat = MMult(scale, mat);//スケーリング行列を掛ける//モデルの大きさを半分にする

	//オフセット
	MATRIX transmat = MGetTranslate(kWingOffset.ToDxLibVector()); // -Z が後ろ
	//これを先に掛けることで、ローカル空間でのオフセットを適用する
	mat = MMult(transmat, mat);

	//モデルにマトリクスをセット
	MV1SetMatrix(m_wingModelHandle, mat);
}
void Player::ApplyPos()
{
	//モデルの座標を更新する
	CharacterBase::ApplyPos();
	//攻撃モデルの座標を更新する
	ApplyPosWithAttackModel();
	////移動制限
	//for (int i = 0; i < static_cast<int>(WaveNumForPlayer::WaveSize); ++i)
	//{
	//	//最期のiならbreakする//最後のiは、ウェーブがないエリアなので、制限しない
	//	if (i == static_cast<int>(WaveNumForPlayer::WaveSize) - 1)break;
	//	switch (i)
	//	{
	//		case static_cast<int>(WaveNumForPlayer::Wave1):
	//			if (m_isWaveArea[i])
	//			{
	//				if (m_rb.m_pos.z >= kArea1MaxZ)
	//				{
	//					{
	//						m_rb.m_pos.z = kArea1MaxZ;
	//					}
	//				}
	//				//エリア制限のエフェクトを出す
	//				if (m_efAreaMaxPlayingHandle[i] == -1)
	//				{
	//					m_efAreaMaxPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMaxHandle[i]);
	//					SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//				}
	//				else
	//				{
	//					SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//				}

	//			}
	//			//違うならエフェクトを止める
	//			else
	//			{
	//				if (m_efAreaMinPlayingHandle[i] != -1)
	//				{
	//					StopEffekseer3DEffect(m_efAreaMinPlayingHandle[i]);
	//					m_efAreaMinPlayingHandle[i] = -1;
	//				}
	//				if (m_efAreaMaxPlayingHandle[i] != -1)
	//				{
	//					StopEffekseer3DEffect(m_efAreaMaxPlayingHandle[i]);
	//					m_efAreaMaxPlayingHandle[i] = -1;
	//				}
	//			}
	//			break;
	//			case static_cast<int>(WaveNumForPlayer::Wave2):
	//				if (m_isWaveArea[i])
	//				{
	//					if (m_rb.m_pos.z <= kArea1MaxZ)
	//					{
	//						m_rb.m_pos.z = kArea1MaxZ;
	//					}
	//					if (m_rb.m_pos.z >= kArea2MaxZ)
	//					{
	//						m_rb.m_pos.z = kArea2MaxZ;
	//					}
	//					//エリア制限のエフェクトを出す
	//					//下限
	//					if (m_efAreaMinPlayingHandle[i] == -1)
	//					{
	//						m_efAreaMinPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMinHandle[i]);
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMinPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1MaxZ);
	//					}
	//					else
	//					{
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMinPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//					}
	//					//上限
	//					if (m_efAreaMaxPlayingHandle[i] == -1)
	//					{
	//						m_efAreaMaxPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMaxHandle[i]);
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1MaxZ);
	//					}
	//					else
	//					{
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea2MaxZ);
	//					}
	//				}	
	//				//違うならエフェクトを止める
	//				else
	//				{
	//					if (m_efAreaMinPlayingHandle[i] != -1)
	//					{
	//						StopEffekseer3DEffect(m_efAreaMinPlayingHandle[i]);
	//						m_efAreaMinPlayingHandle[i] = -1;
	//					}
	//					if (m_efAreaMaxPlayingHandle[i] != -1)
	//					{
	//						StopEffekseer3DEffect(m_efAreaMaxPlayingHandle[i]);
	//						m_efAreaMaxPlayingHandle[i] = -1;
	//					}
	//				}
	//				break;
	//			default:
	//				for (auto& playingHandle : m_efAreaMinPlayingHandle)
	//				{
	//					if (playingHandle != -1)
	//					{
	//						StopEffekseer3DEffect(playingHandle);
	//						playingHandle = -1;
	//					}
	//				}
	//				for (auto& playingHandle : m_efAreaMaxPlayingHandle)
	//				{
	//					if (playingHandle != -1)
	//					{
	//						StopEffekseer3DEffect(playingHandle);
	//						playingHandle = -1;
	//					}
	//				}
	//				break;
	//	}
	//}

	//下方向とのレイキャストで、地面から空中に遷移したかを判定する
	Vector3 causuleTop = m_rb.m_pos + Vector3(0, kGroundCheckCapsuleTopOffset, 0);//カプセルの上端の座標//レイキャストの始点
	Vector3 causuleBottom = m_rb.m_pos + Vector3(0, kGroundCheckCapsuleBottomOffset, 0);//カプセルの下端の座標//レイキャストの終点

	Vector3 rayCastSphere = m_rb.m_pos;
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	auto hitDim = MV1CollCheck_Capsule(stage->GetStageModelHandle(), -1, causuleTop.ToDxLibVector(), causuleBottom.ToDxLibVector(), kGroundCheckCapsuleRadius);
	//	//地面にいる判定はCollisionMangerの押し戻し処理の際にしているのでしない
	//	if (hitDim.HitNum > 0)
	//	{
	//	}
	//	//当たっていなかったら、地面にいないと判定する
	//	else
	//	{
	//		//地面にいたら、地面から離れたときの処理をする
	//		if (IsFloor())
	//		{
	//			ChangeState(std::make_shared<PlayerStateFall>(GetWeakPtr()));//落下ステートにする
	//		}

	//		SetIsFloor(false);
	//		m_isGround = false;

	//	}
	//	MV1CollResultPolyDimTerminate(hitDim);
	//}

		//歩いて地面から離れたら落下ステートにする
	if (IsLeftFloor())
	{
		ChangeState(std::make_shared<PlayerStateFall>(GetWeakPtr()));
	}



	//プレイヤーが地上にいたら空中攻撃をリセット
	if (IsFloor())
	{
		m_comboInfo.isAirAttack = false;
		m_comboInfo.isAirSkillAttack = false;
	}

	WingUpdate();
	m_weapon->Update();//武器の更新
}

void Player::ApplyPosWithAttackModel()
{
	float targetAngle = 0.0f;//目標の角度
	if (m_targetVec.Magnitude() > 0.0f)//最初の入力されないとき以外、ここを通り、モデルの向きを変える
	{
		//モデルの移動方向にモデルの方向を近づける
		targetAngle = atan2f(m_targetVec.x, m_targetVec.z);//移動ベクトルのx成分とz成分から、プレイヤーの向きたい方向の角度を求める
		// Y軸回転行列を作成する//この工程は毎フレーム、原点からモデルの位置に移動してから、回転する行列を作成している
		//180度ずれてたので、回転角度を180度ずらす
		/* m_rotAngle = targetAngle - DX_PI_F;*/
		 //角度の差分を計算//回転角度を-90から90にするため(最短経路を選択)
		float difference = targetAngle - m_rotAngleY - DX_PI_F;
		while (difference > DX_PI_F) difference -= 2.0f * DX_PI_F;
		while (difference < -DX_PI_F) difference += 2.0f * DX_PI_F;
		//targetAngle + DX_PI_F
		m_rotAngleY += difference * kRotationLerpFactor;//回転角度を少しずつ目標の角度に近づける//ほぼlerp
	}
	//モデルは、座標の位置のcenter分下で表示

	Matrix4x4 rotY = Matrix4x4::MakeRotationY(m_rotAngleY);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);

	Matrix4x4 mtx = trans * rotY;
	MV1SetMatrix(m_attackModelHandle, Matrix4x4::ToDxLibMatrix(mtx));
}

bool Player::CanSkillAttack(bool changeGauge)
{
	bool canSkill = m_comboInfo.SkillGauge >= kSkillAttackGaugeCost;

	if (canSkill)
	{
		if (changeGauge)m_comboInfo.SkillGauge -= kSkillAttackGaugeCost;
		return true;
	}

	return false;
}

bool Player::CanUltAttack()
{
	bool canUlt = m_comboInfo.UltGauge >= kMaxGaugeValue;

	if (canUlt)
	{
		m_comboInfo.UltGauge -= kMaxGaugeValue;
		return true;
	}

	return false;
}

void Player::UpdateSoftTarget()
{
	auto target = m_softTarget.lock();
	if (!target)return;

	//死んだら消す
	if (target->GetIsLifeZero())
	{
		ClearSoftTarget();
		return;
	}
	//離れすぎたら消す
	float distance = (target->GetRigidBody().GetPos() - m_rb.m_pos).Magnitude();
	if (distance > kPlayerRockOnRange * kSoftTargetRangeMult)
	{
		ClearSoftTarget();
		return;
	}
	//攻撃中はタイマーを進めない//おそらく空振りの攻撃でもタイマーがとまってしまうため、バグるかも
	if (m_comboInfo.currentComboIndex != ComboIndex::None)
	{
		m_softTargetKeepTimer = kSoftTargetKeepFrame;
		return;
	}
	m_softTargetKeepTimer -= System::GetInstance().GetTimeScale();
	//TImerが過ぎていたら内部ターゲットを解放する
	if (m_softTargetKeepTimer <= 0.0f)
	{

		ClearSoftTarget();
	}

}

