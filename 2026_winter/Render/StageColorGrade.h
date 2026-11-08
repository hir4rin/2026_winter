#pragma once

//背景とステージの色を変える演出(血殺中)
class StageColorGrade
{
public:
	enum class Type
	{
		Red,
		Gray,
	};

	//毎フレーム呼ぶ
	void Update(bool isActive, Type type = Type::Red);
	//背景を塗る(3Dを描く前)
	void DrawBackground()const;
	//ステージを描く直前
	void Begin()const;
	//ステージを描いた直後
	void End()const;

private:
	float m_rate = 0.0f;//かかり具合(0～1)
	Type m_type = Type::Red;
};
