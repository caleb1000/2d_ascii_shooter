#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <curses.h>
#include <vector>
using namespace std;

struct enemy{
    int rate;
    int clock;
    char type;//enemy type
    int pos_x;//enemy x postion
    int pos_y;//enemy y postion
    int d_index;//current move
    char d_list[10];//movement list
    char direction;//current direction goblin is moving
};

struct projectile{
    int rate;//how many cycles before the projectile moves
    int clock;//how many cycles have happened, resets after it equals rate
    int pos_x;
    int pos_y;
    char direction;
    char type;
};

int score = 0;
int static const max_board = 30;//max board screen
int static const visable_board = 30; //the visable board size printed to screen
std::vector<projectile> flying;//vector of currently flying projectiles
std::vector<enemy> enemies;//vector hold currently alive enemies
char lastmov = 'd'; //char holding last move direction of player

pthread_mutex_t cur_matrix_mutex = PTHREAD_MUTEX_INITIALIZER;//mutex to guard writing to cur_matrix aka the one being drawn to the screen
pthread_cond_t cur_updated;//conditional var to tell if cur_matrix has been updated with the new values
pthread_cond_t cur_drawn;//conditional that tells if the cur_matrix has been drawn to the screen
bool updated = false;//if updated false we wait for new matrix to get copied over
bool drawn = true;//if drawn false we wait to update to new current

char new_matrix[max_board][max_board];//new matrix that has yet to be printed to screen
char cur_matrix[max_board][max_board];//current matrix being printed to screen
bool player_dead = false;

void populate_enemies(){

for(int z = 0; z < 6+(score/1000); z++){
    enemy enemy;
    enemy.pos_x = 15+3*z+z;
    enemy.pos_y = 13+2*z-z;
    if(z%2 == 0){
        enemy.rate = 12;
        enemy.clock = 0;
        enemy.type = 'G';
        enemy.d_index = 0;
        enemy.d_list[0] = 'd';
        enemy.d_list[1] = 'd';
        enemy.d_list[2] = 'd';
        enemy.d_list[3] = 'w';
        enemy.d_list[4] = 'd';
        enemy.d_list[5] = 's';
        enemy.d_list[6] = 'd';
        enemy.d_list[7] = 'd';
        enemy.d_list[8] = 's';
    }
    else{
        enemy.rate = 3;
        enemy.clock = 0;
        enemy.type = 'T';
        enemy.d_index = 2;
        enemy.d_list[0] = 's';
        enemy.d_list[1] = 'z';
        enemy.d_list[2] = 'd';
        enemy.d_list[3] = 'z';
        enemy.d_list[4] = 'a';
        enemy.d_list[5] = 'z';
        enemy.d_list[6] = 'd';
        enemy.d_list[7] = 'z';
        enemy.d_list[8] = 's';
    }
    enemy.direction = enemy.d_list[enemy.d_index];
    enemies.push_back(enemy);
}

}

static void * draw(void *arg){

while(1){
    if(player_dead){
               endwin();
               system("clear");
               printf("\nGAME OVER!\n");
               printf("Score: %d \n",score);
               exit(0);
    }
    cout << "\033[2J\033[1;1H";
    printf("Score: %d \n\r",score);
    for(int i =0; i<visable_board; i++){
        printf("-");
    }
    printf("\n\r");
    pthread_mutex_lock(&cur_matrix_mutex);//lock mutex
    if(!updated){
        //give up lock until signaled that cur matrix has been updated
        pthread_cond_wait(&cur_updated,&cur_matrix_mutex);
    }

    for(int x=0;x<visable_board;x++)
    {
        for(int y=0;y<visable_board;y++)
        {
            if(cur_matrix[x][y] == '0'){
                cout << "\033[0;30m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == '.'){
                cout << "\033[0;37m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == 'o'){
                cout << "\033[1;37m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == 'X'){
                cout << "\033[1;33m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == 'G'){
                cout << "\033[1;32m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == 'F'){
                cout << "\033[1;31m" << cur_matrix[x][y] << "\033[0m";
            }
            else if(cur_matrix[x][y] == 'T'){
                cout << "\033[1;35m" << cur_matrix[x][y] << "\033[0m";
            }

            else{
                cout << "\033[0;33m" << cur_matrix[x][y] << "\033[0m";
            }
        }
        printf("|\n\r");
    }
    for(int i =0; i<visable_board; i++){
        printf("-");
    }
        printf("\n\r");
        updated = false;//done updating, maybe we only need one conditional but for now this works
        drawn = true;//now drawing is out of date
        pthread_mutex_unlock(&cur_matrix_mutex);//unlock mutex
        pthread_cond_signal(&cur_drawn);//tell draw thread that it can unblock
        usleep(5000);//honestly this value is almost random, you really need to see how sleeps affect your print times

   }//while
}


int main()
{
    // need to add mutexes to modifying game matrix
    // asigning values, I suppose this is done allready.
    int player_x = 0;
    int player_y = 0;
    char cur ='0';

    cur_drawn = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    cur_updated = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
    //generate blank board for both new and current board
    for(int x=0;x<max_board;x++)
    {
        for(int y=0;y<max_board;y++)
        {
            char temp = '0';
            cur_matrix[x][y]=temp;
            new_matrix[x][y]=temp;
        }
    }
    new_matrix[player_y][player_x] ='X';//draw player

    pthread_t draw_screen;//thread to draw game screen
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);//might want it to be detatched idk
    pthread_create(&draw_screen, &attrs, draw, NULL);

    initscr();//init curses
    noecho();//don't print to stdio keystrokes
    nodelay(stdscr, TRUE);//no delay on getting getch, nonblocking
    curs_set(0);//make cursor invisable

//populate enemy vector
    populate_enemies();


    //player input loop which writes to the new matrix
    while(1){
        if(enemies.empty()){
            score += 1000;
            new_matrix[player_y][player_x]='0';
            player_x = 0;
            player_y = 0;
            new_matrix[player_y][player_x]='X';
            populate_enemies();
        }
        cur = getch();
        int temp_score = 0;
        // Populate enemies, then flying vectors and finally the player before draw to screen
        // If enemy or enemy projectile touching player end game

for(int x =0; x<enemies.size(); x++){
        //if enemy on edge of screen
        if(enemies.at(x).rate != enemies.at(x).clock){
            enemies.at(x).clock++;
            continue;
        }
        else{
            enemies.at(x).clock = 0;
        }
        if(enemies.at(x).pos_x >= max_board-1 || enemies.at(x).pos_x <= 0 || enemies.at(x).pos_y <= 0 || enemies.at(x).pos_y >= max_board-1){
            if(enemies.at(x).d_index >= 9){
                enemies.at(x).d_index = 0;
            }
            enemies.at(x).direction = enemies.at(x).d_list[enemies.at(x).d_index];
            enemies.at(x).d_index++;
            new_matrix[enemies.at(x).pos_y][enemies.at(x).pos_x] = '0';

            if(enemies.at(x).pos_y >= max_board-1){
               enemies.at(x).pos_y = 1;
            }
            else if(enemies.at(x).pos_x >= max_board-1){
               enemies.at(x).pos_x = 1;
            }
            else if(enemies.at(x).pos_y <= 0){
               enemies.at(x).pos_y = max_board-2;
            }
            else if(enemies.at(x).pos_x <= 0){
               enemies.at(x).pos_x = max_board-2;
            }
            new_matrix[enemies.at(x).pos_y][enemies.at(x).pos_x] = enemies.at(x).type;
            continue; //this means we are in an invalid location currently means goblin with be stuck
        }

        new_matrix[enemies.at(x).pos_y][enemies.at(x).pos_x] = '0';

        if(enemies.at(x).direction == 'w'){
            enemies.at(x).pos_x = enemies.at(x).pos_x;
            enemies.at(x).pos_y = enemies.at(x).pos_y-1;
        }
        else if(enemies.at(x).direction == 'a'){
            enemies.at(x).pos_x = enemies.at(x).pos_x-1;
            enemies.at(x).pos_y = enemies.at(x).pos_y;
        }
        else if(enemies.at(x).direction == 's'){
            enemies.at(x).pos_x = enemies.at(x).pos_x;
            enemies.at(x).pos_y = enemies.at(x).pos_y+1;
        }
        else if(enemies.at(x).direction == 'd'){
            enemies.at(x).pos_x = enemies.at(x).pos_x+1;
            enemies.at(x).pos_y = enemies.at(x).pos_y;
        }

        enemies.at(x).d_index++;
        if(enemies.at(x).d_index >= 9){
            enemies.at(x).d_index = 0;
        }
        enemies.at(x).direction = enemies.at(x).d_list[enemies.at(x).d_index];
        if(new_matrix[enemies.at(x).pos_y][enemies.at(x).pos_x] == 'X'){
               //this means fire hit the player
               player_dead = true;
        }

        new_matrix[enemies.at(x).pos_y][enemies.at(x).pos_x] = enemies.at(x).type;

}


for(int x = 0; x<flying.size(); x++){
//write out projectiles to screen, if offscreen remove them from vector

    if(flying.at(x).rate == flying.at(x).clock){
        if(flying.at(x).pos_y != player_y || flying.at(x).pos_x != player_x){
            new_matrix[flying.at(x).pos_y][flying.at(x).pos_x] ='0';//set old position to 0
        }
        if(flying.at(x).direction == 'w'){
            flying.at(x).pos_x = flying.at(x).pos_x;
            flying.at(x).pos_y = flying.at(x).pos_y-1;
        }
        else if(flying.at(x).direction == 'a'){
            flying.at(x).pos_x = flying.at(x).pos_x-1;
            flying.at(x).pos_y = flying.at(x).pos_y;
        }
        else if(flying.at(x).direction == 's'){
            flying.at(x).pos_x = flying.at(x).pos_x;
            flying.at(x).pos_y = flying.at(x).pos_y+1;
        }
        else if(flying.at(x).direction == 'd'){
            flying.at(x).pos_x = flying.at(x).pos_x+1;
            flying.at(x).pos_y = flying.at(x).pos_y;
        }

       if(flying.at(x).pos_x < 0 || flying.at(x).pos_x > max_board-1 || flying.at(x).pos_y < 0 || flying.at(x).pos_y > max_board-1){
           flying.erase(flying.begin() + x);
           //x--;
           continue;
       }

       if(new_matrix[flying.at(x).pos_y][flying.at(x).pos_x] == 'G' && flying.at(x).type != 'F' || new_matrix[flying.at(x).pos_y][flying.at(x).pos_x] == 'T' && flying.at(x).type != 'F'){
               for(int y = 0; y<enemies.size(); y++){
                   if(enemies.at(y).pos_y == flying.at(x).pos_y && enemies.at(y).pos_x == flying.at(x).pos_x){
                       enemies.erase(enemies.begin() + y);
                       temp_score+=100;
                       break;
                   }
               }
               //find and remove goblin or troll
               new_matrix[flying.at(x).pos_y][flying.at(x).pos_x] = '*';
               //file tile with empty space
               flying.erase(flying.begin() + x);
               //x--;
               //remove water from projectile vector
               continue;
       }

       flying.at(x).clock = 0;//reset projectile clock
       new_matrix[flying.at(x).pos_y][flying.at(x).pos_x] = flying.at(x).type;
       }

    else{
        flying.at(x).clock++;
    }
}//end of copypaste



        if(cur == 'w'){
            if(player_y>0){
                new_matrix[player_y][player_x] = '0';
                player_y--;
                lastmov = 'w';
            }
        }

        else if(cur == 'a'){
            if(player_x>0){
                new_matrix[player_y][player_x] ='0';
                player_x--;
                lastmov = 'a';
            }
        }

        else if(cur == 's'){
            if(player_y<max_board-1){
                new_matrix[player_y][player_x] ='0';
                player_y++;
                lastmov = 's';
            }
        }
        else if(cur == 'd'){
            if(player_x<max_board-1){
                new_matrix[player_y][player_x] ='0';
                player_x++;
                lastmov = 'd';
            }
        }

        if( new_matrix[player_y][player_x] == '*'){
            score+=300;//if player collects gold from dead body
        }

        new_matrix[player_y][player_x] ='X';


        if(cur == 'f'){
            projectile fire;
            fire.direction = lastmov;
            fire.pos_x = player_x;
            fire.pos_y = player_y;
            fire.type = '.'; //Shoots a fash bullet
            fire.rate = 4;
            if(score > 4000){
               fire.type = 'O';
               fire.rate = 0;//moves every clock
            }
            else if(score >= 1000){
               fire.type = 'o';
               fire.rate = 2;//moves every 1 clock
            }
            fire.clock =0;
            if(fire.direction == 'w'){
                fire.pos_y--;
            }
            else if(fire.direction == 'a'){
                fire.pos_x--;
            }
            else if(fire.direction == 's'){
                fire.pos_y++;
            }
            else if(fire.direction == 'd'){
                fire.pos_x++;
            }
            if(fire.pos_y < 0 || fire.pos_x < 0 || fire.pos_y > max_board-1 || fire.pos_x > max_board-1){
            continue;
            }
            flying.push_back(fire);//we need a mutext to guard this vector
        }

        pthread_mutex_lock(&cur_matrix_mutex);//lock mutex
        score += temp_score;
	if(!drawn){
            //give up lock until signaled that drawn has completed
            pthread_cond_wait(&cur_drawn,&cur_matrix_mutex);
	}
        memcpy (cur_matrix, new_matrix, max_board*max_board);
        updated = true;//done updating
        drawn = false;//now drawing is out of date
        pthread_mutex_unlock(&cur_matrix_mutex);//unlock mutex
        pthread_cond_signal(&cur_updated);//tell draw thread that it can unblock
//aquire mutex to write to cur_matrix


}//while

    return 0;  // return 0 to the OS.
}
