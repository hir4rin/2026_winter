#include "EnemyBase.h"
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Character/Player/Player.h"
#include "Main/System.h"
#include "Main/imguiApp.h"
#include "Battle/BattleManager.h"
#include "Battle/AttackTokenManager.h"
#include "imgui.h"
#include <algorithm>
#include <format>

//EnemyBaseのデバッグウィンドウ(ボスの攻撃を選んで出す・ステータスを見る)
//EnemyBase.cppが長いので分けている

namespace
{
	constexpr float kWindowWidth = 440.0f;//最初のウィンドウの大きさ
	constexpr float kWindowHeight = 680.0f;
	constexpr float kAttackListHeight = 170.0f;//攻撃の一覧の高さ
	constexpr int kAttackListColumnNum = 6;//攻撃の一覧の列数

	constexpr float kLoopIntervalMin = 0.0f;//繰り返しの間隔(実フレーム)
	constexpr float kLoopIntervalMax = 600.0f;

	constexpr float kFrameDragSpeed = 0.5f;//フレーム・距離のドラッグ感度
	constexpr float kRateDragSpeed = 0.01f;//倍率・秒のドラッグ感度

	const ImVec4 kWarningColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);//注意の文字色
	const ImVec4 kActiveColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);//今出している攻撃の文字色
}

void EnemyBase::DrawDebugWindow()
{
	//HPを減らさない
	if (m_isDebugLockHp && !m_isLifeZero) m_hp = m_status.hp;

	//Idleに戻ったら選んだ攻撃をもう一度出す
	UpdateDebugLoopAttack();

	//同じ種類の敵が2体いても別のウィンドウになるように、IDを付ける
	const std::string title = std::format("Enemy Debug [{}]##{}", m_status.type, GetId());
	ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title.c_str()))
	{
		ImGui::End();
		return;
	}

	DrawDebugWindowStatus();
	DrawDebugWindowControl();
	DrawDebugWindowAttack();
	DrawDebugWindowStatusEdit();

	ImGui::End();
}

void EnemyBase::DrawDebugWindowStatus()
{
	ImGui::SeparatorText("Status");

	//ステート名と情報(情報には日本語が入ることがある)
	const char* stateName = m_currentState ? m_currentState->GetName() : "None";
	const std::string stateInfo = m_currentState ? imguiApp::ToUtf8(m_currentState->GetDebugInfo()) : "";
	ImGui::Text("State      : %s %s", stateName, stateInfo.c_str());

	//HP
	const float hpRate = (m_status.hp > 0) ? std::clamp(static_cast<float>(m_hp) / m_status.hp, 0.0f, 1.0f) : 0.0f;
	const std::string hpText = std::format("HP {} / {}", m_hp, m_status.hp);
	ImGui::ProgressBar(hpRate, ImVec2(-1.0f, 0.0f), hpText.c_str());

	ImGui::Text("Pos        : (%.0f, %.0f, %.0f)", m_rb.m_pos.x, m_rb.m_pos.y, m_rb.m_pos.z);
	if (auto player = m_player.lock())
	{
		Vector3 toPlayer = player->GetRigidBody().GetPos() - m_rb.m_pos;
		toPlayer.y = 0.0f;
		ImGui::Text("Distance   : %.0f (MaxRange %.0f)", toPlayer.Magnitude(), GetMaxAttackRange());
	}
	ImGui::Text("Attack CT  : %.0f", (std::max)(m_attackCoolTime, 0.0f));
	if (m_currentAttackIndex >= 0 && m_currentAttackIndex < static_cast<int>(m_attackList.size()))
	{
		ImGui::Text("Last Attack: #%d %s", m_currentAttackIndex, imguiApp::ToUtf8(m_attackList[m_currentAttackIndex].name).c_str());
	}
	ImGui::Text("ForceHit   : %d / %d", m_forceHitCount, m_status.forceHitCount);
	ImGui::Text("GuardGauge : %.0f / %.0f", m_guardInfo.GurardGauge, m_status.parryGauge);

	bool hasToken = false;
	if (auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock())
	{
		hasToken = tokenMgr->HasToken(*this);
	}
	ImGui::Text("Token:%s  Found:%s  Floor:%s", hasToken ? "Yes" : "No", m_isPlayerFound ? "Yes" : "No", IsFloor() ? "Yes" : "No");
	ImGui::Text("LifeZero:%s  Executed:%s  PartBroken:%s", m_isLifeZero ? "Yes" : "No", m_isExecuted ? "Yes" : "No", m_isPartBroken ? "Yes" : "No");
}

void EnemyBase::DrawDebugWindowControl()
{
	ImGui::SeparatorText("Control");

	//AIを止める//オンにした瞬間に、移動中ならIdleにする
	if (ImGui::Checkbox("Stop AI", &m_isDebugStopAI))
	{
		SetDebugIdle(m_isDebugIdleGlobal);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(stay Idle, attack only from here)");
	if (m_isDebugIdleGlobal)
	{
		ImGui::TextColored(kWarningColor, "I key (all enemies Idle) is ON: attacks are canceled");
	}

	ImGui::Checkbox("Lock HP", &m_isDebugLockHp);
	ImGui::SameLine();
	if (ImGui::Button("Reset HP"))
	{
		if (!m_isLifeZero) m_hp = m_status.hp;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset CT"))
	{
		m_attackCoolTime = 0.0f;
	}

	//ステートを直接変える//死亡・処刑中は変えない
	const bool canChange = !m_isLifeZero && !m_isExecuted;
	if (!canChange)ImGui::BeginDisabled();
	if (ImGui::Button("Idle"))ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
	ImGui::SameLine();
	if (ImGui::Button("Chase"))ChangeState(std::make_shared<EnemyChase>(GetWeakPtr()));
	ImGui::SameLine();
	if (ImGui::Button("Caution"))ChangeState(std::make_shared<EnemyCaution>(GetWeakPtr()));
	ImGui::SameLine();
	if (ImGui::Button("Back"))ChangeState(std::make_shared<EnemyBack>(GetWeakPtr()));
	ImGui::SameLine();
	if (ImGui::Button("Guard"))ChangeState(std::make_shared<EnemyGuard>(GetWeakPtr()));
	if (!canChange)ImGui::EndDisabled();
}

void EnemyBase::DrawDebugWindowAttack()
{
	ImGui::SeparatorText("Attack");

	if (m_attackList.empty())
	{
		ImGui::TextDisabled("No attack data");
		return;
	}
	m_debugSelectedAttack = std::clamp(m_debugSelectedAttack, 0, static_cast<int>(m_attackList.size()) - 1);

	//攻撃の一覧//クリックで選ぶ、ダブルクリックで出す
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("AttackList", kAttackListColumnNum, tableFlags, ImVec2(0.0f, kAttackListHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("#");
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Range");
		ImGui::TableSetupColumn("Pre");
		ImGui::TableSetupColumn("CT");
		ImGui::TableSetupColumn("Next");
		ImGui::TableHeadersRow();

		const bool isAttacking = IsAttackFlowState(m_currentState);
		for (int i = 0; i < static_cast<int>(m_attackList.size()); ++i)
		{
			const EnemyAttackNode& node = m_attackList[i];
			ImGui::TableNextRow();
			ImGui::PushID(i);

			//今出している攻撃は赤くする
			const bool isActive = isAttacking && (i == m_currentAttackIndex);
			if (isActive)ImGui::PushStyleColor(ImGuiCol_Text, kActiveColor);

			ImGui::TableNextColumn();
			const std::string indexText = std::to_string(node.index);
			if (ImGui::Selectable(indexText.c_str(), i == m_debugSelectedAttack, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
			{
				m_debugSelectedAttack = i;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))DebugExecuteAttack(i);
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(imguiApp::ToUtf8(node.name).c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%.0f", node.attackRange);
			ImGui::TableNextColumn();
			ImGui::Text("%.0f", node.preTime);
			ImGui::TableNextColumn();
			ImGui::Text("%.0f", node.coolTime);
			ImGui::TableNextColumn();
			if (node.nextAttack >= 0)ImGui::Text("%d", node.nextAttack);
			else ImGui::TextDisabled("-");

			if (isActive)ImGui::PopStyleColor();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	//選んだ攻撃を出す
	const bool canAttack = !m_isLifeZero && !m_isExecuted;
	if (!canAttack)ImGui::BeginDisabled();
	if (ImGui::Button("Execute"))DebugExecuteAttack(m_debugSelectedAttack);
	if (!canAttack)ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Checkbox("Skip Pre", &m_isDebugSkipPre);
	ImGui::SameLine();
	ImGui::Checkbox("Loop", &m_isDebugLoopAttack);
	if (m_isDebugLoopAttack)
	{
		ImGui::DragFloat("Loop Interval (frames)", &m_debugLoopInterval, kFrameDragSpeed, kLoopIntervalMin, kLoopIntervalMax, "%.0f");
		if (!m_isDebugStopAI)ImGui::TextColored(kWarningColor, "Turn on Stop AI to loop the same attack");
	}

	//選んだ攻撃のデータ//ここで変えた値はQでCSVを読み直すと元に戻る
	EnemyAttackNode& node = m_attackList[m_debugSelectedAttack];
	const std::string detailTitle = std::format("Attack #{} Detail###AttackDetail", node.index);
	if (!ImGui::TreeNode(detailTitle.c_str()))return;

	ImGui::TextDisabled("Edits are lost when Q reloads the CSV");
	ImGui::Text("Pre Anim : %s", node.preAnimName.c_str());
	ImGui::Text("Anim     : %s", node.animName.c_str());

	ImGui::SeparatorText("Pre");
	ImGui::DragFloat("Pre Anim Speed", &node.preAnimTimeScale, kRateDragSpeed);
	ImGui::DragFloat("Pre Anim End Frame", &node.preAnimEndFrame, kFrameDragSpeed);
	ImGui::DragFloat("Pre Blend Frame", &node.preBlendFrame, kFrameDragSpeed);
	ImGui::DragFloat("Pre Time", &node.preTime, kFrameDragSpeed);
	ImGui::DragFloat("Pre Look Time", &node.preLookTime, kFrameDragSpeed);

	ImGui::SeparatorText("Attack");
	ImGui::DragFloat("Anim Speed", &node.animTimeScale, kRateDragSpeed);
	ImGui::DragFloat("Anim End Frame", &node.animEndFrame, kFrameDragSpeed);
	ImGui::DragFloat("Attack Finish Frame", &node.attackFinishFrame, kFrameDragSpeed);
	ImGui::DragFloat("Blend Frame", &node.blendFrame, kFrameDragSpeed);
	ImGui::DragFloat("Col Start Frame", &node.attackColStartFrame, kFrameDragSpeed);
	ImGui::DragFloat("Col End Frame", &node.attackColEndFrame, kFrameDragSpeed);
	ImGui::DragFloat("Move Speed Before Col", &node.moveSpeedBeforeCol, kRateDragSpeed);
	ImGui::DragFloat("Move Speed During Col", &node.moveSpeedDuringCol, kRateDragSpeed);
	ImGui::DragFloat("Attack Range", &node.attackRange, kFrameDragSpeed);

	ImGui::SeparatorText("Hit");
	ImGui::DragFloat("Attack Power", &node.attackPower, kFrameDragSpeed);
	ImGui::DragFloat("Broken Rate (%)", &node.brokenRate, kFrameDragSpeed);
	ImGui::DragFloat("KnockBack XZ", &node.knockBackXZ, kRateDragSpeed);
	ImGui::DragFloat("KnockBack Y", &node.knockBackY, kRateDragSpeed);
	ImGui::Checkbox("Kirimomi", &node.isKirimomi);
	ImGui::DragFloat("HitStop (sec)", &node.hitStopTime, kRateDragSpeed);
	ImGui::DragFloat("Col Radius", &node.colRadius, kFrameDragSpeed);
	ImGui::DragFloat("Col Forward Offset", &node.colForwardOffset, kFrameDragSpeed);
	ImGui::DragFloat("Col Offset Y", &node.colOffsetY, kFrameDragSpeed);

	ImGui::SeparatorText("After");
	ImGui::DragFloat("Attack End Time", &node.attackEndTimeFrame, kFrameDragSpeed);
	ImGui::DragFloat("Cool Time", &node.coolTime, kFrameDragSpeed);
	ImGui::InputInt("Next Attack", &node.nextAttack);
	node.nextAttack = std::clamp(node.nextAttack, -1, static_cast<int>(m_attackList.size()) - 1);

	ImGui::TreePop();
}

void EnemyBase::DrawDebugWindowStatusEdit()
{
	//ここで変えた値はQでCSVを読み直すと元に戻る//HP・当たり判定は生成時にしか使わないので変えても効かない
	if (!ImGui::CollapsingHeader("Status Edit"))return;

	ImGui::TextDisabled("Edits are lost when Q reloads the CSV");
	ImGui::DragFloat("Move Speed", &m_status.moveSpeed, kRateDragSpeed);
	ImGui::DragFloat("Back Speed", &m_status.backSpeed, kRateDragSpeed);
	ImGui::DragFloat("Caution Speed", &m_status.cautionSpeed, kRateDragSpeed);

	ImGui::DragFloat("Guard Rate (%)", &m_status.guardRate, kFrameDragSpeed, 0.0f, 100.0f);
	ImGui::DragFloat("Guard Time", &m_status.guardTime, kFrameDragSpeed);
	ImGui::DragFloat("Parry Gauge", &m_status.parryGauge, kFrameDragSpeed);
	ImGui::DragFloat("Parry Time", &m_status.parryTime, kFrameDragSpeed);
	ImGui::DragFloat("Guard Break Time", &m_status.guardBreakTime, kFrameDragSpeed);
	ImGui::DragInt("Force Hit Count", &m_status.forceHitCount);
	ImGui::DragFloat("Force Hit Reset Time", &m_status.forceHitResetTime, kFrameDragSpeed);
	ImGui::DragFloat("Part Break Rate", &m_status.partBreakRate, kRateDragSpeed);

	ImGui::DragFloat("Back Distance", &m_status.backDistance, kFrameDragSpeed);
	ImGui::DragFloat("Chase Rate (%)", &m_status.chaseRate, kFrameDragSpeed, 0.0f, 100.0f);
	ImGui::DragFloat("Idle Time", &m_status.idleTime, kFrameDragSpeed);
	ImGui::DragFloat("Caution Time", &m_status.cautionTime, kFrameDragSpeed);
	ImGui::DragFloat("Knock Down Time", &m_status.knockDownTime, kFrameDragSpeed);
}

void EnemyBase::DebugExecuteAttack(int index)
{
	if (m_isLifeZero || m_isExecuted)return;
	if (index < 0 || index >= static_cast<int>(m_attackList.size()))return;
	auto player = m_player.lock();
	if (!player)return;

	//攻撃権・クールタイムは見ない(持っていなくても攻撃後のReleaseは何もしないだけ)
	m_currentAttackIndex = index;
	if (m_isDebugSkipPre)
	{
		//予備動作を飛ばすので、AttackPreでしているようにプレイヤーの方を向いて踏み込み先を決める
		m_targetPos = player->GetRigidBody().GetPos();
		ToPlayerLook();
		ChangeState(std::make_shared<EnemyAttack>(GetWeakPtr()));
		return;
	}
	ChangeState(std::make_shared<EnemyAttackPre>(GetWeakPtr()));
}

void EnemyBase::UpdateDebugLoopAttack()
{
	//Idleにいる間だけ数える
	if (!m_isDebugLoopAttack || !std::dynamic_pointer_cast<EnemyIdle>(m_currentState))
	{
		m_debugLoopTimer = 0.0f;
		return;
	}
	m_debugLoopTimer += 1.0f;
	if (m_debugLoopTimer < m_debugLoopInterval)return;

	m_debugLoopTimer = 0.0f;
	DebugExecuteAttack(m_debugSelectedAttack);
}
