#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>  

// main window params +2 because of inner border
#define HEIGHT_w 42
#define WIDTH_w 102

// game window params
#define HEIGHT 40
#define WIDTH 100

// colors
#define GRASS_PAIR     1
#define LAUNCHER_PAIR    4
#define HOUSE_PAIR 5
#define EXPLOSION_PAIR 6
#define BACKGROUND_PAIR 7

// max missiles
#define MAX_MISSILES 10

// wave modifiers
#define MISSILES_WAVE_MODIFIER 1.2 // missiles_per_wave *= MISSILES_WAVE_MODIFIER;
#define GAME_SPEED_WAVE_MODIFIER 1.1// game_speed /= GAME_SPEED_WAVE_MODIFIER; 
#define LAUNCH_CHANCE_WAVE_MODIFIER 1.05// launch_chance /= LAUNCH_CHANCE_WAVE_MODIFIER;
#define MISSILE_DIVIDE_CHANCE_WAVE_MODIFIER 1.05// missile_divide_chance /=missile_divide_chance;
#define FRIENDLY_MISSILES_WAVE_MODIFIER 1.1// friendly_missiles*=FRIENDLY_MISSILES_WAVE_MODIFIER;

// main refresh rate and explosion delay are constant
#define MAIN_REFRESH_RATE 1000000/30 //30fps
#define EXPLOSION_DELAY 1000000

// 0 - enemy missiles, 1 - player missiles
#define MISSILE_ENEMY 0
#define MISSILE_PLAYER 1
#define PLAYER_MISSILE_AMOUNT 12

// initial wave params
#define MISSILES_PER_WAVE 20
#define GAME_SPEED 200000
#define PLAYER_MISSILE_SPEED 10000 // this one stays constant
#define LAUNCH_CHANCE 20
#define MISSILE_DIVIDE_CHANCE 100

// missile struct
struct missile
{
    int xpos;
    int ypos;
    int spawn_x;
    int spawn_y;
    int target_x;
    int target_y;
    bool path[HEIGHT][WIDTH];
    bool launched;
};
// two types of missiles (enemy and player)
struct missile missiles[2][MAX_MISSILES];

// game variables
int score = 0;
int launched_missiles;
int missile_count = 0;
int missiles_per_wave = MISSILES_PER_WAVE;
int game_speed = GAME_SPEED;
int launch_chance = LAUNCH_CHANCE;
int missile_divide_chance = MISSILE_DIVIDE_CHANCE;
int friendly_missiles = PLAYER_MISSILE_AMOUNT;
bool freeze_explosion = true; 
int cities_remaining;
// missile launchers (2 launchers)
bool launcher_exists[2] ={false,false};
// missiles left in each launcher
int missiles_left[2] = {PLAYER_MISSILE_AMOUNT,PLAYER_MISSILE_AMOUNT};
// launcher coordinates
int launcher_y[2];
int launcher_x[2];
// map
char map[HEIGHT][WIDTH];

// game functions
void initmap();
void init_missiles();
void printmap(WINDOW * win);
void get_launcher_positions();
void launch_missile(int y,int x,int type);
void updatemissiles(int type);
bool missiles_in_air();
void checkforhit(int type);
bool path_collition(int y,int x,int missile_no,int type);
void resetpath(int type,int i);
void destroy_missile(int missile_no,int type);
// menu functions
void game_start();
void next_wave();
void game_over();

int main()
{
    srand(time(NULL));   // Initialization for random numbers
    // clocks    
    clock_t start_time = clock();
    clock_t explosion_timer = clock();
    clock_t start_time_player = clock();
    clock_t start_refresh_rate = clock();

	/* Initialize curses */
	initscr(); // initiate ncurses
	noecho(); // dont echo input
	cbreak();	//line buffering disabled. pass on everything
    clear(); // clear the screen
    wresize(stdscr, HEIGHT_w, WIDTH_w);
    WINDOW * win = newwin(HEIGHT,WIDTH,1,1); // creates window

    curs_set(0); // invisible cursor

    // reports input as KEY_MOUSE
    keypad(win, TRUE);
    keypad(stdscr, TRUE);

    // add colors
    start_color();
    init_pair(GRASS_PAIR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(LAUNCHER_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(EXPLOSION_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(HOUSE_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(BACKGROUND_PAIR, COLOR_BLACK, COLOR_WHITE);
    //set bg color for the main screen(the border around the game)
    wbkgd(stdscr, COLOR_PAIR(BACKGROUND_PAIR));

    // game init
    refresh();
    init_missiles();
    initmap();
    get_launcher_positions();
    wtimeout(win,1);
    wrefresh(win);

    // input init
    mouseinterval(1);
    timeout(1);
    MEVENT event;
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    // start game
    game_start();
    launch_missile(0,0,MISSILE_ENEMY); // start with a missile :)
    while(1)
    {
        if(clock()-game_speed>start_time)
        {
            start_time = clock();
            printmap(win);
            if(rand()%launch_chance == 1) 
                launch_missile(0,0,MISSILE_ENEMY);
            updatemissiles(MISSILE_ENEMY);
            wrefresh(win);
        }
        if(clock()-EXPLOSION_DELAY>explosion_timer)
        {
            explosion_timer = clock();
            freeze_explosion=false; // @ (explosion ch) are not cleared from the screen if true
        }
        if(clock()-PLAYER_MISSILE_SPEED>start_time_player)
        {
            printmap(win);
            start_time_player = clock();
            updatemissiles(MISSILE_PLAYER);
            wrefresh(win);
        }
        if(clock()-MAIN_REFRESH_RATE>start_refresh_rate)
        {
            
            start_refresh_rate = clock();
            wresize(stdscr, HEIGHT_w, WIDTH_w);
            wresize(win, HEIGHT, WIDTH);
            refresh();
            int x,y ;            // to store where you are
            getyx(stdscr, y, x); // save current pos
            move(0, 0);          // move to begining of line
            clrtoeol();          // clear line
            mvprintw(0,2,"Score: %d\n",score);
            mvprintw(0,20,"Cities: %d\n",cities_remaining);
            mvprintw(41,5,"Missiles: %d\n",missiles_left[0]);
            mvprintw(41,85,"Missiles: %d\n",missiles_left[1]);
            mvprintw(0,20,"Cities: %d\n",cities_remaining);
            if(cities_remaining==0) 
            {
                game_over();
                return 0;
            }
            if(missile_count>=missiles_per_wave && missiles_in_air() == false)
            {
                next_wave();
            }
            int c = wgetch(win); 
            switch(c) 
            { 
                case KEY_MOUSE: 
                    if(getmouse(&event) == OK) 
                    { 
                        if(event.bstate & BUTTON1_PRESSED)
                        {
                            //mvprintw(0,0,"Mouse Pressed\n");
                            launch_missile(event.y,event.x,MISSILE_PLAYER);
                            break;
                        }
                        
                    } 
                default:
                break;
            }
        }
    }
    return 0;
}
// loads map into the array
void initmap()
{
    for(int i = 0;i<HEIGHT-4;i++)
    {
        for(int a =0;a<WIDTH;a++)
        {
            map[i][a] = ' ';
        }
    }
    FILE *fp;
    fp = fopen("map.txt", "r");
    for(int i = HEIGHT-4;i<HEIGHT;i++)
    {
        for(int a =0;a<WIDTH;a++)
        {
            char ch = getc(fp); 
            if(ch=='\n') ch = getc(fp); 
            map[i][a] = ch;
        }
    }
    fclose(fp);
}
// prints the map
void printmap(WINDOW *win)
{
    launcher_exists[0] = false;
    launcher_exists[1] = false;
    cities_remaining=0;
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
                if (x==launcher_x[0]) launcher_exists[0] = true;
                else if (x==launcher_x[1]) launcher_exists[1] = true;
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
            else if(ch=='*')
            {
                wattron(win,COLOR_PAIR(HOUSE_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }   
            else if(ch=='=')
            {
                cities_remaining++;
                wattron(win,COLOR_PAIR(HOUSE_PAIR));
                mvwaddch(win,y,x,ch);
                wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }   
            else
            {
                wattron(win,COLOR_PAIR(HOUSE_PAIR));
                mvwaddch(win,y,x,' ');
                wattroff(win,COLOR_PAIR(HOUSE_PAIR));
            }         
        }
    }
    if(launcher_exists[0]==false) missiles_left[0]=0;
    else if(launcher_exists[1]==false) missiles_left[1]=0;
}
// sets missile launch status to false
void init_missiles()
{
    for(int i = 0;i< MAX_MISSILES;i++)
    {
        missiles[MISSILE_ENEMY][i].launched = false;
        missiles[MISSILE_PLAYER][i].launched = false;
    }
}
// launch a missile
void launch_missile(int y,int x,int type)
{
    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[type][i].launched == false) 
            {
                resetpath(type,i);
                if(type==MISSILE_ENEMY)
                {
                    if(missile_count==missiles_per_wave) break;
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
                    missile_count++;
                    
                }
                else
                {
                    if(x<=WIDTH/2)
                    {
                        if(missiles_left[0]!=0)
                        {
                            missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[0];
                            missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[0];
                            missiles_left[0]--;
                        }
                        else if(missiles_left[1]!=0)
                        {
                            missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[1];
                            missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[1];
                            missiles_left[1]--;
                        }
                        else break;
                    }
                    else
                    {
                        if(missiles_left[1]!=0)
                        {
                            missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[1];
                            missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[1];
                            missiles_left[1]--;
                        }
                        else if(missiles_left[0]!=0)
                        {
                            missiles[MISSILE_PLAYER][i].spawn_x = launcher_x[0];
                            missiles[MISSILE_PLAYER][i].spawn_y = launcher_y[0];
                            missiles_left[0]--;
                        }
                        else break;
                    }
                    missiles[MISSILE_PLAYER][i].target_x = x;
                    missiles[MISSILE_PLAYER][i].target_y = y;
                }

                missiles[type][i].launched = true;
                missiles[type][i].xpos = missiles[type][i].spawn_x;
                missiles[type][i].ypos = missiles[type][i].spawn_y;
                break;
            }
    }
}
// Missile movement, impact detection
void updatemissiles(int type)
{
    if(type==MISSILE_ENEMY)
    {
        for(int i = 0;i< MAX_MISSILES; i++)
        {
            if(missiles[MISSILE_ENEMY][i].launched == true) 
            {
                
                if (missiles[MISSILE_ENEMY][i].xpos < missiles[MISSILE_ENEMY][i].target_x)
                    missiles[MISSILE_ENEMY][i].xpos++;
                else if (missiles[MISSILE_ENEMY][i].xpos > missiles[MISSILE_ENEMY][i].target_x)
                    missiles[MISSILE_ENEMY][i].xpos--;     
                missiles[MISSILE_ENEMY][i].ypos++;
                
                if (map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '=' || map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '#' || map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '^' )
                    destroy_missile(i,MISSILE_ENEMY);
                else if (missiles[MISSILE_ENEMY][i].ypos >=HEIGHT)
                    destroy_missile(i,MISSILE_ENEMY);
                else if(map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] == '@')
                {
                    destroy_missile(i,MISSILE_ENEMY);
                    score++;
                }
                else
                {
                    if( missiles[MISSILE_ENEMY][i].path[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] ==false)
                    {
                        map[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] = '*';
                        missiles[MISSILE_ENEMY][i].path[missiles[MISSILE_ENEMY][i].ypos][missiles[MISSILE_ENEMY][i].xpos] = true;
                    }
                    if(rand()%missile_divide_chance == 1 && missiles[MISSILE_ENEMY][i].ypos<HEIGHT-5) 
                    {
                        launch_missile(missiles[MISSILE_ENEMY][i].ypos,missiles[MISSILE_ENEMY][i].xpos,MISSILE_ENEMY);
                    }
                }
            }
        }   
    }
    else
    {
        for(int i = 0;i< MAX_MISSILES; i++)
        {
            if(missiles[MISSILE_PLAYER][i].launched == true) 
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
                if((missiles[MISSILE_PLAYER][i].ypos <= missiles[MISSILE_PLAYER][i].target_y && missiles[MISSILE_PLAYER][i].xpos == missiles[MISSILE_PLAYER][i].target_x) || (map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '=' || map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '#' || map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '^' || map[missiles[MISSILE_PLAYER][i].ypos][missiles[MISSILE_PLAYER][i].xpos] == '@'))
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
// checks whether missile trails collide (to prevent deletion of another missile's trail after explosion)
bool path_collition(int y,int x,int missile_no,int type)
{
    for(int i = 0;i<MAX_MISSILES;i++)
    {
        if(missiles[type][i].launched == true && missiles[type][i].path[y][x] == true && missile_no!=i)
            return true;
    }
    return false;
}
// destroys missile, sets destroyed area to @, clears the trail
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
    missiles[type][missile_no].launched = false;
    resetpath(type,missile_no);
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos] = '@';
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos-1] = '@';
    map[missiles[type][missile_no].ypos][missiles[type][missile_no].xpos+1] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos-1] = '@';
    map[missiles[type][missile_no].ypos-1][missiles[type][missile_no].xpos+1] = '@';
}
// gets launcher positions (y is currently unused)
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
// checks whether a missile has hit anything
void checkforhit(int type)
{

    for(int i = 0;i< MAX_MISSILES; i++)
    {
        if(missiles[type][i].launched == true) 
        {
            if (map[missiles[type][i].ypos][missiles[type][i].xpos] == '=' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '#' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '^' || map[missiles[type][i].ypos][missiles[type][i].xpos] == '@' )
            {
                destroy_missile(i,type);
                score++;
            }
        }
    }
}
// resets missile path 
void resetpath(int type,int i)
{
    for(int y = 0; y<HEIGHT;y++)
        {
        for (int x=0;x<WIDTH;x++)
            missiles[type][i].path[y][x] = false;
        }
}
// game start screen
void game_start()
{
    attron(COLOR_PAIR(HOUSE_PAIR));
    MEVENT event;
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    char name_msg[]="Missile Command";  
    char description_msg[]="Defend 6 cities by destroying enemy missiles";
    char cities_msg[]="= = = = = =";  
    char start_game_msg[]="-->START GAME<--";  
    while(1)
    {
        refresh();
        int x,y ;            // to store where you are
        mvprintw(HEIGHT/2,WIDTH/2-strlen(name_msg)/2,"%s",name_msg);
        mvprintw(HEIGHT/2+1,WIDTH/2-strlen(description_msg)/2,"%s",description_msg);
        mvprintw(HEIGHT/2+2,WIDTH/2-strlen(cities_msg)/2,"%s",cities_msg);
        mvprintw(HEIGHT/2+3,WIDTH/2-strlen(start_game_msg)/2,"%s",start_game_msg);
        int c = getch(); 
        if(getmouse(&event) == OK) 
        { 
            if(event.bstate & BUTTON1_PRESSED)
            {
                if(event.y = HEIGHT/2+3 && event.x>WIDTH/2-5 && event.x<WIDTH/2+15)
                {
                    attroff(COLOR_PAIR(HOUSE_PAIR));
                    break;
                }
            }
        }
        usleep(10);
    }
}
// game over screen
void game_over()
{
    attron(COLOR_PAIR(HOUSE_PAIR));
    MEVENT event;
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    char game_over_msg[]="GAME OVER!";  
    char score_msg[]="Score: ";  
    char exit_game_msg[]="-->EXIT GAME<--";  
    while(1)
    {
        refresh();
        mvprintw(HEIGHT/2,WIDTH/2-strlen(game_over_msg)/2,"%s",game_over_msg);
        mvprintw(HEIGHT/2+1,WIDTH/2-strlen(score_msg)/2-2,"%s%d",score_msg,score);
        mvprintw(HEIGHT/2+2,WIDTH/2-strlen(exit_game_msg)/2,"%s",exit_game_msg);
        int c = getch(); 
        if(getmouse(&event) == OK) 
        { 
            if(event.bstate & BUTTON1_PRESSED)
            {
                if(event.y = HEIGHT/2+2 && event.x>WIDTH/2-5 && event.x<WIDTH/2+15)
                {
                    endwin();
                    attroff(COLOR_PAIR(HOUSE_PAIR));
                    break;
                }
            }
        }
        usleep(10);
    }
}
// next wave screen. Counts score, updates params for next wave
void next_wave()
{
    attron(COLOR_PAIR(HOUSE_PAIR));
    int missile_bases = 0;
    char score_msg[]="Score: ";  
    char bonus_points_msg[]="BONUS POINTS:";  
    char missile_bases_msg[]="Missile base bonus: ";  
    char missiles_msg[]="Missile bonus: ";  
    char city_msg[]="City bonus: ";  
    char continue_msg[]="-->CONTINUE<--";
    if(launcher_exists[0]==true) missile_bases+=10;
    if(launcher_exists[1]==true) missile_bases+=10;
    score += missile_bases + missiles_left[0]*2 + missiles_left[1]*2 + cities_remaining*10;
    MEVENT event;
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    while(1)
    {
        refresh();
        mvprintw(HEIGHT/2,WIDTH/2-strlen(score_msg)/2-3,"%s%d",score_msg,score);
        mvprintw(HEIGHT/2+1,WIDTH/2-strlen(bonus_points_msg)/2,"%s",bonus_points_msg);
        mvprintw(HEIGHT/2+2,WIDTH/2-strlen(missile_bases_msg)/2,"%s%d",missile_bases_msg,missile_bases);
        mvprintw(HEIGHT/2+3,WIDTH/2-strlen(missiles_msg)/2,"%s%d",missiles_msg,(missiles_left[0]*2 + missiles_left[1]*2));
        mvprintw(HEIGHT/2+4,WIDTH/2-strlen(city_msg)/2,"%s%d",city_msg,cities_remaining*10);
        mvprintw(HEIGHT/2+5,WIDTH/2-strlen(continue_msg)/2,"%s",continue_msg);
        int c = getch(); 
        if(getmouse(&event) == OK) 
        { 
            if(event.bstate & BUTTON1_PRESSED)
            {
                if(event.y = HEIGHT/2+4 && event.x>WIDTH/2-10 && event.x<WIDTH/2+10)
                {
                    attroff(COLOR_PAIR(HOUSE_PAIR));

                    //reset params
                    initmap();
                    init_missiles();
                    missile_count = 0;
                    clear();
                    //
                    missiles_per_wave *= MISSILES_WAVE_MODIFIER;
                    game_speed /= GAME_SPEED_WAVE_MODIFIER; 
                    launch_chance /= LAUNCH_CHANCE_WAVE_MODIFIER;
                    missile_divide_chance /=MISSILE_DIVIDE_CHANCE_WAVE_MODIFIER;
                    friendly_missiles*=FRIENDLY_MISSILES_WAVE_MODIFIER;
                    missiles_left[0] = friendly_missiles;
                    missiles_left[1] = friendly_missiles;
                    break;
                }
            }
        }
        usleep(10);
    }
}
// true if enemy missiles in the air
bool missiles_in_air()
{
    for(int i = 0;i<MAX_MISSILES;i++)
    {
        if(missiles[MISSILE_ENEMY][i].launched == true)
         return true;
    }
    return false;
}