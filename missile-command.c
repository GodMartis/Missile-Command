#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include "setup.h"
#include <time.h>
#include <stdlib.h>

int spawned_missiles;
int score = 0;
int missile_count = 0;
int current_wave = 0;
int game_speed = 500000;
int spawn_chance = 20;
int missile_divide_chance = 100;
// ground
char map[HEIGHT][WIDTH];
// missile struct
struct missile
{
    //int speed;
    int xpos;
    int ypos;
    int spawn_x;
    int spawn_y;
    int target_x;
    bool path[HEIGHT][WIDTH];
    bool spawned;
};
struct missile missiles[MAX_MISSILES];

void init_missiles();
void spawn_missile(int y,int x);
void printmap(WINDOW * win);
void printscreen(WINDOW * win);
void checkWindowSize();
void updatemissiles();
bool path_collition(int y,int x,int missile_no);
int get_missile_count();
void destroy_missile(int missile_no);
void delay(int number_of_seconds) ;
int main()
{

    MEVENT event;

	/* Initialize curses */
	initscr();
	clear();
	noecho();
	cbreak();	//Line buffering disabled. pass on everything
    wresize(stdscr, HEIGHT_w, WIDTH_w);
    border( 0, 0, 0, 0, 0, 0, 0, 0); // draws window border
    WINDOW * win = newwin(HEIGHT,WIDTH,1,1); // creates window
    // Enables keypad mode. This makes (at least for me) mouse events getting
    // reported as KEY_MOUSE, instead as of random letters.
    keypad(win, TRUE);
    start_color();
    init_pair(GRASS_PAIR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(WATER_PAIR, COLOR_CYAN, COLOR_BLACK);
    init_pair(MOUNTAIN_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(LAUNCHER_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(EXPLOSION_PAIR, COLOR_RED, COLOR_RED);
    init_pair(HOUSE_PAIR, COLOR_WHITE, COLOR_BLACK);
    curs_set(0);
    refresh();
    srand(time(NULL));   // Initialization, should only be called once.

    init_missiles();
    spawn_missile(0,0);
    wrefresh(win);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    nodelay(win,TRUE);
    clock_t start_time = clock();
    while(1)
    {
        if(clock()-game_speed>start_time)
        {
            start_time = clock();
            checkWindowSize(); // checks if terminal window is not too small
            if(rand()%spawn_chance == 1) 
                spawn_missile(0,0);
            updatemissiles();
            printmap(win);
            wrefresh(win);
        }
        int c = wgetch(win); 
        switch(c) 
        { 
            case KEY_MOUSE: 
                if(getmouse(&event) == OK) 
                { 
                    if(event.bstate & BUTTON1_PRESSED) // This works for left-click 
                    { 
                        mvprintw(0,0,"Mouse Pressed\n");
                        refresh();
                    } 
                } 
            break;
            default:
            break;
        }
        
        //endwin();
    }
    return 0;
}





void printmap(WINDOW *win)
{
    for(int y = 0;y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            char ch = map[y][x];
            if(ch == '#')
            {
                wattron(win,COLOR_PAIR(GRASS_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(GRASS_PAIR));
            }
            else if(ch=='^') 
            {
                wattron(win,COLOR_PAIR(LAUNCHER_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(LAUNCHER_PAIR));
            }
            else if(ch=='@')
            {
                wattron(win,COLOR_PAIR(EXPLOSION_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(EXPLOSION_PAIR));
                map[y][x] = ' ';
            }
            else
            {
                wattron(win,COLOR_PAIR(HOUSE_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }         
        }
    }
}
void checkWindowSize()
{
    int cols,rows;
    getmaxyx(stdscr,rows,cols);
    if(rows<HEIGHT_w || cols < WIDTH_w)
    {
        mvprintw(0,0,"Please expand your window\n");
        while(1)
        {
            refresh();
            getmaxyx(stdscr,rows,cols);
            if(rows>=HEIGHT_w && cols >= WIDTH_w)
            {
                wresize(stdscr, HEIGHT_w, WIDTH_w);
                refresh();
                border( 0, 0, 0, 0, 0, 0, 0, 0); // draws window border
                refresh();
                break;
            }
        }
    }

}

void init_missiles()
{
    for(int i = 0;i< MAX_MISSILES;i++)
    {
        missiles[i].spawned = false;
    }
}
int get_missile_count()
{
    int count;
    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[i].spawned == true) 
            count++;
    }
    return count;
}
void spawn_missile(int y,int x)
{
    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[i].spawned == false) 
            {
                missiles[i].spawned = true;
                if (x==0) missiles[i].spawn_x = rand() % WIDTH;
                else missiles[i].spawn_x = x;
                if (y==0) missiles[i].spawn_y = 0;
                else missiles[i].spawn_y = y;
                missiles[i].target_x = missiles[i].spawn_x + rand()%40 - 20;
                if (missiles[i].target_x> WIDTH)
                    missiles[i].target_x-=80;
                else if (missiles[i].target_x< 0)
                    missiles[i].target_x+=80;
                missiles[i].xpos = missiles[i].spawn_x;
                missiles[i].ypos = missiles[i].spawn_y;
                map[missiles[i].ypos][missiles[i].xpos] = '*';
                for(int y = 0; y<HEIGHT;y++)
                {
                    for (int x=0;x<WIDTH;x++)
                    missiles[i].path[y][x] = false;
                }
                missiles[i].path[missiles[i].ypos][missiles[i].xpos] = true;
                break;
            }
    }
}
void updatemissiles()
{
    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[i].spawned == true) 
        {
            if (missiles[i].xpos < missiles[i].target_x)
                missiles[i].xpos++;
            else if (missiles[i].xpos > missiles[i].target_x)
                missiles[i].xpos--;     
            missiles[i].ypos++;
            if (map[missiles[i].ypos][missiles[i].xpos] == '=' || map[missiles[i].ypos][missiles[i].xpos] == '#' || map[missiles[i].ypos][missiles[i].xpos] == '^' )
            {
                destroy_missile(i);
            }
            else
            {
                missiles[i].path[missiles[i].ypos][missiles[i].xpos] = true;
                map[missiles[i].ypos][missiles[i].xpos] = '*';
                if(rand()%missile_divide_chance == 1) 
                {
                    spawn_missile(missiles[i].ypos,missiles[i].xpos);
                    spawn_missile(missiles[i].ypos,missiles[i].xpos);
                }
            }
        }
    }
}
bool path_collition(int y,int x,int missile_no)
{
    for(int i = 0;i<MAX_MISSILES;i++)
    {
        if(missiles[i].spawned == true && missiles[i].path[y][x] == true && missile_no!=i)
            return true;
    }
    return false;
}
void destroy_missile(int missile_no)
{
    for(int y = 0;y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            if(missiles[missile_no].path[y][x] == true && path_collition(y,x,missile_no) == false)
                map[y][x] = ' ';
        }
    }
    missiles[missile_no].spawned = false;
    map[missiles[missile_no].ypos][missiles[missile_no].xpos] = '@';
}