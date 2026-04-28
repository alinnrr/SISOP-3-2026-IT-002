#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include "arena.h"

void init_arena(Arena *arena) {
    	pthread_mutexattr_t attr;

    memset(arena, 0, sizeof(Arena));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&arena->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    arena->magic = 777;
}
	int main() {
    int shmid;
    Arena *arena;
    char command[50];
    shmid = shmget(SHM_KEY, sizeof(Arena), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }
    arena = (Arena *)shmat(shmid, NULL, 0);
    	if (arena == (void *)-1) {
        perror("shmat");
        return 1;
    }

    if (arena->magic != 777) {
        init_arena(arena);
    }
    printf("Orion is ready.\n");
    printf("SHM_KEY: 0x00001234\n");
    printf("Commands: users, battle, reset, exit\n");
    	while (1) {
        printf("orion> ");
        fflush(stdout);
        	if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "users") == 0) {
            pthread_mutex_lock(&arena->mutex);
            printf("\n=== USERS ===\n");
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (arena->players[i].used) {
                    printf("%s | Gold: %d | Level: %d | XP: %d | Login: %d | Weapon: +%d\n",
                           arena->players[i].username,
                           arena->players[i].gold,
                           arena->players[i].level,
                           arena->players[i].xp,
                           arena->players[i].logged_in,
                           arena->players[i].weapon_damage);
                }
            }
            pthread_mutex_unlock(&arena->mutex);
        } else if (strcmp(command, "battle") == 0) {
            pthread_mutex_lock(&arena->mutex);
            printf("\n=== BATTLE STATUS ===\n");
            if (arena->battle.active) {
                printf("%s vs %s\n", arena->battle.p1, arena->battle.p2);
                printf("HP: %d vs %d\n", arena->battle.hp1, arena->battle.hp2);
                printf("Damage: %d vs %d\n", arena->battle.damage1, arena->battle.damage2);
            } else {
                printf("No active battle.\n");
            }
            pthread_mutex_unlock(&arena->mutex);
        } else if (strcmp(command, "reset") == 0) {
            pthread_mutex_lock(&arena->mutex);
            init_arena(arena);
            pthread_mutex_unlock(&arena->mutex);

            printf("Arena reset.\n");
        } else {
            printf("Unknown command.\n");
            printf("Commands: users, battle, reset, exit\n");
        }
    }
    shmdt(arena);
    return 0;
}
