//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"

// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
} sprite;

sprite racket;//ракетка игрока
sprite enemy;//ракетка противника
sprite ball;//шарик
const int bx = 14;
const int by = 4;
sprite bricks[bx][by];
sprite extra_ball;
sprite heal;
sprite fireball;

struct {
    int score, balls;//количество набранных очков и оставшихся "жизней"
    bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

HBITMAP hBack;// хэндл для фонового изображения

//cекция кода

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
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
    racket.speed = 30;//скорость перемещения ракетки
    racket.x = window.width / 2.;//ракетка посередине окна
    racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки

    enemy.x = racket.x;//х координату оппонета ставим в ту же точку что и игрока

    ball.dy = (rand() % 65 + 35) / 100.;//формируем вектор полета шарика
    ball.dx = -(1 - ball.dy);//формируем вектор полета шарика
    float lenght = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
    ball.dy /= lenght;
    ball.dx /= lenght;
    ball.speed = 5;
    ball.rad = 20;
    ball.x = window.width / 2;//x координата шарика - на середие ракетки
    ball.y = +150;//шарик лежит сверху ракетки
    //ball.x = racket.x;//x координата шарика - на середие ракетки
    //ball.y = racket.y - ball.rad;//шарик лежит сверху ракетки

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

void ProcessSound(const char* name)//проигрывание аудиофайла в формате .wav, файл должен лежать в той же папке где и программа
{
    PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//переменная name содежрит имя файла. флаг ASYNC позволяет проигрывать звук паралельно с исполнением программы
}

void ShowScore()
{
    //поиграем шрифтами и цветами
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//буфер для текста
    _itoa_s(game.score, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
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

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        }

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }

    DeleteDC(hMemDC); // Удаляем контекст памяти
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ракетка игрока

    if (ball.dy < 0 && (enemy.x - racket.width / 4 > ball.x || ball.x > enemy.x + racket.width / 4))
    {
        //имитируем разумность оппонента. на самом деле, компьютер никогда не проигрывает, и мы не считаем попадает ли его ракетка по шарику
        //вместо этого, мы всегда делаем отскок от потолка, а раектку противника двигаем - подставляем под шарик
        //движение будет только если шарик летит вверх, и только если шарик по оси X выходит за пределы половины длины ракетки
        //в этом случае, мы смешиваем координаты ракетки и шарика в пропорции 9 к 1
        enemy.x = ball.x * .1 + enemy.x * .9;
    }
    for (int x = 0; x < bx; x++) {
        for (int y = 0; y < by; y++) {
            ShowBitmap(window.context, bricks[x][y].x, bricks[x][y].y, bricks[x][y].width, bricks[x][y].height, enemy.hBitmap);

        }
    }
    //ShowBitmap(window.context, enemy.x - racket.width / 2, 0, racket.width, racket.height, enemy.hBitmap);//ракетка оппонента
    ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// шарик
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
    racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
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

                if (x > x1 - ball.rad && x < x4 + ball.rad) //потолок, пол
                {
                    if (fabs(y - y4 - ball.rad) < eps)
                    {
                        if (x > x1 - ball.rad && x < x2 + ball.rad)//отскок левый угол по Х
                        {
                            if (fabs(y - y4 - ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        if (x > x3 - ball.rad && x < x4 + ball.rad)//отскок правый угол по X
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
                        if (x > x1 - ball.rad && x < x2 + ball.rad)//отскок левый угол по Х
                        {
                            if (fabs(y - y1 + ball.rad) < eps)
                            {
                                ball.dx *= -1;
                            }
                        }
                        if (x > x3 - ball.rad && x < x4 + ball.rad)//отскок правый угол по X
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
                if (y > y1 - ball.rad && y < y4 + ball.rad) //стены
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
                if (y > y1 - ball.rad && y < y4 + ball.rad) //стены
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
    if (ball.y > window.height - ball.rad - racket.height)//шарик пересек линию отскока - горизонталь ракетки
    {
        if (!tail && ball.x >= racket.x - racket.width / 2. && ball.x <= racket.x + racket.width / 2.)//шарик отбит, и мы не в режиме обработки хвоста
        {
            //game.score++;//за каждое отбитие даем одно очко
            //ball.speed += 5. / game.score;//но увеличиваем сложность - прибавляем скорости 
            //racket.width -= 10. / game.score;//дополнительно уменьшаем ширину ракетки - для сложности
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
            ProcessSound("bounce.wav");//играем звук отскока
        }
        else
        {//шарик не отбит

            tail = true;//дадим шарику упасть ниже ракетки

            if (ball.y - ball.rad > window.height)//если шарик ушел за пределы окна
            {
                game.balls--;//уменьшаем количество "жизней"

                ProcessSound("fail.wav");//играем звук

                if (game.balls < 0) { //проверка условия окончания "жизней"

                    MessageBoxA(window.hWnd, "game over", "", MB_OK);//выводим сообщение о проигрыше
                    InitGame();//переинициализируем игру
                }

                ball.dy = (rand() % 65 + 35) / 100.;//задаем новый случайный вектор для шарика
                ball.dx = -(1 - ball.dy);
                ball.x = racket.x;//инициализируем координаты шарика - ставим его на ракетку
                ball.y = racket.y - ball.rad;
                game.action = false;//приостанавливаем игру, пока игрок не нажмет пробел
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
    //обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
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
        //если игра в активном режиме - перемещаем шарик
        ball.x += ball.dx * ball.speed;
        ball.y += ball.dy * ball.speed;
    }
    else
    {
        //иначе - шарик "приклеен" к ракетке
        ball.x = racket.x;
    }
    if (game.score == 41) {
        MessageBoxA(window.hWnd, "Victory!", "", MB_OK);//выводим сообщение о проигрыше
        InitGame();//переинициализируем игру
    }
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры

    mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        ShowRacketAndBall();//рисуем фон, ракетку и шарик
        ShowScore();//рисуем очик и жизни
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)

        ProcessInput();//опрос клавиатуры
        LimitRacket();//проверяем, чтобы ракетка не убежала за экран
        ProcessBall();//перемещаем шарик
        ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку
    }

}
