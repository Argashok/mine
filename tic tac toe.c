#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define ver 4       //длина вертикали
#define hor 3       //длина горизонтали
#define line 3      //длина линии необходимой для победы

struct coor {
    int x;
    int y;
};

void getmem(char **b) {                               //функция выделения разделяемой памяти под поле
    key_t key;
    int shmid, i, j;
    char *p;

    key = ftok("zak666", 'Z');
    if (hor * ver <= 20) {
        shmid = shmget(key, hor * ver, 0666 | IPC_CREAT);
        *b = shmat(shmid, NULL, 0);
    } else {                                           //если нужно выделить больше памяти, чем возможно за раз
        shmid = shmget(key, 20, 0666 | IPC_CREAT);    //выделение памяти несколько раз с присоединением
        *b = shmat(shmid, NULL, 0);
        for (i = 1; i < (hor * ver) / 20; i++) {
            j = shmget(key, 20, 0666 | IPC_CREAT);
            p = shmat(j, *b + i * 20, 0);
        }
        j = shmget(key, (hor * ver) % 20, 0666 | IPC_CREAT);
        p = shmat(j, *b + i * 20, 0);
    }

}


void playerin(int *i) {                           //вход игрока в программу
    key_t key;
    int shmid;
    char *shmadr;

    key = ftok("zak666", 'p');
    shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT);
    shmadr = shmat(shmid, NULL, 0);
    if (*shmadr == 1) *i = 2;
    else {
        *shmadr = 1;
        *i = 1;
    }
}

void playerout(void) {                          //выход игрока из программы
    key_t key;
    int shmid;
    char *shmadr;

    key = ftok("zak666", 'p');
    shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT);
    shmadr = shmat(shmid, NULL, 0);
    *shmadr = 0;
    shmdt(shmadr);
}

void setfield(char f[][hor + 1]) {                                  //обнуление значений поля
    int i, j;

    for (i = 0; i < ver; i++) for (j = 0; j < hor; j++) f[i][j] = ' ';
}

void showfield(char f[][hor + 1]) {                             //ф-я вывода поля на экран
    int i, j;

    printf("  ");
    for (j = 0; j < hor; j++) printf("%d   ", j + 1);
    printf("\n");
    for (i = 0; i < ver; i++) {
        printf("%d ", i + 1);
        for (j = 0; j < hor; j++) {
            printf("%c ", f[i][j]);
            if (j < hor - 1) printf("| ");
        }
        printf("\n");
        if (i < ver - 1) {
            printf(" ");
            for (j = 0; j < 2 * hor - 1; j++) printf("--");
        }
        printf("\n");
    }
}

int wincheck(char f[][hor + 1], char c) {                                       //ф-я проверки победы
    int i, j, k, flg = 1;

    for (i = 0; i < ver; i++)                 //горизонтальная проверка
        for (j = 0; j < hor - line + 1; j++) {
            for (k = 0; k < line; k++)
                if (f[i][j + k] != c) {
                    flg = 0;
                    break;
                }
            if (flg) return 1;
            flg = 1;
        }

    for (j = 0; j < hor; j++)                 //вертикальная проверка
        for (i = 0; i < ver - line + 1; i++) {
            for (k = 0; k < line; k++)
                if (f[i + k][j] != c) {
                    flg = 0;
                    break;
                }
            if (flg) return 1;
            flg = 1;
        }

    for (i = 0; i < ver - line + 1; i++)          //диагональная проверка 1
        for (j = 0; j < hor - line + 1; j++) {
            for (k = 0; k < line; k++)
                if (f[i + k][j + k] != c) {
                    flg = 0;
                    break;
                }
            if (flg) return 1;
            flg = 1;
        }

    for (i = 0; i < ver - line + 1; i++)          //диагональная проверка 2
        for (j = hor - 1; j > line - 2; j--) {
            for (k = 0; k < line; k++)
                if (f[i + k][j - k] != c) {
                    flg = 0;
                    break;
                }
            if (flg) return 1;
            flg = 1;
        }
    return 0;
}


int main(int argc, char **argv) {        //основная программа

    struct coor coor;
    struct sembuf sops1, sops2;
    char field[ver][hor + 1];
    key_t key;
    int semid, playernum, i, j, k;
    char *shmadr;

    fopen("zak666", "w");
    playerin(&playernum);
    getmem(&shmadr);
    key = ftok("zak666", 's');
    semid = semget(key, 1, 0666 | IPC_CREAT);

    sops1.sem_num = 0;
    sops1.sem_flg = 0;
    sops2.sem_num = 0;
    sops2.sem_flg = 0;
    if (playernum == 1) {
        setfield(field);
        for (i = 0; i < ver; i++) strcpy((shmadr + i * hor), field[i]);
        semctl(semid, 0, SETVAL, (int) 0);
    }
    for (i = 0; i < ver; i++) strcpy(field[i], (shmadr + i * hor));
    showfield(field);

    switch (playernum) {
        case 1:                             //алгоритм для первого игрока
            printf("Вы ходите первым\n");
            for (k = 0;; k++) {
                printf("Введите ккординату клетки:");
                vvod1:
                scanf("%d%d", &coor.x, &coor.y);
                if ((coor.x > ver) || (coor.y > hor)) {
                    printf("\nКоординаты не в границах поля, попробуйте снова:");
                    goto vvod1;
                }
                --coor.x;
                --coor.y;
                if (*(shmadr + (coor.x) * hor + coor.y) != ' ') {
                    printf("\nДанное поле занято, попробуйте снова:");
                    goto vvod1;
                }
                *(shmadr + (coor.x) * hor + coor.y) = 'X';
                for (i = 0; i < ver; i++) strcpy(field[i], (shmadr + i * hor));
                showfield(field);
                sops1.sem_op = 2;
                semop(semid, &sops1, 1);
                if (wincheck(field, 'X')) goto Xwin;
                if (k == (hor * ver) / 2 + (hor * ver) % 2 - 1) break;
                printf("Ожидайте хода противника...\n");
                sops1.sem_op = 0;
                semop(semid, &sops1, 1);
                for (i = 0; i < ver; i++) strcpy(field[i], (shmadr + i * hor));
                showfield(field);
                if (wincheck(field, 'O')) goto Xlose;
            }
            printf("\nНичья!\n");
            break;
        Xlose:
            printf("\nПроигрыш :(\n");
            break;
        Xwin:
            printf("\n!!!!Победа!!!!\n");
            break;


        case 2:                         //алгоритм для второго игрока
            printf("Вы ходите вторым\n");
            for (k = 0;; k++) {
                printf("Ожидайте хода противника...\n");
                sops2.sem_op = -1;
                semop(semid, &sops2, 1);
                for (i = 0; i < ver; i++) strcpy(field[i], (shmadr + i * hor));
                showfield(field);
                if (wincheck(field, 'X')) goto Olose;
                if (k == (hor * ver) / 2) break;
                printf("Введите ккординату клетки:");
                vvod2:
                scanf("%d %d", &coor.x, &coor.y);
                if ((coor.x > ver) || (coor.y > hor)) {
                    printf("\nКоординаты не в границах поля, попробуйте снова:");
                    goto vvod2;
                }
                --coor.x;
                --coor.y;
                if (*(shmadr + (coor.x) * hor + coor.y) != ' ') {
                    printf("\nДанное поле занято, попробуйте снова:");
                    goto vvod2;
                }
                *(shmadr + (coor.x) * hor + coor.y) = 'O';
                for (i = 0; i < ver; i++) strcpy(field[i], (shmadr + i * hor));
                showfield(field);
                sops2.sem_op = -1;
                semop(semid, &sops2, 1);
                if (wincheck(field, 'O')) goto Owin;
            }
            printf("\nНичья!\n");
            break;
        Olose:
            printf("\nПроигрыш :(\n");
            break;
        Owin:
            printf("\n!!!!Победа!!!!\n");
            break;

    }

    playerout();
    shmdt(shmadr);
    return (0);
}
