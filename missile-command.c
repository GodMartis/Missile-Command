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
int game_speed = 200000;
int player_missile_speed = 10000;
int spawn_chance = 10;
int missile_divide_chance = 100;
int explosion_delay = 1000000;
int main_refresh_rate = 1000000/30;//30fps
bool freeze_explosion = true;
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
    int target_y;
    bool path[HEIGHT][WIDTH];
    bool spawned;
};
// 0 - enemy missiles, 1 - player missiles
#define MISSILE_ENEMY 0
#define MISSILE_PLAYER 1
struct missile missiles[2][MAX_MISSILES];

//Missile launchers (2 launchers)
bool launcher_exists[2] ={false,false};
int launcher_y[2];
int launcher_x[2];
int missiles_left[2];
void get_launcher_positions();

void checkforhit(int type);
void init_missiles();
void spawn_missile(int y,int x,int type);
void printmap(WINDOW * win);
void printscreen(WINDOW * win);
void checkWindowSize(WINDOW *win);
void updatemissiles(int type);
bool path_collition(int y,int x,int missile_no,int type);
void destroy_missile(int missile_no,int type);
void resetpath(int type,int i);
void delay(int number_of_seconds) ;
int main()
{


	/* Initialize curses */
	initscr();
	clear();
	noecho();
	cbreak();	//Line buffering disabled. pass on everything
    wresize(stdscr, HEIGHT_w, WIDTH_w);
    WINDOW * win = newwin(HEIGHT,WIDTH,1,1); // creates window
    // Enables keypad mode. This makes (at least for me) mouse events getting
    // reported as KEY_MOUSE, instead as of random letters.
    keypad(win, TRUE);
    keypad(stdscr, TRUE);
    start_color();
    init_pair(GRASS_PAIR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(WATER_PAIR, COLOR_CYAN, COLOR_BLACK);
    init_pair(MOUNTAIN_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(LAUNCHER_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(EXPLOSION_PAIR, COLOR_RED, COLOR_RED);
    init_pair(HOUSE_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(BACKGROUND_PAIR, COLOR_BLACK, COLOR_WHITE);
    wbkgd(stdscr, COLOR_PAIR(BACKGROUND_PAIR));
    curs_set(0);
    refresh();
    srand(time(NULL));   // Initialization, should only be called once.

    init_missiles();
    get_launcher_positions();
    wrefresh(win);
    //nodelay(win,TRUE);
    //nodelay(stdscr,TRUE);
    mouseinterval(0);
    timeout(0);
    wtimeout(win,0);
    clock_t start_time = clock();
    clock_t explosion_timer = clock();
    clock_t start_time_player = clock();
    clock_t start_refresh_rate = clock();
    MEVENT event;
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    while(1)
    {
        if(clock()-game_speed>start_time)
        {
            start_time = clock();
            checkWindowSize(win); // checks if terminal window is not too small
            printmap(win);
            if(rand()%spawn_chance == 1) 
                spawn_missile(0,0,MISSILE_ENEMY);
            updatemissiles(MISSILE_ENEMY);
            wrefresh(win);
        }
        if(clock()-explosion_delay>explosion_timer)
        {
            explosion_timer = clock();
            freeze_explosion=false;
        }
        if(clock()-player_missile_speed>start_time_player)
        {
            printmap(win);
            start_time_player = clock();
            updatemissiles(MISSILE_PLAYER);
            wrefresh(win);
        }
        if(clock()-main_refresh_rate>start_refresh_rate)
        {
            start_refresh_rate = clock();
            refresh();
            int x,y ;            // to store where you are
            getyx(stdscr, y, x); // save current pos
            move(0, 0);          // move to begining of line
            clrtoeol();          // clear line
            mvprintw(0,2,"Score");
        }
        //napms(1000 / 30);
        //usleep(1);
        //endwin();
        int c = wgetch(win); 
        switch(c) 
        { 
            case KEY_MOUSE: 
                if(getmouse(&event) == OK) 
                { 
                    if(event.bstate & BUTTON1_PRESSED)
                    {
                        //mvprintw(0,0,"Mouse Pressed\n");
                        spawn_missile(event.y,event.x,MISSILE_PLAYER);
                        break;
                    }
                    
                } 
            default:
            break;
        }
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
                if(freeze_explosion ==false) map[y][x] = ' ';
            }
            else if(ch=='*' || ch=='=')
            {
                wattron(win,COLOR_PAIR(HOUSE_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }   
            else if(ch == ' ')
            {
                    wattron(win,COLOR_PAIR(HOUSE_PAIR));
                    mvwaddch(win,y,x,' ');
                    wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }         
        }
    }
}
void checkWindowSize(WINDOW *win)
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
                
                wbkgd(stdscr, COLOR_PAIR(HOUSE_PAIR));
                clear();
                refresh();
                wresize(stdscr, HEIGHT_w, WIDTH_w);
                wresize(win, HEIGHT, WIDTH);
                wbkgd(stdscr, COLOR_PAIR(BACKGROUND_PAIR));
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
        missiles[MISSILE_ENEMY][i].spawned = false;
        missiles[MISSILE_PLAYER][i].spawned = false;
    }
}
void spawn_missile(int y,int x,int type)
{
    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[type][i].spawned == false) 
            {
                resetpath(type,i);
                if(type==MISSILE_ENEMY)
                {
                    //map[y][x] = '*';
                    if (x==0) missiles[MISSILE_ENEMY][i].spawn_x = rand() % WIDTH;
                    else missiles[MISSILE_ENEMY][i].spawn_x = x;
                    if (y==0) missiles[MISSILE_ENEMY][i].spawn_y = 0;
                    else missiles[MISSILE_ENEMY][i].spawn_y = y;
                    missiles[MISSILE_ENEMY][i].target_x = missiles[MISSILE_ENEMY][i].spawn_x + rand()%40 - 20;
                    if (missiles[MISSILE_ENEMY][i].target_x> WIDTH)
                        missiles[MISSILE_ENEMY][i].target_x-=80;
                    else if (missiles[MISSILE_ENEMY][i].target_x< 0)
                        missiles[MISSILE_ENEMY][i].target_x+=80;
                    missiles[MISSILE_ENEMY][i].path[missiles[MISSILE_ENEMY][i].spawn_y][missiles[MISSILE_ENEMY][i].spawn_x] = true;
                    map[missiles[MISSILE_ENEMY][i].spawn_y][missiles[MISSILE_ENEMY][i].spawn_x] = '*';
                    
                }
                else
                {
                    missiles[MISSILE_PLAYER][i].target_x = x;
                    missiles[MISSILE_PLAYER][i].target_y = y;
                    if(missiles[MISSILE_PLAYER][i].target_x<=WIDTH/2)
                    {
                        missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[0];
                        missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[0];
                    }
                    else
                    {
                        missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[1];
                        missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[1];
                    }
                }
                missiles[type][i].spawned = true;
                missiles[type][i].xpos = missiles[type][i].spawn_x;
                missiles[type][i].ypos = missiles[type][i].spawn_y;
                break;
            }
    }
}
void updatemissiles(int type)
{
    if(type==MISSILE_ENEMY)
    {
        for(int i = 0;i< MAX_MISSILES; i++)
        {
            if(missiles[MISSILE_ENEMY][i].spawned == true) 
            {
                
                if (missiles[MISSILE_ENEMY][i].xpos < missiles[MISSILE_ENEMY][i].target_x)
                    missiles[MISSILE_ENEMY][i].xpos++;
                else if (missiles[MISSILE_ENEMY][i].xpos > missiles[MISSILE_ENEMY][i].target_x)
                    missiles[MISSILE_ENEMY][i].xpos--;     
                missiles[MISSILE_ENEMY][i].ypos++;
                
                if (map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '=' || map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '#' || map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '^' || map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '@' )
                    destroy_missile(i,MISSILE_ENEMY);
                else if (map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].target_x] == ' ' && missiles[MISSILE_ENEMY][i].ypos == HEIGHT-1)
                    destroy_missile(i,MISSILE_ENEMY);
                else
                {
                    if( missiles[MISSILE_ENEMY][i].path[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] ==false)
                    {
                        map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] = '*';
                        missiles[MISSILE_ENEMY][i].path[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] = true;
                    }
                    if(rand()%missile_divide_chance == 1 && missiles[MISSILE_ENEMY][i].ypos<HEIGHT-5) 
                    {
                        spawn_missile(missiles[MISSILE_ENEMY][i].ypos,missiles[MISSILE_ENEMY][i].xpos,MISSILE_ENEMY);
                    }
                }
            }
        }   
    }
    else
    {
        for(int i = 0;i< MAX_MISSILES; i++)
        {
            if(missiles[MISSILE_PLAYER][i].spawned == true) 
            {
                if (missiles[MISSILE_PLAYER][i].xpos < missiles[MISSILE_PLAYER][i].target_x)
                    missiles[MISSILE_PLAYER][i].xpos++;
                else if (missiles[MISSILE_PLAYER][i].xpos > missiles[MISSILE_PLAYER][i].target_x)
                    missiles[MISSILE_PLAYER][i].xpos--;     
                if (missiles[MISSILE_PLAYER][i].ypos > missiles[MISSILE_PLAYER][i].target_y)                
                {
                    missiles[MISSILE_PLAYER][i].ypos--;
                }
                missiles[MISSILE_PLAYER][i].path[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] = true;
                if((missiles[MISSILE_PLAYER][i].ypos <= missiles[MISSILE_PLAYER][i].target_y && missiles[MISSILE_PLAYER][i].xpos == missiles[MISSILE_PLAYER][i].target_x) || (map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '=' || map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '#' || map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '^' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '@' ))
                    {
                        destroy_missile(i,type);
                        checkforhit(MISSILE_ENEMY);
                    }
                else
                {
                   map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] = '*';
                }
            }
        }
    }

}

bool path_collition(int y,int x,int missile_no,int type)
{
    for(int i = 0;i<MAX_MISSILES;i++)
    {
        if(missiles[type][i].spawned == true && missiles[type][i].path[y][x] == true && missile_no!=i)
            return true;
    }
    return false;
}
void destroy_missile(int missile_no, int type)
{
    freeze_explosion=true;
    for(int y = 0;y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            if(missiles[type][missile_no].path[y][x] == true && path_collition(y,x,missile_no,type) == false)
                map[y][x] = ' ';
        }
    }
    missiles[type][missile_no].spawned = false;
    resetpath(type,missile_no);
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos] = '@';
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos-1] = '@';
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos+1] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos-1] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos+1] = '@';
}
void get_launcher_positions()
{
    for(int y = 0;y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            if(map[y][x] == '^')
            {
                if(launcher_exists[0] == false)
                {
                    launcher_exists[0] = true;
                    launcher_x[0] = x;
                    launcher_y[0] = y;
                }
                else if(launcher_exists[1] == false)
                {
                    launcher_exists[1] = true;
                    launcher_x[1] = x;
                    launcher_y[1] = y;
                }
            }
        }
    }
}
void checkforhit(int type)
{

    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[type][i].spawned == true) 
        {
            if (map[missiles[type][i].ypos][missiles[type][i].xpos] == '=' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '#' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '^' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '@' )
                destroy_missile(i,type);
            else if (map[missiles[type][i].ypos][missiles[type][i].target_x] == ' ' && missiles[type][i].ypos == HEIGHT-1)
                destroy_missile(i,type);
        }
    }
}
void resetpath(int type,int i)
{
    for(int y = 0; y<HEIGHT;y++)
        {
        for (int x=0;x<WIDTH;x++)
            missiles[type][i].path[y][x] = false;
        }
}
