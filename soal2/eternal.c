#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/select.h>
#include "arena.h"

Arena *arena;

void trim(char *s) {
    s[strcspn(s, "\n")] = '\0';
}
void input_text(const char *label, char *buf, int size) {
    printf("%s", label);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    trim(buf);
}
int find_player(const char *username) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (arena->players[i].used &&
            strcmp(arena->players[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}
void time_now(char *buf) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, 20, "%H:%M", t);
}
void add_history(Player *p, const char *opponent, const char *result, int xp_gain) {
    int idx;

    if (p->history_count < MAX_HISTORY) {
        idx = p->history_count;
        p->history_count++;
    } else {
        for (int i = 1; i < MAX_HISTORY; i++) {
            p->history[i - 1] = p->history[i];
        }

 	       idx = MAX_HISTORY - 1;
    }

    strcpy(p->history[idx].opponent, opponent);
    strcpy(p->history[idx].result, result);
    p->history[idx].xp_gain = xp_gain;
    time_now(p->history[idx].time_text);
}
void update_level(Player *p) {
    p->level = 1 + (p->xp / 100);
}

int total_damage(Player *p) {
    return BASE_DAMAGE + (p->xp / 50) + p->weapon_damage;
}
int total_health(Player *p) {
    return BASE_HEALTH + (p->xp / 10);
}

void register_player() {
    char username[NAME_SIZE];
    char password[PASS_SIZE];

    input_text("Username: ", username, sizeof(username));
    input_text("Password: ", password, sizeof(password));

    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Username and password cannot be empty.\n");
        return;
    }

    pthread_mutex_lock(&arena->mutex);
    if (find_player(username) != -1) {
        printf("Username already exists.\n");
        pthread_mutex_unlock(&arena->mutex);
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!arena->players[i].used) {
            arena->players[i].used = 1;
            arena->players[i].logged_in = 0;
            strcpy(arena->players[i].username, username);
            strcpy(arena->players[i].password, password);
            arena->players[i].gold = GOLD_START;
            arena->players[i].level = LEVEL_START;
            arena->players[i].xp = XP_START;
            arena->players[i].weapon_damage = 0;
            arena->players[i].history_count = 0;

            printf("Account created.\n");

            pthread_mutex_unlock(&arena->mutex);
            return;
        }
    }

    printf("Player storage full.\n");
    pthread_mutex_unlock(&arena->mutex);
}
int login_player() {
    char username[NAME_SIZE];
    char password[PASS_SIZE];
    input_text("Username: ", username, sizeof(username));
    input_text("Password: ", password, sizeof(password));
    pthread_mutex_lock(&arena->mutex);

    int idx = find_player(username);

    if (idx == -1) {
        printf("Account not found.\n");
        pthread_mutex_unlock(&arena->mutex);
        return -1;
    }

    if (strcmp(arena->players[idx].password, password) != 0) {
        printf("Wrong password.\n");
        pthread_mutex_unlock(&arena->mutex);
        return -1;
    }
    if (arena->players[idx].logged_in) {
        printf("Account already logged in.\n");
        pthread_mutex_unlock(&arena->mutex);
        return -1;
    }

    arena->players[idx].logged_in = 1;

    printf("Welcome, %s.\n", username);

    pthread_mutex_unlock(&arena->mutex);
    return idx;
}

void logout_player(int idx) {
    pthread_mutex_lock(&arena->mutex);

    arena->players[idx].logged_in = 0;

    if (strcmp(arena->waiting_player, arena->players[idx].username) == 0) {
        arena->waiting_player[0] = '\0';
    }

    pthread_mutex_unlock(&arena->mutex);
}

void show_profile(int idx) {
    Player *p = &arena->players[idx];
    printf("\n=== PROFILE ===\n");
    printf("Name         : %s\n", p->username);
    printf("Gold         : %d\n", p->gold);
    printf("Level        : %d\n", p->level);
    printf("XP           : %d\n", p->xp);
    printf("Damage       : %d\n", total_damage(p));
    printf("Health       : %d\n", total_health(p));
    printf("Weapon Bonus : +%d\n", p->weapon_damage);
    printf("================\n\n");
}

void armory(int idx) {
    int choice;

    int price[] = {100, 300, 600, 1500, 5000};
    int bonus[] = {5, 15, 30, 60, 150};

    char *weapon_name[] = {
        "Wood Sword",
        "Iron Sword",
        "Steel Axe",
        "Demon Blade",
        "God Slayer"
    };

    while (1) {
        pthread_mutex_lock(&arena->mutex);

        printf("\n=== ARMORY ===\n");
        printf("Gold: %d\n\n", arena->players[idx].gold);

        for (int i = 0; i < 5; i++) {
            printf("%d. %s | %d Gold | +%d Damage\n",
                   i + 1,
                   weapon_name[i],
                   price[i],
                   bonus[i]);
        }

        printf("0. Back\n");

        pthread_mutex_unlock(&arena->mutex);

        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input.\n");
            continue;
        }

        getchar();

        if (choice == 0) {
            return;
        }

        if (choice < 1 || choice > 5) {
            printf("Invalid choice.\n");
            continue;
        }

        pthread_mutex_lock(&arena->mutex);

        Player *p = &arena->players[idx];

        if (p->gold < price[choice - 1]) {
            printf("Not enough gold.\n");
        } else {
            p->gold -= price[choice - 1];

            if (bonus[choice - 1] > p->weapon_damage) {
                p->weapon_damage = bonus[choice - 1];
            }

            printf("Purchased %s.\n", weapon_name[choice - 1]);
        }

        pthread_mutex_unlock(&arena->mutex);
    }
}

void show_history(int idx) {
    pthread_mutex_lock(&arena->mutex);

    Player *p = &arena->players[idx];

    printf("\n=== MATCH HISTORY ===\n");

    if (p->history_count == 0) {
        printf("No history.\n");
    }

    for (int i = 0; i < p->history_count; i++) {
        printf("%s | %-15s | %-5s | +%d XP\n",
               p->history[i].time_text,
               p->history[i].opponent,
               p->history[i].result,
               p->history[i].xp_gain);
    }

    pthread_mutex_unlock(&arena->mutex);
}

void push_battle_log(const char *msg) {
    if (arena->battle.log_count < 5) {
        strcpy(arena->battle.log[arena->battle.log_count], msg);
        arena->battle.log_count++;
    } else {
        for (int i = 1; i < 5; i++) {
            strcpy(arena->battle.log[i - 1], arena->battle.log[i]);
        }

        strcpy(arena->battle.log[4], msg);
    }
}
void create_battle(const char *p1, const char *p2, int bot) {
    int idx1 = find_player(p1);
    int idx2 = find_player(p2);
    memset(&arena->battle, 0, sizeof(Battle));
    arena->battle.active = 1;
    arena->battle.bot = bot;
    arena->battle.rewarded = 0;

    strcpy(arena->battle.p1, p1);
    strcpy(arena->battle.p2, p2);

    arena->battle.hp1 = total_health(&arena->players[idx1]);
    arena->battle.damage1 = total_damage(&arena->players[idx1]);

    if (bot) {
        arena->battle.hp2 = BASE_HEALTH;
        arena->battle.damage2 = BASE_DAMAGE;
    } else {
        arena->battle.hp2 = total_health(&arena->players[idx2]);
        arena->battle.damage2 = total_damage(&arena->players[idx2]);
    }
    arena->battle.ultimate1 = 0;
    arena->battle.ultimate2 = 0;
    arena->battle.last_attack1 = 0;
    arena->battle.last_attack2 = 0;
    arena->battle.log_count = 0;
    arena->battle.winner[0] = '\0';
    push_battle_log("Battle started");
}
void reward_players() {
    if (arena->battle.rewarded) {
        return;
    }
    int p1 = find_player(arena->battle.p1);
    int p2 = find_player(arena->battle.p2);

    if (p1 == -1) {
        return;
    }

    if (arena->battle.hp1 <= 0) {
        strcpy(arena->battle.winner, arena->battle.p2);
    } else if (arena->battle.hp2 <= 0) {
        strcpy(arena->battle.winner, arena->battle.p1);
    } else {
        return;
    }

    if (strcmp(arena->battle.winner, arena->battle.p1) == 0) {
        arena->players[p1].xp += 50;
        arena->players[p1].gold += 120;
        update_level(&arena->players[p1]);
        add_history(&arena->players[p1], arena->battle.p2, "WIN", 50);
        	if (!arena->battle.bot && p2 != -1) {
            arena->players[p2].xp += 15;
            arena->players[p2].gold += 30;
            update_level(&arena->players[p2]);
            add_history(&arena->players[p2], arena->battle.p1, "LOSS", 15);
        }
    	} else {
        arena->players[p1].xp += 15;
        arena->players[p1].gold += 30;
        update_level(&arena->players[p1]);
        add_history(&arena->players[p1], arena->battle.p2, "LOSS", 15);

        if (!arena->battle.bot && p2 != -1) {
            arena->players[p2].xp += 50;
            arena->players[p2].gold += 120;
            update_level(&arena->players[p2]);
            add_history(&arena->players[p2], arena->battle.p1, "WIN", 50);
        }
    }
    arena->battle.rewarded = 1;
}
int input_available() {
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0;
}
void print_battle_screen(int side) {
    Battle *b = &arena->battle;
    printf("\033[2J\033[H");
    printf("=== ARENA ===\n\n");

    printf("%s | HP: %d | DMG: %d\n", b->p1, b->hp1, b->damage1);
    printf("VS\n");
    printf("%s | HP: %d | DMG: %d\n\n", b->p2, b->hp2, b->damage2);

    printf("Combat Log:\n");

    for (int i = 0; i < b->log_count; i++) {
        printf("> %s\n", b->log[i]);
    }
    printf("\nCommands:\n");
    printf("a = attack\n");
    printf("u = ultimate\n");
    printf("q = leave view\n");

    if (side == 1) {
        printf("\nYou are: %s\n", b->p1);
    } else {
        printf("\nYou are: %s\n", b->p2);
    }

    printf("Input: ");
    fflush(stdout);
}
void bot_action_if_needed(int side) {
    time_t now;
    char msg[100];

    if (!arena->battle.active || !arena->battle.bot || side != 1) {
        return;
    }
    now = time(NULL);

    if (now - arena->battle.last_attack2 >= 2) {
        arena->battle.hp1 -= arena->battle.damage2;
        arena->battle.last_attack2 = now;

        snprintf(msg, sizeof(msg), "%s hit %s for %d damage",
                 arena->battle.p2,
                 arena->battle.p1,
                 arena->battle.damage2);

        push_battle_log(msg);
    }
}
	void battle_loop(int idx) {
    char username[NAME_SIZE];
    char command[20];
    int side;
    time_t now;
    char msg[100];
    strcpy(username, arena->players[idx].username);

    while (1) {
        pthread_mutex_lock(&arena->mutex);

        if (!arena->battle.active) {
            pthread_mutex_unlock(&arena->mutex);
            printf("No active battle.\n");
            return;
        }
        if (strcmp(arena->battle.p1, username) == 0) {
            side = 1;
        } else if (strcmp(arena->battle.p2, username) == 0) {
            side = 2;
        } else {
            pthread_mutex_unlock(&arena->mutex);
            printf("You are not in this battle.\n");
            return;
        }

        print_battle_screen(side);

        if (arena->battle.hp1 <= 0 || arena->battle.hp2 <= 0) {
            reward_players();

            printf("\nBattle ended.\n");

            if (strcmp(arena->battle.winner, username) == 0) {
                printf("VICTORY\n");
            } else {
                printf("DEFEAT\n");
            }
            pthread_mutex_unlock(&arena->mutex);
            printf("Press ENTER to continue...");
            getchar();
            pthread_mutex_lock(&arena->mutex);
            arena->battle.active = 0;
            pthread_mutex_unlock(&arena->mutex);
            return;
        }
        pthread_mutex_unlock(&arena->mutex);

        if (!input_available()) {
            pthread_mutex_lock(&arena->mutex);
            bot_action_if_needed(side);
            pthread_mutex_unlock(&arena->mutex);
            continue;
        }

        fgets(command, sizeof(command), stdin);
        trim(command);

        pthread_mutex_lock(&arena->mutex);

        now = time(NULL);

        if (strcmp(command, "q") == 0) {
            pthread_mutex_unlock(&arena->mutex);
            return;
        }
        if (side == 1) {
            if (strcmp(command, "a") == 0) {
                if (now - arena->battle.last_attack1 >= 1) {
                    arena->battle.hp2 -= arena->battle.damage1;
                    arena->battle.last_attack1 = now;

                    snprintf(msg, sizeof(msg), "%s hit %s for %d damage",
                             arena->battle.p1,
                             arena->battle.p2,
                             arena->battle.damage1);

                    push_battle_log(msg);
                } else {
                    push_battle_log("Attack cooldown");
                }
            } else if (strcmp(command, "u") == 0) {
                if (arena->players[idx].weapon_damage > 0 &&
                    arena->battle.ultimate1 == 0) {
                    arena->battle.hp2 -= arena->battle.damage1 * 3;
                    arena->battle.ultimate1 = 1;
                    snprintf(msg, sizeof(msg), "%s used Ultimate for %d damage",
                             arena->battle.p1,
                             arena->battle.damage1 * 3);
                    push_battle_log(msg);
                } else {
                    push_battle_log("Ultimate unavailable");
                }
            }
        } else {
            if (strcmp(command, "a") == 0) {
                if (now - arena->battle.last_attack2 >= 1) {
                    arena->battle.hp1 -= arena->battle.damage2;
                    arena->battle.last_attack2 = now;
                    snprintf(msg, sizeof(msg), "%s hit %s for %d damage",
                             arena->battle.p2,
                             arena->battle.p1,
                             arena->battle.damage2);

                    push_battle_log(msg);
                } else {
                    push_battle_log("Attack cooldown");
                }
            } else if (strcmp(command, "u") == 0) {
                if (arena->players[idx].weapon_damage > 0 &&
                    arena->battle.ultimate2 == 0) {
                    arena->battle.hp1 -= arena->battle.damage2 * 3;
                    arena->battle.ultimate2 = 1;
                    snprintf(msg, sizeof(msg), "%s used Ultimate for %d damage",
                             arena->battle.p2,
                             arena->battle.damage2 * 3);

                    push_battle_log(msg);
 	               } else {
                    push_battle_log("Ultimate unavailable");
                }
            }
        }

        pthread_mutex_unlock(&arena->mutex);
    }
}
void matchmaking(int idx) {
    char username[NAME_SIZE];
    time_t start;
    strcpy(username, arena->players[idx].username);
    pthread_mutex_lock(&arena->mutex);
    if (arena->battle.active) {
        printf("Battle arena is busy. Try again later.\n");
        pthread_mutex_unlock(&arena->mutex);
        return;
    }
    	if (strlen(arena->waiting_player) == 0) {
        strcpy(arena->waiting_player, username);
        printf("Searching for an opponent... [%d seconds]\n", MATCH_TIME);
        pthread_mutex_unlock(&arena->mutex);
        start = time(NULL);
        while (time(NULL) - start < MATCH_TIME) {
            pthread_mutex_lock(&arena->mutex);

            if (arena->battle.active &&
                (strcmp(arena->battle.p1, username) == 0 ||
                 strcmp(arena->battle.p2, username) == 0)) {
                pthread_mutex_unlock(&arena->mutex);
                battle_loop(idx);
                return;
            }
            pthread_mutex_unlock(&arena->mutex);
            sleep(1);
        }
        pthread_mutex_lock(&arena->mutex);
        if (!arena->battle.active &&
            strcmp(arena->waiting_player, username) == 0) {
            arena->waiting_player[0] = '\0';
            create_battle(username, "Wild Beast", 1);
            printf("No opponent found. Fighting monster bot.\n");
            pthread_mutex_unlock(&arena->mutex);
            battle_loop(idx);
            return;
        }
        pthread_mutex_unlock(&arena->mutex);
    } else {
        if (strcmp(arena->waiting_player, username) == 0) {
            printf("You are already in queue.\n");
            pthread_mutex_unlock(&arena->mutex);
            return;
        }
        create_battle(arena->waiting_player, username, 0);
        arena->waiting_player[0] = '\0';
        printf("Opponent found.\n");
        pthread_mutex_unlock(&arena->mutex);
        battle_loop(idx);
    }
}
void user_menu(int idx) {
    int choice;
    	while (1) {
        pthread_mutex_lock(&arena->mutex);
        show_profile(idx);
        pthread_mutex_unlock(&arena->mutex);
        printf("1. Battle\n");
        printf("2. Armory\n");
        printf("3. History\n");
        printf("4. Logout\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input.\n");
            continue;
        }
        getchar();
        if (choice == 1) {
            matchmaking(idx);
        } else if (choice == 2) {
            armory(idx);
        } else if (choice == 3) {
            show_history(idx);
            printf("Press ENTER to continue...");
            getchar();
        } else if (choice == 4) {
            logout_player(idx);
            return;
        } else {
            printf("Invalid choice.\n");
        }
    }
}
	int main() {
    int shmid;
    int choice;

    shmid = shmget(SHM_KEY, sizeof(Arena), 0666);
    if (shmid < 0) {
        printf("Orion is not running. Start ./orion first.\n");
        return 1;
    }
    arena = (Arena *)shmat(shmid, NULL, 0);
    if (arena == (void *)-1) {
        perror("shmat");
        return 1;
    }

    while (1) {
        printf("\n=== BATTLE OF ETERION ===\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input.\n");
            continue;
        }
        getchar();
     	   if (choice == 1) {
            register_player();
        } else if (choice == 2) {
            int idx = login_player();

            if (idx != -1) {
                user_menu(idx);
            }
        } else if (choice == 3) {
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }
    shmdt(arena);
    return 0;
}
