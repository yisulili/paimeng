#include <graphics.h>
#include <vector>
#include <string>

#define	WIND_WIDTH 1280
#define WIND_HRIGHT 720

const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

bool running = true;
bool is_game_started = false;

#pragma comment(lib,"Winmm.lib")
//绘制图片的方法
#pragma comment(lib,"MSIMG32.LIB")
void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

class Atlas
{
public:
	Atlas(LPCTCH path, int num)
	{
		TCHAR path_file[256];
		for (int i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);
			IMAGE* frame = new IMAGE;
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}

	~Atlas()
	{
		for (int i = 0; i < frame_list.size(); i++)
			delete frame_list[i];
	}
public:
	std::vector<IMAGE*> frame_list;
};
Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;


//动画类 (循环播放)
class Animation
{
public:
	Animation(Atlas* atlas,int interval)
	{
		anim_atlas = atlas;
		interval_ms = interval;
	}

	~Animation() = default;

	void Play(int x,int y,int delta)
	{
		timer += delta;
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size(); //确保动画循环
			timer = 0;
		}
		putimage_alpha(x,y, anim_atlas->frame_list[idx_frame]);
	}
private:
	int interval_ms = 0; //帧间隔
	int timer = 0;	//动画计时器
	int idx_frame = 0;	//动画帧索引

private:
	Atlas* anim_atlas;
};
//玩家类
class Player
{
public:
	Player()
	{
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left, 45);//帧间隔为45
		anim_right = new Animation(atlas_player_right, 45);
	}

	~Player()
	{
		delete anim_left;
		delete anim_right;
	}
	//消息处理
	void ProcessEvent(const ExMessage& msg)
	{
		if (msg.message == WM_KEYDOWN)
		{
			switch (msg.vkcode)
			{
			case VK_UP:case 'W':case 'w':
				up = true; break;
			case VK_DOWN:case 'S':case 's':
				down = true; break;
			case VK_RIGHT:case 'D':case 'd':
				right = true; break;
			case VK_LEFT:case 'A':case 'a':
				left = true; break;

			}
		}
		else if (msg.message == WM_KEYUP)
		{
			switch (msg.vkcode)
			{
			case VK_UP:case 'W':case 'w':
				up = false; break;
			case VK_DOWN:case 'S':case 's':
				down = false; break;
			case VK_RIGHT:case 'D':case 'd':
				right = false; break;
			case VK_LEFT:case 'A':case 'a':
				left = false; break;
			}
		}
	}

	//移动
	void Move()
	{
		int dir_x = right - left;
		int dir_y = down - up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);

		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			player_position.x += (int)(SPEED * normalized_x);
			player_position.y += (int)(SPEED * normalized_y);
		}

		if (player_position.x <= 0) player_position.x = 0;
		if (player_position.x >= WIND_WIDTH - PLAYER_WIDTH) player_position.x = WIND_WIDTH - PLAYER_WIDTH;
		if (player_position.y <= 0) player_position.y = 0;
		if (player_position.y >= WIND_HRIGHT - PLAYER_HEIGHT) player_position.y = WIND_HRIGHT - PLAYER_HEIGHT;
	}

	//绘制
	void Draw(int delta)
	{
		putimage_alpha(player_position.x + PLAYER_WIDTH / 2 - SHADOW_WIDTH / 2,
						player_position.y + PLAYER_HEIGHT - 8,&img_shadow);
		static bool is_facing_left;
		int dir = right - left;
		if (dir > 0)
			is_facing_left = false;
		else if(dir < 0)
			is_facing_left = true;

		if (is_facing_left)
			anim_left->Play(player_position.x,player_position.y,delta);
		else
			anim_right->Play(player_position.x, player_position.y, delta);
			
	}
	const POINT& GetPlayerPosition() const
	{
		return player_position;
	}
public:
	const int SPEED = 3;
	const int PLAYER_WIDTH = 80;	//玩家宽度
	const int PLAYER_HEIGHT = 80;	//玩家高度
	const int SHADOW_WIDTH = 32;	//阴影宽度

private:
	Animation* anim_left;
	Animation* anim_right;
	IMAGE img_shadow;
	POINT player_position = {500,500};
	bool up = false;
	bool down = false;
	bool right = false;
	bool left = false;
};
//子弹类
class Bullet
{
public:
	POINT position = { 0,0 };

public:
	Bullet() = default;
	~Bullet() = default;
	void Draw() const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 0));
		fillcircle(position.x, position.y, RADIUS);
	}
private:
	const int RADIUS = 10;
};

//敌人类
class Enemy
{
public:
	Enemy()
	{
		loadimage(&img_shadow,_T("img/shadow_enemy.png"));
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_right, 45);

		//敌人生成边界
		enum class SpawnEdge
		{
			Up = 0,
			Down,
			Left,
			Right
		};
		//将敌人放置在边界外的随机位置
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::Up:
			enemy_position.x = rand() % WIND_WIDTH;
			enemy_position.y = -ENEMY_HEIGHT;
			break;
		case SpawnEdge::Down:
			enemy_position.x = rand() % WIND_WIDTH;
			enemy_position.y = WIND_HRIGHT +ENEMY_HEIGHT;
			break;
		case SpawnEdge::Left:
			enemy_position.x = WIND_WIDTH + ENEMY_WIDTH;
			enemy_position.y = rand() % WIND_HRIGHT;
			break;
		case SpawnEdge::Right:
			enemy_position.x = -ENEMY_WIDTH;
			enemy_position.y = rand() % WIND_HRIGHT;
			break;
		}
	}
	~Enemy()
	{
		delete anim_left;
		delete anim_right;
	}

	void Move(const Player& player)
	{
		int dir_x = player.GetPlayerPosition().x - enemy_position.x;
		int dir_y = player.GetPlayerPosition().y - enemy_position.y;
		double dir_len = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (dir_len != 0)
		{
			double normalized_x = dir_x / dir_len;
			double normalized_y = dir_y / dir_len;
			enemy_position.x += (int)(SPEED * normalized_x);
			enemy_position.y += (int)(SPEED * normalized_y);
		}
		if (dir_x < 0)
			is_facing_left = true;
		else if (dir_x > 0)
			is_facing_left = false;
	}

	void Draw(int delta)
	{
		int pos_shadow_x = enemy_position.x + (ENEMY_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = enemy_position.y + ENEMY_HEIGHT - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		if (is_facing_left)
			anim_left->Play(enemy_position.x, enemy_position.y, delta);
		else
			anim_right->Play(enemy_position.x, enemy_position.y, delta);
	}

	//与子弹碰撞时
	bool CheckBulletCollision(const Bullet& bullet)
	{
		//将子弹视为一个点，只有点在敌人体型内才发生碰撞
		bool is_overlab_x = bullet.position.x >= enemy_position.x && bullet.position.x <= enemy_position.x + ENEMY_WIDTH;
		bool is_overlab_y = bullet.position.y >= enemy_position.y && bullet.position.y <= enemy_position.y + ENEMY_HEIGHT;
		return is_overlab_x && is_overlab_y;
	}

	//与玩家碰撞时返回true
	bool CheckPlayerCollision(const Player& player)
	{
		//将敌人中心位置等效为点，判断点是否在玩家矩形内
		POINT check_point = { enemy_position.x + ENEMY_WIDTH / 2,enemy_position.y + ENEMY_HEIGHT / 2 };
		bool check_x = check_point.x > player.GetPlayerPosition().x && check_point.x < player.GetPlayerPosition().x + player.PLAYER_WIDTH;
		bool check_y = check_point.y > player.GetPlayerPosition().y && check_point.y < player.GetPlayerPosition().y + player.PLAYER_HEIGHT;
		return check_x && check_y;
	}
	void Hurt()
	{
		alive = false;
	}

	bool CheckAlive()
	{
		return alive;
	}

private:
	const int SPEED = 2;
	const int ENEMY_WIDTH = 80;
	const int ENEMY_HEIGHT = 80;
	const int SHADOW_WIDTH = 48;
private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT enemy_position;
	bool is_facing_left = false;
	bool alive = true;
};

//按钮类
class Button
{
public:
	Button(RECT rect,LPCTSTR path_img_idle,LPCTSTR path_img_hovered,LPCTSTR path_img_pushed)
	{
		region = rect;

		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	}

	~Button() = default;

	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle); break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered); break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed); break;
		}
	}

	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))//鼠标移动到按钮变成hovered
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y)) //鼠标离开时变回idle
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}

protected:
	virtual void OnClick() = 0; //纯虚函数:继承必须重写才能被实例化

private:
	enum class Status
	{
		Idle = 0,
		Hovered,
		Pushed
	};

private:
	RECT region; //地域，地区
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;

private:
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}
};

//开始游戏按按钮
class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle,path_img_hovered, path_img_pushed) {}
	~StartGameButton() = default;
protected:
	void OnClick()
	{
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

//退出游戏按钮
class QuitGameButton : public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;
	protected:
		void OnClick()
		{
			running = false;
		}
};

//生成新的敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) //当定时器达到指定间隔数便添加新的敌人
	{
		enemy_list.push_back(new Enemy());
	}
}

//更新子弹位置
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
	const double RADIAL_SPEED = 0.0045;	//径向波动速度(波动速度)
	const double TANGENT_SPEED = 0.0055;  //切向波动速度
	double radian_interval = 2 * 3.14159 / bullet_list.size(); //弧度间隔
	POINT player_position = player.GetPlayerPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);
	for (size_t i = 0; i < bullet_list.size(); i++)
	{
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;
		bullet_list[i].position.x = player_position.x + player.PLAYER_WIDTH / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.PLAYER_HEIGHT / 2 + (int)(radius * cos(radian));

	}
}

//绘制玩家得分
void DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分:%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}


int main()
{
	initgraph(WIND_WIDTH,WIND_HRIGHT);

	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 6);

	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	int score = 0;
	Player player;
	ExMessage msg;
	IMAGE img_menu;
	IMAGE img_background;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullet> bullet_list(3);

	RECT region_btn_start_game, region_btn_quit_game;

	region_btn_start_game.left = (WIND_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	region_btn_quit_game.left = (WIND_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"),_T("img/ui_start_hovered.png"),_T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_background, _T("img/background.png"));

	BeginBatchDraw();

	while (running)
	{
		DWORD start_time = GetTickCount();
		while (peekmessage(&msg))
		{
			if (is_game_started)
				player.ProcessEvent(msg);
			else
			{
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}
		if (is_game_started)
		{
			TryGenerateEnemy(enemy_list); //尝试生成新的敌人
			player.Move(); //移动玩家
			UpdateBullets(bullet_list, player); //更新子弹位置
			for (Enemy* enemy : enemy_list) //移动敌人
			{
				enemy->Move(player);
			}

			//检测敌人和玩家碰撞
			for (Enemy* enemy : enemy_list)
			{
				if (enemy->CheckPlayerCollision(player))
				{
					MessageBox(GetHWnd(), _T("扣1观看视频复活"), _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}

			//检测子弹和敌人的碰撞
			for (Enemy* enemy : enemy_list)
			{
				for (const Bullet& bullet : bullet_list)
				{
					if (enemy->CheckBulletCollision(bullet))
					{
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->Hurt();
						score++;
					}
				}
			}

			//移除生命值归零的敌人
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive())
				{
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
				}
			}
		} // if(is_game_started)

		cleardevice();

		if(is_game_started)
		{
			putimage(0, 0, &img_background);
			player.Draw(1000 / 144);
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);
			for (const Bullet& bullet : bullet_list)
				bullet.Draw();
			DrawPlayerScore(score);
		}
		else
		{
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}

		FlushBatchDraw();

		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 144) //确保每秒运行144帧？
		{
			Sleep(1000 / 144 - delta_time);
		}

	} // while(running)

	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;


	EndBatchDraw();
} //main()


