#pragma once
#include <memory>
#include <list>
#include <utility>


class Input;
class Scene;

/// <summary>
/// シーンを内部に保持し、切り替えを提供する
/// </summary>
class SceneController
{
public:
	SceneController();
private:
	// 現在スタックに積まれているシーン
	// 最後に積んだものだけがUpdateされる
	std::list<std::shared_ptr<Scene>> scenes_;	
public:

	/// <summary>
	/// 強制的なシーンの切り替え(今積まれているシーンをすべて破棄し、新しいシーン1つだけにする)
	/// </summary>
	/// <typeparam name="T">生成したいSceneの派生クラス</typeparam>
	/// <param name="args">Tのコンストラクタに渡す引数(SceneController&を除く)</param>
	/// <note>
	/// 先に新しいシーンを作ってから今のシーンを破棄する、という順番だと、
	/// CollisionManagerなど「シーンのデストラクタで後片付けするグローバルな状態」を
	/// 新しいシーンが登録した直後に古いシーンが消してしまう事故が起きる。
	/// そのため、このテンプレート自身の中で「先に全部破棄→そのあとで生成」の順を保証する。
	/// </note>
	template<typename T, typename... Args>
	void ResetScene(Args&&... args)
	{
		scenes_.clear();//先に今のシーンを全部破棄する(各シーンのデストラクタでの後片付けを先に済ませる)
		scenes_.push_back(std::make_shared<T>(*this, std::forward<Args>(args)...));//そのあとで新しいシーンを作る
	}

	/// <summary>
	/// シーンの切り替え
	/// </summary>
	/// <param name="scene">新しいシーン</param>
	/// <note>最後に積んだシーンがChangeされる</note>
	void ChangeScene(std::shared_ptr<Scene> scene);

	/// <summary>
	/// シーンを新しく積みます
	/// </summary>
	/// <param name="scene">積みたいシーン</param>
	/// <note>ChangeSceneと違って、もともと積まれてるシーンは残り続ける</note>
	void PushScene(std::shared_ptr<Scene> scene);

	/// <summary>
	/// 最後に積んだシーンをリストから外す(自動的に解放される)
	/// </summary>
	void PopScene();

	/// <summary>
	/// 内部に持ってるシーンのUpdateを呼び出す
	/// </summary>
	void Update();

	/// <summary>
	/// 内部に持ってるシーンのDrawを呼び出す
	/// </summary>
	void Draw();
};

