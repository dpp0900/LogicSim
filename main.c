#ifdef _WIN32 || _WIN64
#include <windows.h>
#else
#include <ncurses.h>
#include <unistd.h>
#include "io.h" // for _getch() and _kbhit() in macOS and Linux (include termios.h, unistd.h, fcntl.h)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONN 0
#define NOT 1
#define AND 2
#define OR 3
#define XOR 4
#define CONTROLMODE 99

#define GATE_IN_X1 0
#define GATE_IN_Y1 1
#define GATE_IN_X2 0
#define GATE_IN_Y2 3
#define GATE_OUT_X 11
#define GATE_OUT_Y 2

#define NOT_IN_X 0
#define NOT_IN_Y 1
#define NOT_OUT_X 5
#define NOT_OUT_Y 1

#define GATE_END_X 11
#define GATE_END_Y 3

typedef struct gate gate;

struct gate {
    int idx;
    int x;
    int y;
    int in_x[2];
    int in_y[2];
    int out_x;
    int out_y;
    int type;
    gate* prev[2][30];
    int prev_count[2];
    gate* next[30];
    int next_count;
    int in_pin_status[2];
    int out_pin_status;
};

gate* gates[30];

int gateCount = 0;
int gateSelected1 = -1;
int gateSelected2 = -1;

int input_count = 0;

char* gateType[5] = { "PIN", "NOT", "AND", "OR", "XOR" };
char* pinType[2] = { "IN", "OUT" };

int x = 5, y = 5;

void initVar() {
    for (int i = 0; i < 30; i++) {
        gates[i] = (gate*)malloc(sizeof(gate));
    }
}

void clearScreen() {
#ifdef _WIN32 || _WIN64
    system("cls");
#else
    system("clear");
#endif
}

void moveCursorTo(int x, int y) {
#ifdef _WIN32 || _WIN64
    COORD coord;
    coord.X = x - 1;
    coord.Y = y - 1;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else
    printf("\033[%d;%df", y, x);
    fflush(stdout);
#endif
}


void drawAndGate(int x, int y, int status) {
    moveCursorTo(x, y);
    printf("   _______ ");
    moveCursorTo(x, y + 1);
    printf("─ |       \\");
    moveCursorTo(x, y + 2);
    printf("  |  AND  │─%d", status);
    moveCursorTo(x, y + 3);
    printf("─ |_______/  ");
}

void drawOrGate(int x, int y, int status) {
    moveCursorTo(x, y);
    printf("  ________");
    moveCursorTo(x, y + 1);
    printf("─ \\       \\");
    moveCursorTo(x, y + 2);
    printf("  |  O R  │─%d", status);
    moveCursorTo(x, y + 3);
    printf("─ /_______/ ");
}

void drawXorGate(int x, int y, int status) {
    moveCursorTo(x, y);
    printf("   _______");
    moveCursorTo(x, y + 1);
    printf("─ \\\\      \\");
    moveCursorTo(x, y + 2);
    printf("  || X O R│─%d", status);
    moveCursorTo(x, y + 3);
    printf("─ //______/ ");
}

void drawNotGate(int x, int y, int status) {
    moveCursorTo(x, y);
    printf("  |\\");
    moveCursorTo(x, y + 1);
    printf("─ | ○─%d", status);
    moveCursorTo(x, y + 2);
    printf("  |/");
}

void drawWireBetween(int x1, int y1, int x2, int y2) {
    int i;
    if (x1 == x2) {
        if (y1 < y2) {
            moveCursorTo(x1 + 1, y1);
            printf("┐");
            for (i = y1 + 1; i < y2; i++) {
                moveCursorTo(x1 + 1, i);
                printf("│");
            }
            moveCursorTo(x1 + 1, y2);
            printf("└");
        }
        else {
            moveCursorTo(x1 + 1, y1);
            printf("┘");
            for (i = y2 + 1; i < y1; i++) {
                moveCursorTo(x1 + 1, i);
                printf("│");
            }
            moveCursorTo(x1 + 1, y2);
            printf("┌");
        }
    }
    else if (y1 == y2) {
        if (x1 < x2) {
            for (i = x1 + 1; i < x2; i++) {
                moveCursorTo(i, y1);
                printf("─");
            }
        }
        else {
            for (i = x2 + 1; i < x1; i++) {
                moveCursorTo(i, y1);
                printf("─");
            }
        }
    }
    else {
        int mid_x = (x1 + x2) / 2;
        for (i = x1 + 1; i < mid_x; i++) {
            moveCursorTo(i, y1);
            printf("─");
        }
        if (y1 < y2) {
            moveCursorTo(mid_x, y1);
            printf("┐");
            for (i = y1 + 1; i < y2; i++) {
                moveCursorTo(mid_x, i);
                printf("│");
            }
            moveCursorTo(mid_x, y2);
            printf("└");
        }
        else {
            moveCursorTo(mid_x, y1);
            printf("┘");
            for (i = y2 + 1; i < y1; i++) {
                moveCursorTo(mid_x, i);
                printf("│");
            }
            moveCursorTo(mid_x, y2);
            printf("┌");
        }
        for (i = mid_x + 1; i < x2; i++) {
            moveCursorTo(i, y2);
            printf("─");
        }
    }
}

void drawWire() {
    int i, j;
    for (i = 0; i < gateCount; i++) {
        for (j = 0; j < gates[i]->prev_count[0]; j++) {
            if (gates[i]->prev[0][j] != NULL) {
                int x2 = gates[i]->in_x[0] - 1;
                int y2 = gates[i]->in_y[0];
                int x1 = gates[i]->prev[0][j]->out_x + 1;
                int y1 = gates[i]->prev[0][j]->out_y;
                drawWireBetween(x1, y1, x2, y2);
            }
        }
        for (j = 0; j < gates[i]->prev_count[1]; j++) {
            if (gates[i]->prev[1][j] != NULL) {
                int x2 = gates[i]->in_x[1] - 1;
                int y2 = gates[i]->in_y[1];
                int x1 = gates[i]->prev[1][j]->out_x + 1;
                int y1 = gates[i]->prev[1][j]->out_y;
                drawWireBetween(x1, y1, x2, y2);
            }
        }
    }
}

void printInit(int mode) {
    printf("Enter %s\t1 : AND\t2 : OR\t3 : XOR\t4 : NOT\t e : select\tMODE : %s\t\t%d, %d\t",(mode == CONTROLMODE ? "0 : CONNECT MODE" : "9 : CONTROL MODE"),(mode != CONTROLMODE ? "CONNECT MODE" : "CONTROL MODE"),x,y);
    int i;
    //printf("Selected 1 : %d\tSelected 2 : %d", gateSelected1, gateSelected2);
    if (gateSelected1 == -1) {
        for (i = 0; i < gateCount; i++) {
            if ((gates[i]->out_x == x || gates[i]->out_x + 1 == x) && gates[i]->out_y == y) {
                gateSelected1 = i;
                printf("Selected1 : %s %s (%d)\t\t", gateType[gates[gateSelected1]->type], pinType[1], gateSelected1);
                break;
            }
            if ((gates[i]->in_x[0] == x || gates[i]->in_x[0] + 1 == x) && gates[i]->in_y[0] == y) {
                gateSelected1 = i;
                printf("Selected1 : %s %s (%d)\t\t", gateType[gates[gateSelected1]->type], pinType[0], gateSelected1);
                break;
            }
            if ((gates[i]->in_x[1] == x || gates[i]->in_x[1] + 1 == x) && gates[i]->in_y[1] == y) {
                gateSelected1 = i;
                printf("Selected1 : %s %s (%d)\t\t", gateType[gates[gateSelected1]->type], pinType[0], gateSelected1);
                break;
            }
        }
        gateSelected1 = -1;
    }
    else {
        printf("Selected1 : %s %s (%d)\t\t", gateType[gates[gateSelected1]->type], pinType[1], gateSelected1);
    }
    if (gateSelected2 == -1) {
        for (i = 0; i < gateCount; i++) {
            if ((gates[i]->out_x == x || gates[i]->out_x + 1 == x) && gates[i]->out_y == y) {
                gateSelected2 = i;
                printf("Selected2 : %s %s (%d)", gateType[gates[gateSelected2]->type], pinType[1], gateSelected2);
                break;
            }
            if ((gates[i]->in_x[0] == x || gates[i]->in_x[0] + 1 == x) && gates[i]->in_y[0] == y) {
                gateSelected2 = i;
                printf("Selected2 : %s %s (%d)", gateType[gates[gateSelected2]->type], pinType[0], gateSelected2);
                break;
            }
            if ((gates[i]->in_x[1] == x || gates[i]->in_x[1] + 1 == x) && gates[i]->in_y[1] == y) {
                gateSelected2 = i;
                printf("Selected2 : %s %s (%d)", gateType[gates[gateSelected2]->type], pinType[0], gateSelected2);
                break;
            }
        }
        gateSelected2 = -1;
    }
    else {
        printf("Selected2 : %s %s (%d)\t\t", gateType[gates[gateSelected2]->type], pinType[0], gateSelected2);
    }

    for (i = 0; i < input_count; i++) {
        printf("\n%d\n", gates[i]->out_pin_status);
    }
    for (i = 0; i < gateCount; i++) {
        if (gates[i]->type == AND)
            drawAndGate(gates[i]->x, gates[i]->y, gates[i]->out_pin_status);
        else if (gates[i]->type == OR)
            drawOrGate(gates[i]->x, gates[i]->y, gates[i]->out_pin_status);
        else if (gates[i]->type == XOR)
            drawXorGate(gates[i]->x, gates[i]->y, gates[i]->out_pin_status);
        else if (gates[i]->type == NOT)
            drawNotGate(gates[i]->x, gates[i]->y, gates[i]->out_pin_status);
    }
    drawWire();
}

void setGate(x, y, type) {
    gates[gateCount]->x = x;
    gates[gateCount]->y = y;
    gates[gateCount]->idx = gateCount;
    if (type == NOT) {
        gates[gateCount]->in_x[0] = x - NOT_IN_X;
        gates[gateCount]->in_y[0] = y + NOT_IN_Y;
        gates[gateCount]->out_x = x + NOT_OUT_X;
        gates[gateCount]->out_y = y + NOT_OUT_Y;
    }
    else {
        gates[gateCount]->in_x[0] = x + GATE_IN_X1;
        gates[gateCount]->in_y[0] = y + GATE_IN_Y1;
        gates[gateCount]->in_x[1] = x + GATE_IN_X2;
        gates[gateCount]->in_y[1] = y + GATE_IN_Y2;
        gates[gateCount]->out_x = x + GATE_OUT_X;
        gates[gateCount]->out_y = y + GATE_OUT_Y;
    }
    gates[gateCount]->type = type;
    memset(gates[gateCount]->prev_count, 0, sizeof(gates[gateCount]->prev_count));
    gates[gateCount]->next_count = 0;
    memset(gates[gateCount]->prev, 0, sizeof(gates[gateCount]->prev));
    memset(gates[gateCount]->next, 0, sizeof(gates[gateCount]->next));
    memset(gates[gateCount]->in_pin_status, 0, sizeof(gates[gateCount]->in_pin_status));
    gates[gateCount]->out_pin_status = 0;
    gateCount += 1;
}

void setPin() {
    int i;
    for (i = 1; i <= input_count; i++) {
        gates[gateCount]->idx = gateCount;
        gates[gateCount]->x = 1;
        gates[gateCount]->y = i * 2;
        gates[gateCount]->out_x = 1;
        gates[gateCount]->out_y = i * 2;
        gates[gateCount]->type = CONN;
        memset(gates[gateCount]->prev_count, 0, sizeof(gates[gateCount]->prev_count));
        gates[gateCount]->next_count = 0;
        memset(gates[gateCount]->prev, 0, sizeof(gates[gateCount]->prev));
        memset(gates[gateCount]->next, 0, sizeof(gates[gateCount]->next));
        memset(gates[gateCount]->in_pin_status, 0, sizeof(gates[gateCount]->in_pin_status));
        gates[gateCount]->out_pin_status = 0;
        gateCount += 1;
    }
}

void connectGate() {
    int i;
    if ((gates[gateSelected2]->in_x[0] == x || gates[gateSelected2]->in_x[0] + 1 == x) && gates[gateSelected2]->in_y[0] == y) {
        gates[gateSelected2]->prev[0][gates[gateSelected2]->prev_count[0]] = gates[gateSelected1];
        gates[gateSelected2]->prev_count[0] += 1;
        gates[gateSelected1]->next[gates[gateSelected1]->next_count] = gates[gateSelected2];
        gates[gateSelected1]->next_count += 1;
    }
    else if ((gates[gateSelected2]->in_x[1] == x || gates[gateSelected2]->in_x[1] + 1 == x) && gates[gateSelected2]->in_y[1] == y) {
        gates[gateSelected2]->prev[1][gates[gateSelected2]->prev_count[1]] = gates[gateSelected1];
        gates[gateSelected2]->prev_count[1] += 1;
        gates[gateSelected1]->next[gates[gateSelected1]->next_count] = gates[gateSelected2];
        gates[gateSelected1]->next_count += 1;
    }
    else {
        return;
    }
    gateSelected1 = -1;
    gateSelected2 = -1;
}

void selectGate() {
    int i;
    if (gateSelected1 == -1 || gateSelected2 != -1) {
        for (i = 0; i < gateCount; i++) {
            if ((gates[i]->out_x == x || gates[i]->out_x + 1 == x) && gates[i]->out_y == y) {
                gateSelected1 = i;
                break;
            }
            if ((gates[i]->in_x[0] == x || gates[i]->in_x[0] + 1 == x) && gates[i]->in_y[0] == y) {
                gateSelected1 = i;
                break;
            }
            if ((gates[i]->in_x[1] == x || gates[i]->in_x[1] + 1 == x) && gates[i]->in_y[1] == y) {
                gateSelected1 = i;
                break;
            }
        }
        gateSelected2 = -1;
    }
    else {
        for (i = 0; i < gateCount; i++) {
            if ((gates[i]->out_x == x || gates[i]->out_x + 1 == x) && gates[i]->out_y == y) {
                gateSelected2 = i;
                break;
            }
            if ((gates[i]->in_x[0] == x || gates[i]->in_x[0] + 1 == x) && gates[i]->in_y[0] == y) {
                gateSelected2 = i;
                break;
            }
            if ((gates[i]->in_x[1] == x || gates[i]->in_x[1] + 1 == x) && gates[i]->in_y[1] == y) {
                gateSelected2 = i;
                break;
            }
        }
        connectGate();

    }
}


void simGates() {
    int i, j, k;
    for (i = 0; i < gateCount; i++) {
        if (gates[i]->type == NOT) {
            gates[i]->out_pin_status = !gates[i]->in_pin_status[0];
        }
        if (gates[i]->type == AND) {
            gates[i]->out_pin_status = gates[i]->in_pin_status[0] && gates[i]->in_pin_status[1];
        }
        if (gates[i]->type == OR) {
            gates[i]->out_pin_status = gates[i]->in_pin_status[0] || gates[i]->in_pin_status[1];

        }
        if (gates[i]->type == XOR) {
            gates[i]->out_pin_status = gates[i]->in_pin_status[0] ^ gates[i]->in_pin_status[1];
        }
        if (gates[i]->next != NULL) {
            gates[i]->in_pin_status[0] = 0;
            gates[i]->in_pin_status[1] = 0;
            for (j = 0; j < gates[i]->next_count; j++) {
                for (k = 0; k < gates[i]->next[j]->prev_count[0]; k++) {
                    if (gates[i]->next[j]->prev[0][k] == gates[i]) {
                        gates[i]->next[j]->in_pin_status[0] = gates[i]->out_pin_status || gates[i]->next[j]->in_pin_status[0];
                    }
                }
                for(k = 0; k < gates[i]->next[j]->prev_count[1]; k++){
                    if (gates[i]->next[j]->prev[1][k] == gates[i]) {
                        gates[i]->next[j]->in_pin_status[1] = gates[i]->out_pin_status || gates[i]->next[j]->in_pin_status[1];
                    }
                }
            }
        }
    }
}

void switchPinOut() {
    gateSelected1 = -1;
    gateSelected2 = -1;
    selectGate();
    if (gates[gateSelected1]->type == CONN) {
        gates[gateSelected1]->out_pin_status = !gates[gateSelected1]->out_pin_status;
    }
    gateSelected1 = -1;
    gateSelected2 = -1;
}


void placeGate(int gateType) {
    while (1) {
        clearScreen();
        simGates();
        printInit(gateType);

        if (gateType == AND)
            drawAndGate(x, y, 0);
        else if (gateType == OR)
            drawOrGate(x, y, 0);
        else if (gateType == XOR)
            drawXorGate(x, y, 0);
        else if (gateType == NOT)
            drawNotGate(x, y, 0);
        else if (gateType == CONN)
            moveCursorTo(x, y);
        else if (gateType == CONTROLMODE)
            moveCursorTo(x, y);
        if (_kbhit()) {
            char input = _getch();
            switch (input) {
            case '1':
                gateType = AND;
                break;
            case '2':
                gateType = OR;
                break;
            case '3':
                gateType = XOR;
                break;
            case '4':
                gateType = NOT;
                break;
            case '0':
                gateType = CONN;
                break;
            case '9':
                gateType = CONTROLMODE;
                break;
            case 'w':
                y--;
                break;
            case 's':
                y++;
                break;
            case 'a':
                x--;
                break;
            case 'd':
                x++;
                break;
            case 'e':
                if (gateType == CONN)
                    selectGate();
                else if (gateType == CONTROLMODE)
                    switchPinOut();
                else
                    setGate(x, y, gateType);
                break;
            case 'q':
                exit(0);
            }
        }
#ifdef _WIN32 || _WIN64
        Sleep(50);
#else
        usleep(50000);
#endif
    }
}



int main()
{
    printf("Pin Number(1~9) : ");
    scanf("%d", &input_count);
    initVar();
    setPin();
    printf("PIN SET\n");
    placeGate(CONN);
    return 0;
}