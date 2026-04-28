#ifndef ARENA_H
#define ARENA_H
#include <pthread.h>
#include <time.h>

#define SHM_KEY 0x00001234
#define MAX_PLAYERS 50
#define MAX_HISTORY 20
#define NAME_SIZE 50
#define PASS_SIZE 50

#define BASE_HEALTH 100
#define BASE_DAMAGE 10

#define GOLD_START 150
#define LEVEL_START 1
#define XP_START 0
#define MATCH_TIME 35
typedef struct {
    char opponent[NAME_SIZE];
    char result[10];
    int xp_gain;
    char time_text[20];
} History;
	typedef struct {
    char username[NAME_SIZE];
    char password[PASS_SIZE];
    int used;
    int logged_in;
    int gold;
    int level;
    int xp;
    int weapon_damage;
    History history[MAX_HISTORY];
    int history_count;
} Player;
typedef struct {
    int active;
    int bot;
    int rewarded;
    char p1[NAME_SIZE];
    char p2[NAME_SIZE];
    int hp1;
    int hp2;
    int damage1;
    int damage2;
    int ultimate1;
    int ultimate2;
    time_t last_attack1;
    time_t last_attack2;
    char log[5][100];
    int log_count;
    char winner[NAME_SIZE];
} Battle;

	typedef struct {
    int magic;
    pthread_mutex_t mutex;
    Player players[MAX_PLAYERS];
    char waiting_player[NAME_SIZE];
    Battle battle;
} Arena;
#endif
