#pragma once
#include <windows.h>
#include <vector>
#include "Math/Vector3.h"
#include "Camera/CameraState/CameraStateBase.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

//Camera Animatorで保存する1個分のカメラ情報
//(座標・注視点・次のキーフレームへの遷移設定)
//遷移設定はCameraStateBase::BlendSettingを流用し、普段のカメラ遷移と同じMode/duration(フレーム数)/easingPower/pivotで調整できるようにする
struct CameraKeyframe
{
	Vector3 pos;
	Vector3 target;
	CameraStateBase::BlendSetting blendSetting;//このキーフレームから次のキーフレームへ遷移する際のブレンド設定
};

//ImGuiを使ったデバッグ機能をまとめるクラス
//シングルトンで扱う(System,Inputと同じ方針)→プログラム中に必ず1個だけ存在するようにする書き方
class imguiApp
{
private:
	//コンストラクタ・デストラクタをprivateにして、外から勝手に new imguiApp() 等ができないようにする
	//(シングルトンにするためのお約束。GetInstance()経由でしか使わせない)
	imguiApp() = default;
	~imguiApp() = default;
	//コピー禁止。これがないと imguiApp a = imguiApp::GetInstance(); のようなコピーができてしまい、
	//「唯一の1個」というシングルトンの前提が崩れる
	imguiApp(const imguiApp&) = delete;
	imguiApp& operator=(const imguiApp&) = delete;

public:
	//唯一のインスタンスを取得する関数。プログラムのどこから呼んでも同じ1個の imguiApp が返ってくる
	static imguiApp& GetInstance()
	{
		static imguiApp instance;
		return instance;
	}

	/// <summary>
	/// ImGuiを使えるようにする初期化処理。ゲーム開始時(DxLib_Initの後)に1回だけ呼ぶ。
	/// DxLibがすでに作っているウィンドウ・DirectX11デバイスを"借りて"ImGuiにも使わせる。
	/// </summary>
	/// <param name="hwnd">GetMainWindowHandle()で取得したウィンドウハンドル</param>
	/// <param name="device">GetUseDirect3D11Device()をID3D11Device*にキャストしたもの</param>
	/// <param name="context">GetUseDirect3D11DeviceContext()をID3D11DeviceContext*にキャストしたもの</param>
	void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
	//Init()の後片付け。ゲーム終了時(DxLib_Endの前)に1回だけ呼ぶ
	void Uninit();

	//毎フレームの最初(Update前)に呼ぶ。「今フレーム分のImGuiウィンドウ作りを始めます」という合図
	void NewFrame();
	//ImGuiのウィンドウを組み立て終わった後、ScreenFlipの前に呼ぶ。「組み立てた内容を実際に画面に描いて」という合図
	void Render();

	//デバッグ用ウィンドウ(Camera Debug/Camera Animator)を表示するかどうかを切り替える(F2キーに割り当てる想定)
	//Releaseビルドでもこのフラグをtrueにすればデバッグウィンドウを使える
	void ToggleDebugUIVisible() { m_isDebugUIVisible = !m_isDebugUIVisible; }
	bool IsDebugUIVisible()const { return m_isDebugUIVisible; }

	/// <summary>
	/// カメラ調整用のデバッグウィンドウを組み立てて表示する(この中でImGui::Begin~Endを使っている)
	/// </summary>
	/// <param name="currentPos">現在のカメラ座標(表示・Sync用)</param>
	/// <param name="currentTarget">現在の注視点(表示・Sync用)</param>
	void DrawCameraDebugWindow(const Vector3& currentPos, const Vector3& currentTarget);

	/// <summary>
	/// カメラアニメーター用のデバッグウィンドウを組み立てて表示する(この中でImGui::Begin~Endを使っている)
	/// 現在のカメラをキーフレームとして保存し、保存したキーフレーム間を時間経過でブレンドしながら再生できる
	/// </summary>
	/// <param name="currentPos">現在のカメラ座標(Save Cameraボタンで保存する値)</param>
	/// <param name="currentTarget">現在の注視点(Save Cameraボタンで保存する値)</param>
	void DrawCameraAnimatorWindow(const Vector3& currentPos, const Vector3& currentTarget);

	/// <summary>
	/// Camera Animatorで選択中の1個のキーフレームを、詳細に編集するためのウィンドウ(この中でImGui::Begin~Endを使っている)
	/// Position/Target/Blend Mode/Duration/Easing Power/Slerp Pivotをまとめて編集できる
	/// (Camera Animator側のリストが長くなると窮屈だったので、詳細編集はこちらのウィンドウに分けている)
	/// </summary>
	void DrawCameraKeyframeEditorWindow();

	//カメラ上書きが有効かどうかを返す(ウィンドウ内の"Override Camera"チェックボックスの状態そのもの)
	bool IsCameraOverrideEnabled()const { return m_overrideCamera; }
	//上書き用のカメラ座標を返す(IsCameraOverrideEnabledがtrueのときだけCameraManager側で使われる)
	const Vector3& GetOverrideCameraPos()const { return m_camPos; }
	//上書き用の注視点を返す(同上)
	const Vector3& GetOverrideCameraTarget()const { return m_camTarget; }

	//カメラアニメーターが再生中かどうかを返す("Execute"ボタンを押してから再生が終わるまでtrue)
	bool IsCameraAnimatorPlaying()const { return m_isAnimatorPlaying; }
	//再生中の補間結果のカメラ座標を返す(IsCameraAnimatorPlayingがtrueのときだけCameraManager側で使われる)
	const Vector3& GetAnimatorCameraPos()const { return m_animatorCamPos; }
	//再生中の補間結果の注視点を返す(同上)
	const Vector3& GetAnimatorCameraTarget()const { return m_animatorCamTarget; }

	//DxLibのウィンドウプロシージャに割り込ませるためのハンドラ関数
	//マウス移動・クリック・キー入力などのメッセージが来るたびにWindowsから呼ばれる
	static LRESULT CALLBACK WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	//角度(m_angleHDeg,m_angleVDeg)と距離(m_distance)から、カメラ座標(m_camPos)を計算しなおす
	//「注視点を中心に、その角度・距離ぶん離れた場所」を三角関数で求める処理
	void RecalcPosFromAngleDistance();
	//座標(m_camPos)と注視点(m_camTarget)から、角度・距離を逆算する(↑の計算の逆)
	//「今のカメラは注視点から見て何度・どれくらいの距離にあるか」を求める処理
	void RecalcAngleDistanceFromPos();

	//カメラアニメーターの再生を1フレーム分進める(再生中でなければ何もしない)
	//DrawCameraAnimatorWindow()の中で毎フレーム呼ばれる
	void UpdateCameraAnimatorPlayback();

private:
	bool m_initialized = false;//Init()済みかどうかのフラグ(二重初期化やUninit漏れによる誤動作を防ぐ)
	WNDPROC m_prevWndProc = nullptr;//乗っ取る前の、DxLib本来のウィンドウプロシージャ(呼び出しを引き継ぐために保存)

	//デバッグウィンドウの表示/非表示(F2キーでトグル)。
	//Debugビルドは今まで通りずっと表示された状態から始まり、Releaseビルドは非表示から始まる(F2で初めて使える)
#ifdef _DEBUG
	bool m_isDebugUIVisible = true;
#else
	bool m_isDebugUIVisible = false;
#endif

	bool m_overrideCamera = false;//trueの間、このクラスが持つ値でカメラ位置を上書きする(ImGui上のチェックボックスと連動)

	Vector3 m_camPos = Vector3(0.0f, 300.0f, -700.0f);//カメラ座標(ImGuiで編集する値)
	Vector3 m_camTarget = Vector3(0.0f, 0.0f, 0.0f);//注視点(ImGuiで編集する値)
	float m_angleHDeg = 0.0f;//注視点を中心にした水平方向の角度(度)
	float m_angleVDeg = 0.0f;//注視点を中心にした垂直方向の角度(度)
	float m_distance = 700.0f;//注視点までの距離

	std::vector<CameraKeyframe> m_cameraKeyframes;//Camera Animatorで保存したキーフレームの一覧(再生順)
	int m_selectedKeyframeIndex = -1;//Camera Keyframe Editorで今編集対象になっているキーフレームのindex(-1なら未選択)

	bool m_isAnimatorPlaying = false;//"Execute"を押してから再生が終わるまでtrue
	int m_animatorSegmentIndex = 0;//今補間している区間(m_cameraKeyframes[index]→[index+1])
	float m_animatorSegmentElapsed = 0.0f;//今の区間に入ってからの経過フレーム数(CameraStateBaseのm_blendElapsedと同じ考え方)
	Vector3 m_animatorCamPos;//再生中の補間結果のカメラ座標(GetAnimatorCameraPosで返す値)
	Vector3 m_animatorCamTarget;//再生中の補間結果の注視点(GetAnimatorCameraTargetで返す値)
};
