//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"

// ������ ������ ����  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//����� � ������� ������ 
} sprite;

sprite racket;//������� ������
sprite enemy;//������� ����������
sprite ball;//�����
const int bx = 14;
const int by = 4;
sprite bricks[bx][by];
sprite extra_ball;
sprite heal;
sprite fireball;

struct {
    int score, balls;//���������� ��������� ����� � ���������� "������"
    bool action = false;//��������� - �������� (����� ������ ������ ������) ��� ����
} game;

struct {
    HWND hWnd;//����� ����
    HDC device_context, context;// ��� ��������� ���������� (��� �����������)
    int width, height;//���� �������� ������� ���� ������� ������� ���������
} window;

HBITMAP hBack;// ����� ��� �������� �����������

//c����� ����

void InitGame()
{
    //� ���� ������ ��������� ������� � ������� ������� gdi
    //���� ������������� - ����� ������ ������ ����� � .exe 
    //��������� ������ LoadImageA ��������� � ������� ��������, ��������� �������� ����� ������������� � ������� ���� �������
    ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    enemy.hBitmap = (HBITMAP)LoadImageA(NULL, "racket_enemy.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    extra_ball.hBitmap = (HBITMAP)LoadImageA(NULL, "extra_ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    fireball.hBitmap = (HBITMAP)LoadImageA(NULL, "fireball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    heal.hBitmap = (HBITMAP)LoadImageA(NULL, "heal.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------

    racket.width = window.width;
    racket.height = 50;
    racket.speed = 30;//�������� ����������� �������
    racket.x = window.width / 2.;//������� ���������� ����
    racket.y = window.height - racket.height;//���� ���� ���� ������ - �� ������ �������

    enemy.x = racket.x;//� ���������� �������� ������ � �� �� ����� ��� � ������

    ball.dy = (rand() % 65 + 35) / 100.;//��������� ������ ������ ������
    ball.dx = -(1 - ball.dy);//��������� ������ ������ ������
    float lenght = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
    ball.dy /= lenght;
    ball.dx /= lenght;
    ball.speed = 5;
    ball.rad = 20;
    ball.x = window.width / 2;//x ���������� ������ - �� ������� �������
    ball.y = +150;//����� ����� ������ �������
    //ball.x = racket.x;//x ���������� ������ - �� ������� �������
    //ball.y = racket.y - ball.rad;//����� ����� ������ �������

    game.score = 1;
    game.balls = 3;
    float brick_height = 200;

    for (int x = 0; x < bx; x++) {
        for (int y = 0; y < by; y++) {
            bricks[x][y].width = window.width / bx;
            bricks[x][y].height = brick_height;
            bricks[x][y].x = x * bricks[x][y].width;
            bricks[x][y].y = 375 + y * brick_height;
            bricks[x][y].hBitmap = enemy.hBitmap;
            /*bricks[4][2].x += window.width;
            bricks[5][2].x += window.width;*/
        }
    }
    //extra_ball.x;
    //extra_ball.y;
}

void ProcessSound(const char* name)//������������ ���������� � ������� .wav, ���� ������ ������ � ��� �� ����� ��� � ���������
{
    PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//���������� name �������� ��� �����. ���� ASYNC ��������� ����������� ���� ���������� � ����������� ���������
}

void ShowScore()
{
    //�������� �������� � �������
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//����� ��� ������
    _itoa_s(game.score, txt, 10);//�������������� �������� ���������� � �����. ����� �������� � ���������� txt
    TextOutA(window.context, 10, 10, "Score", 5);
    TextOutA(window.context, 200, 10, (LPCSTR)txt, strlen(txt));

    _itoa_s(game.balls, txt, 10);
    TextOutA(window.context, 10, 100, "Balls", 5);
    TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));
}

void ProcessInput()
{
    if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
    if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;

    if (!game.action && GetAsyncKeyState(VK_SPACE))
    {
        game.action = true;
        ProcessSound("bounce.wav");
    }
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // ������� �������� ������, ����������� � ���������� �����������
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// �������� ����������� bitmap � �������� ������

    if (hOldbm) // ���� �� ���� ������, ���������� ������
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // ���������� ������� �����������

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//��� ������� ������� ����� ����� ��������������� ��� ����������
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // ������ ����������� bitmap
        }

        SelectObject(hMemDC, hOldbm);// ��������������� �������� ������
    }

    DeleteDC(hMemDC); // ������� �������� ������
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//������ ���
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ������� ������

    if (ball.dy < 0 && (enemy.x - racket.width / 4 > ball.x || ball.x > enemy.x + racket.width / 4))
    {
        //��������� ���������� ���������. �� ����� ����, ��������� ������� �� �����������, � �� �� ������� �������� �� ��� ������� �� ������
        //������ �����, �� ������ ������ ������ �� �������, � ������� ���������� ������� - ����������� ��� �����
        //�������� ����� ������ ���� ����� ����� �����, � ������ ���� ����� �� ��� X ������� �� ������� �������� ����� �������
        //� ���� ������, �� ��������� ���������� ������� � ������ � ��������� 9 � 1
        enemy.x = ball.x * .1 + enemy.x * .9;
    }
    for (int x = 0; x < bx; x++) {
        for (int y = 0; y < by; y++) {
            ShowBitmap(window.context, bricks[x][y].x, bricks[x][y].y, bricks[x][y].width, bricks[x][y].height, enemy.hBitmap);

        }
    }
    //ShowBitmap(window.context, enemy.x - racket.width / 2, 0, racket.width, racket.height, enemy.hBitmap);//������� ���������
    ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// �����
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//���� ��������� ������ ���� ������� ������ ����, �������� �� ����
    racket.x = min(racket.x, window.width - racket.width / 2.);//���������� ��� ������� ����
}

void CheckWalls()
{
    if (ball.x < ball.rad || ball.x > window.width - ball.rad)
    {
        ball.dx *= -1;
        ProcessSound("bounce.wav");
    }
}
void CheckBricks()
{
    float lenght_delta = sqrt((ball.dx * ball.dx * ball.speed * ball.speed) + (ball.dy * ball.dy * ball.speed * ball.speed));
    float x = ball.x;
    float y = ball.y;
    float x1, x2, x3, x4, y1, y2, y3, y4;
    float eps_box = 0.1;
    float eps = 3;

    for (int i = 0; i < lenght_delta; i++)
    {
        x += ball.dx / lenght_delta + 1;
        y += ball.dy / lenght_delta + 1;
        for (int x_ = 0; x_ < bx; x_++)
        {
            for (int y_ = 0; y_ < by; y_++)
            {
                x1 = bricks[x_][y_].x;
                x2 = bricks[x_][y_].x + eps_box;
                x3 = x2 + bricks[x_][y_].width - eps_box;
                x4 = x3 + eps_box;
                y1 = bricks[x_][y_].y;
                y2 = bricks[x_][y_].y + eps_box;
                y3 = y2 + bricks[x_][y_].height - eps_box;
                y4 = y3 + eps_box;

                if (x > x1 - ball.rad && x < x4 + ball.rad) //�������, ���
                {
                    if (fabs(y - y4 - ball.rad) < eps)
                    {
                        if (x > x1 - ball.rad && x < x2 + ball.rad)//������ ����� ���� �� �
                        {
                            if (fabs(y - y4 - ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        if (x > x3 - ball.rad && x < x4 + ball.rad)//������ ������ ���� �� X
                        {
                            if (fabs(y - y4 - ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        ball.dy *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
                if (x > x1 - ball.rad && x < x4 + ball.rad)
                {
                    if (fabs(y - y1 + ball.rad) < eps)
                    {
                        if (x > x1 - ball.rad && x < x2 + ball.rad)//������ ����� ���� �� �
                        {
                            if (fabs(y - y1 + ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        if (x > x3 - ball.rad && x < x4 + ball.rad)//������ ������ ���� �� X
                        {
                            if (fabs(y - y1 + ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        ball.dy *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
                if (y > y1 - ball.rad && y < y4 + ball.rad) //�����
                {
                    if (fabs(x - x4 - ball.rad) < eps)
                    {
                        if (y > y1 - ball.rad && y < y2 + ball.rad)
                        {
                            if (fabs(y - x4 - ball.rad) < eps)
                            {
                                ball.dy *= -1;
                            }
                        }
                        ball.dx *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
                if (y > y1 - ball.rad && y < y4 + ball.rad)
                {
                    if (fabs(x - x4 + ball.rad) < eps)
                    {
                        if (y > y1 - ball.rad && y < y2 + ball.rad)
                        {
                            if (fabs(y - x4 + ball.rad) < eps)
                            {
                                ball.dy *= -1;
                            }
                        }
                        ball.dx *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
                if (y > y1 - ball.rad && y < y4 + ball.rad) //�����
                {
                    if (fabs(x - x1 + ball.rad) < eps)
                    {
                        if (y > y1 - ball.rad && y < y2 + ball.rad)
                        {
                            if (fabs(y - x1 + ball.rad) < eps)
                            {
                                ball.dy *= -1;
                            }
                        }
                        ball.dx *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
                if (y > y1 - ball.rad && y < y4 + ball.rad)
                {
                    if (fabs(x - x1 - ball.rad) < eps)
                    {
                        if (y > y1 - ball.rad && y < y2 + ball.rad)
                        {
                            if (fabs(y - x1 - ball.rad) < eps)
                            {
                                ball.dy *= -1;
                            }
                        }
                        ball.dx *= -1;
                        bricks[x_][y_].x += 2 * window.width;
                    }
                }
            }
        }
    }
}
void CheckRoof()
{
    if (ball.y < ball.rad + racket.height)
    {
        ball.dy *= -1;
        ProcessSound("bounce.wav");
    }
}

bool tail = false;

float sign(float x)
{
    if (x < 0) return -1;
    return 1;
}

void CheckFloor()
{
    if (ball.y > window.height - ball.rad - racket.height)//����� ������� ����� ������� - ����������� �������
    {
        if (!tail && ball.x >= racket.x - racket.width / 2. && ball.x <= racket.x + racket.width / 2.)//����� �����, � �� �� � ������ ��������� ������
        {
            //game.score++;//�� ������ ������� ���� ���� ����
            //ball.speed += 5. / game.score;//�� ����������� ��������� - ���������� �������� 
            //racket.width -= 10. / game.score;//������������� ��������� ������ ������� - ��� ���������
            /*float x1min = racket.x - racket.width / 2;
            float x1max = racket.x - racket.width / 4;
            float x3min = racket.x + racket.width / 4;
            float x3max = racket.x + racket.width / 2;
            if ((ball.x > x1min && ball.x < x1max) || (ball.x > x1min && ball.x < x1max)) {
                auto dx = ball.dx;
                auto dy = ball.dy;
                ball.dy *= -0.25;
                if (fabs(ball.dy) < 0.05) {
                    ball.dy = 0.15 * sign(ball.dy);
                }
                float lenght = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
                ball.dy /= lenght;
                ball.dx /= lenght;

                return;
            }*/

            ball.dy *= -1;
            ProcessSound("bounce.wav");//������ ���� �������
        }
        else
        {//����� �� �����

            tail = true;//����� ������ ������ ���� �������

            if (ball.y - ball.rad > window.height)//���� ����� ���� �� ������� ����
            {
                game.balls--;//��������� ���������� "������"

                ProcessSound("fail.wav");//������ ����

                if (game.balls < 0) { //�������� ������� ��������� "������"

                    MessageBoxA(window.hWnd, "game over", "", MB_OK);//������� ��������� � ���������
                    InitGame();//������������������ ����
                }

                ball.dy = (rand() % 65 + 35) / 100.;//������ ����� ��������� ������ ��� ������
                ball.dx = -(1 - ball.dy);
                ball.x = racket.x;//�������������� ���������� ������ - ������ ��� �� �������
                ball.y = racket.y - ball.rad;
                game.action = false;//���������������� ����, ���� ����� �� ������ ������
                tail = false;
            }
        }
    }
}
void CheckRandomBox() {
    //switch (game.score++) {
    //    ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// box
    //}
    ////if (game.score += 1) {
    ////    
    ////    //ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// box
    ////}
    //float box;
}

void ProcessRoom()
{
    //������������ �����, ������� � ���. ������� - ���� ������� ����� ���� ���������, � ������, ��� ������� �� ����� ������ ������������� ����� ������� �������� ������
    CheckWalls();
    CheckBricks();
    CheckRoof();
    CheckFloor();
    CheckRandomBox();
}

void ProcessBall()
{
    if (game.action)
    {
        //���� ���� � �������� ������ - ���������� �����
        ball.x += ball.dx * ball.speed;
        ball.y += ball.dy * ball.speed;
    }
    else
    {
        //����� - ����� "��������" � �������
        ball.x = racket.x;
    }
    if (game.score == 41) {
        MessageBoxA(window.hWnd, "Victory!", "", MB_OK);//������� ��������� � ���������
        InitGame();//������������������ ����
    }
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//�� ������ ���� ������� ����� ��������� ���������� ��� ���������
    window.width = r.right - r.left;//���������� ������� � ���������
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//������ �����
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//����������� ���� � ���������
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    InitWindow();//����� �������������� ��� ��� ����� ��� ��������� � ����
    InitGame();//����� �������������� ���������� ����

    mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        ShowRacketAndBall();//������ ���, ������� � �����
        ShowScore();//������ ���� � �����
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//�������� ����� � ����
        Sleep(16);//���� 16 ���������� (1/���������� ������ � �������)

        ProcessInput();//����� ����������
        LimitRacket();//���������, ����� ������� �� ������� �� �����
        ProcessBall();//���������� �����
        ProcessRoom();//������������ ������� �� ���� � �������, ��������� ������ � ��������
    }

}
