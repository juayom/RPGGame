#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <windows.h> // Sleep ��� �� PlaySound ���
#include <mmsystem.h> // PlaySound ���
#pragma comment(lib, "winmm.lib") // ��ũ�� ���̺귯�� ����

#define BASE_WIDTH 20
#define BASE_HEIGHT 10
#define MONSTER_COUNT_BASE 3
#define INVENTORY_SIZE 10
#define MAX_PLAYERS 10

int currentLevel = 1; // ���� ���� ����
const int maxLevel = 3; // �ִ� ����

void mainMenu();
//������ ���� ����
typedef enum {
    WEAPON,  // ���ݷ� ��ȭ (��, ���ݷ� + 10)
    HELMET,  // ���ݷ� ��� (����, ���ݷ� + 5)
    SHOES,   // ���ǵ� (���� ���ǵ� �÷���, ���ǵ� + 10)
    CLOTHES, // ��Ʒ� ��ȭ (����, ���� + 10)
    SHIELD,  //��Ʒ� ��ȭ (����, ���� + 10)
    JEWEL,  // ü�� ȸ�� (���� ����, HP Ǯ������)
    POTION  // ü�� ȸ�� ü�� ����, HP + 20)
}ItemType;

// ������ ����ü ����
typedef struct {
    ItemType type;
    char name[50];  // ������ �̸� �߰�
    int attack;     // ����
    int defense;    // ����
    int speed;      // ���ǵ�
    int hp;         // ü�� ȸ��
}Item;
// ĳ���� ����ü ����
typedef struct {
    char name[20];           // �÷��̾� �̸�
    int HP;                  // ü��
    int level;               // ����
    int experience;          // ����ġ
    int agility;             // ��ø
    double physicalAttack;   // ���� ���ݷ�
    int defense;             // ����
    double speed;            // ���ǵ� 
    int strength;            // ��
    Item inventory[INVENTORY_SIZE]; // �κ��丮 (������ �迭)
    int inventoryCount;      // ���� �κ��丮�� �ִ� ������ ��
} Player;

//���� ����ü ����
typedef struct {
    int level;              // ����
    int strength;           // ��
    int agility;            // ��ø 
    int HP;                 // ü��
    double physicalAttack;  // ���� ���ݷ�
    double defense;         // ����
    double speed;           // ���ǵ�
} Monster;

// �÷��̾� ��ŷ ����ü
typedef struct {
    char name[20];
    int level;
    int experience;
} PlayerRanking;
typedef struct PlayerNode {
    Player player;
    struct PlayerNode* next;
} PlayerNode;

PlayerNode* rankingHead = NULL;

// ===== ���� ���� =====


int width = BASE_WIDTH, height = BASE_HEIGHT;
int monsterCount = MONSTER_COUNT_BASE;  // ���� ������ ���� �޶����� ���� ��

char** map;
int* monsterX;
int* monsterY;
int* monstersAlive;
Monster* monsters;

int playerX, playerY;
int keyX = -1, keyY = -1; // ���� ��ġ (-1�̸� ����)
int keyDropped = 0;       // ���谡 �̹� ����Ǿ����� ����

Player player;
PlayerRanking rankings[MAX_PLAYERS];
int rankingCount = 0;

void allocate_map(int new_width, int new_height) {
    map = (char**)malloc(new_height * sizeof(char*));
    if (!map) { // �Ҵ� ���� Ȯ��
        fprintf(stderr, "Error allocating memory for map\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < new_height; i++) {
        map[i] = (char*)malloc(new_width * sizeof(char));
        if (!map[i]) { // �Ҵ� ���� Ȯ��
            fprintf(stderr, "Error allocating memory for map row\n");
            exit(EXIT_FAILURE);
        }
    }
}

void free_map(int old_height) {
    for (int i = 0; i < old_height; i++) {
        free(map[i]);
    }
    free(map);
    map = NULL; // ���� �� �����͸� NULL�� ����
}

void allocate_monsters(int new_monster_count) {
    monsterX = (int*)malloc(new_monster_count * sizeof(int));
    monsterY = (int*)malloc(new_monster_count * sizeof(int));
    monstersAlive = (int*)malloc(new_monster_count * sizeof(int));
    monsters = (Monster*)malloc(new_monster_count * sizeof(Monster));
}

void free_monsters() {
    free(monsterX);
    free(monsterY);
    free(monstersAlive);
    free(monsters);
}

// ������ ���� ���ڿ�
const char* itemTypeNames[] = { "��", "����", "�Ź�", "����", "����", "���� ����", "����" };

// ���� ������ ȿ�� ���� �Լ�
void applyItemEffect(Player* player, Item item) {
    switch (item.type) {
    case WEAPON:
        player->physicalAttack += 10;
        printf("���ݷ��� 10 �����߽��ϴ�! ���� ���ݷ�: %d\n", player->physicalAttack);
        break;
    case HELMET:
        player->physicalAttack += 5;
        printf("���ݷ��� 5 �����߽��ϴ�! ���� ���ݷ�: %d\n", player->physicalAttack);
        break;
    case SHOES:
        player->speed += 10;
        printf("���ǵ尡 10 �����߽��ϴ�! ���� ���ǵ�: %d\n", player->speed);
        break;
    case CLOTHES:
        player->defense += 10;
        printf("������ 10 �����߽��ϴ�! ���� ����: %d\n", player->defense);
        break;
    case SHIELD:
        player->defense += 10;
        printf("������ 10 �����߽��ϴ�! ���� ����: %d\n", player->defense);
        break;
    case JEWEL:
        player->HP += 100; // ü�� Ǯ������
        printf("ü���� 100 ȸ���Ǿ����ϴ�! ���� ü��: %d\n", player->HP);
        break;
    case POTION:
        player->HP += 20; // ü�� +20
        printf("ü���� 20 �����߽��ϴ�! ���� ü��: %d\n", player->HP);
        break;
    default:
        printf("�� �� ���� ������ Ÿ���Դϴ�.\n");
        break;
    }
}
void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
// �ܼ� ȭ���� �����
void clear_screen() {
    system("cls"); // Windows���� ȭ�� �����
}
// ������ ���� �Լ�
Item createItem(ItemType type) {
    Item newItem;
    newItem.type = type;
    strcpy(newItem.name, itemTypeNames[type]); // �̸� ����
    newItem.attack = (type == WEAPON) ? 10 : (type == HELMET) ? 5 : 0;
    newItem.defense = (type == CLOTHES || type == SHIELD) ? 10 : 0;
    newItem.speed = (type == SHOES) ? 10 : 0;
    newItem.hp = (type == JEWEL) ? 100 : (type == POTION) ? 20 : 0;
    return newItem;
}

// �κ��丮�� ������ �߰�
void addItemToInventory(Player* player, Item item) {
    if (player->inventoryCount < INVENTORY_SIZE) {
        player->inventory[player->inventoryCount++] = item;
        printf("'%s' �������� �κ��丮�� �߰��߽��ϴ�.\n", item.name);
    }
    else {
        printf("�κ��丮�� ���� á���ϴ�! �������� �߰��� �� �����ϴ�.\n");
    }
}
// ���� ȹ�� �Լ�
void collectTreasure(Player* player) {
    if (map[playerY][playerX] == '*') {  // �÷��̾� ��ġ�� ������ �ִ��� Ȯ��
        printf("������ ȹ���߽��ϴ�!\n");
    }
    int randomIndex = rand() % 7; // 0~6 �� �ϳ��� �������� ����
    Item randomItem = createItem((ItemType)randomIndex);
    addItemToInventory(player, randomItem);

    // ���� ����
    map[playerY][playerX] = ' ';
}


// �κ��丮 ���� �� ������ ���
void showInventory(Player* player) {
    int inventoryStartColumn = width + 10;  // ���� �����ʿ� ������ �ΰ� �κ��丮�� ���
    setCursorPosition(inventoryStartColumn, 0); // �κ��丮�� ����� ��ġ�� Ŀ�� �̵�

    printf("\n=== �κ��丮 ===\n");
    if (player->inventoryCount == 0) {
        printf("�κ��丮�� ��� �ֽ��ϴ�.\n");
        _getch(); // ����� Ȯ�� �� �κ��丮 �ݱ�
        return;
    }

    for (int i = 0; i < player->inventoryCount; i++) {
        printf("%d. %s (���ݷ� +%d, ���� +%d, ���ǵ� +%d, ü�� +%d)\n",
            i + 1, player->inventory[i].name,
            player->inventory[i].attack,
            player->inventory[i].defense,
            player->inventory[i].speed,
            player->inventory[i].hp);
    }

    // ������ ��� �ɼ�
    int choice;
    printf("����� ������ ��ȣ�� �����ϼ��� (���: 0): ");
    scanf("%d", &choice);

    if (choice > 0 && choice <= player->inventoryCount) {
        Item item = player->inventory[choice - 1];
        printf("'%s' �������� ����մϴ�.\n", item.name);
        applyItemEffect(player, item); // ȿ�� ����

        // �κ��丮���� ������ ����
        for (int i = choice - 1; i < player->inventoryCount - 1; i++) {
            player->inventory[i] = player->inventory[i + 1];
        }
        player->inventoryCount--;
    }
    else if (choice != 0) {
        printf("�߸��� �����Դϴ�.\n");
    }

    _getch(); // ����� Ȯ�� �� �κ��丮 �ݱ�
}

// ������ ���� ���
void item_print(Item* item) {
    printf("\n=== ������ ���� ===\n");
    printf("�̸�: %s\n", item->name);
    printf("����: %s\n", itemTypeNames[item->type]);
    printf("���ݷ�: %d\n", item->attack);
    printf("����: %d\n", item->defense);
    printf("���ǵ�: %d\n", item->speed);
    printf("ü�� ȸ��: %d\n", item->hp);
}

// ���� ���� �Լ�
void saveRanking(Player* player) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (!newNode) {
        printf("�޸� �Ҵ� ����!\n");
        return;
    }
    newNode->player = *player;
    newNode->next = NULL;

    // ����Ʈ�� ����ִ� ���
    if (rankingHead == NULL) {
        rankingHead = newNode;
        return;
    }

    // ������ ��ġ�� ã�� ����Ʈ�� �߰� (���ĵ� ���� ����)
    PlayerNode* current = rankingHead;
    PlayerNode* prev = NULL;

    while (current != NULL &&
        (current->player.level > player->level ||
            (current->player.level == player->level && current->player.experience > player->experience))) {
        prev = current;
        current = current->next;
    }

    if (prev == NULL) { // �� �տ� ����
        newNode->next = rankingHead;
        rankingHead = newNode;
    }
    else { // �߰� �Ǵ� ���� ����
        prev->next = newNode;
        newNode->next = current;
    }
}
// �÷��̾� ���� ���� �Լ�
void showPlayerRankings() {
    printf("\n=== �÷��̾� ���� ===\n");

    if (rankingHead == NULL) {
        printf("����� ������ �����ϴ�.\n");
        return;
    }

    PlayerNode* current = rankingHead;
    int rank = 1;

    while (current != NULL) {
        printf("%d��: %s (����: %d, ����ġ: %d)\n", rank, current->player.name, current->player.level, current->player.experience);
        rank++;
        current = current->next;
    }
}

// ��ŷ ����Ʈ ���� �Լ�
void freeRankings() {
    PlayerNode* current = rankingHead;

    while (current != NULL) {
        PlayerNode* temp = current;
        current = current->next;
        free(temp);
    }

    rankingHead = NULL;
}

// Ŀ�� ����� �Լ�
void hide_cursor() {
    printf("\033[?25l");
}

// Ŀ�� ���̱� �Լ�
void show_cursor() {
    printf("\033[?25h");
}

void init_player(Player* player) {
    player->HP = 100;
    player->level = 1;
    player->experience = 0;
    player->strength = 10;
    player->agility = 10;
    player->defense = 5;
    player->physicalAttack = 10.0;
    player->speed = 1.0;
    player->inventoryCount = 0; // �κ��丮 �ʱ�ȭ
}

void init_monsters() {
    allocate_monsters(monsterCount); // ���� �޸� �Ҵ�
    for (int i = 0; i < monsterCount; i++) {
        monsterX[i] = rand() % (width - 2) + 1;
        monsterY[i] = rand() % (height - 2) + 1;
        monstersAlive[i] = 1;
        monsters[i].level = rand() % 3 + 1;
        monsters[i].HP = 20 + monsters[i].level * 10;
        monsters[i].physicalAttack = 5.0 + monsters[i].level;
        monsters[i].defense = 2.0 + monsters[i].level;
        map[monsterY[i]][monsterX[i]] = 'M';
    }
}


void init_map() {
    if (map != NULL) { // ���� �� ������ ����
        free_map(height);
    }
    allocate_map(width, height); // ���� �޸� �Ҵ�

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0 || i == height - 1 || j == 0 || j == width - 1) {
                map[i][j] = '#'; // ��
            }
            else {
                map[i][j] = ' '; // �� ����
            }
        }
    }

    playerX = width / 2;
    playerY = height / 2;
    map[playerY][playerX] = 'P'; // �÷��̾� ǥ��
    // ���� ���� �ʱ�ȭ
    keyX = -1;
    keyY = -1;
    keyDropped = 0;
}

// �÷��̾� ���� ���
void player_print(Player* player) {
    printf("=== %s ���� ===\n", player->name);
    printf("����: %d", player->level);
    printf("����ġ: %d", player->experience);
    printf("HP: %d\n", player->HP);
    printf("���ݷ�: %.1f\n", player->physicalAttack);
    printf("����: %d\n", player->defense);
    printf("���ǵ�: %.1f\n", player->speed);
    printf("��: %d, ��ø: %d\n", player->strength, player->agility);
}


// ����ġ ���� �� ������ �Լ�
void gainExperience(Player* player, int exp) {
    player->experience += exp;
    printf("\n%d ����ġ�� ȹ���߽��ϴ�!\n", exp);

    // ����ġ�� 100 �̻��� �� ������
    while (player->experience >= 100) {
        player->experience -= 100;
        player->level++;
        player->physicalAttack += 5; // ������ �� ���ݷ� ����
        printf("������! ���� ����: %d, ���ݷ�: %.1f, ü��: %d\n", player->level, player->physicalAttack, player->HP);
    }
}


void print_map() {
    printf("\033[H");// Ŀ���� �� ���� �̵� (ȭ�� ��ü ���ΰ�ħ ���)

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            printf("%c", map[i][j]);
        }
        printf("\n");
    }
}
void drop_treasure(int x, int y) {
    int directions[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    for (int i = 0; i < 4; i++) {
        int newX = x + directions[i][0];
        int newY = y + directions[i][1];
        if (map[newY][newX] == ' ') {
            map[newY][newX] = '*';
            printf("���Ͱ� ����(*)�� ����߷Ƚ��ϴ�!\n");
            return;
        }
    }
    // ��� ������ ���������� ���� ��ġ�� ���
    map[y][x] = '*';
    printf("���Ͱ� ����(*)�� ����߷Ƚ��ϴ�!\n");
}

void drop_key(int x, int y) {
    if (!keyDropped) {
        int directions[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];
            if (map[newY][newX] == ' ') {
                map[newY][newX] = 'K';
                keyX = newX;
                keyY = newY;
                printf("���Ͱ� ����(K)�� ����߷Ƚ��ϴ�!\n");
                keyDropped = 1;
                return;
            }
        }
        // ��� ������ ���������� ���� ��ġ�� ���
        if (map[y][x] == ' ') {
            map[y][x] = 'K';
            keyX = x;
            keyY = y;
            printf("���Ͱ� ����(K)�� ����߷Ƚ��ϴ�!\n");
            keyDropped = 1;
        }
    }
}
// ū ���ڷ� GAME OVER ���
void printGameOver() {
    clear_screen();
    printf("\n\n");
    printf("*****************************************\n");
    printf("*                                       *\n");
    printf("*              GAME OVER                *\n");
    printf("*                                       *\n");
    printf("*****************************************\n");
    printf("\n\n");
}
// ū ���ڷ� GAME CLAER ���
void printGameClear() {
    clear_screen();
    printf("\n\n");
    printf("*****************************************\n");
    printf("*                                       *\n");
    printf("*              GAME CLEAR               *\n");
    printf("*                                       *\n");
    printf("*****************************************\n");
    printf("\n\n");
}
// �÷��̾� ��� ó��
void handlePlayerDeath() {
    PlaySound(NULL, 0, 0); // ���� ��� ���� ���带 ����
    PlaySound(TEXT("gameover.wav"), NULL, SND_FILENAME | SND_ASYNC); // ���� ���� ���� ���

    printGameOver();
    saveRanking(&player); // ���� �÷��̾� ��ŷ ����
    showPlayerRankings(); // ����� ��ŷ ���

    printf("\n�ƹ� Ű�� ���� ���� �޴��� ���ư�����...\n");
    _getch(); // Ű �Է� ���
    // �÷��̾� ������ ���� ���� �ʱ�ȭ

    init_player(&player); // �÷��̾� ���� �ʱ�ȭ
    width = BASE_WIDTH;   // �� ũ�� �ʱ�ȭ
    height = BASE_HEIGHT;
    monsterCount = MONSTER_COUNT_BASE;

    currentLevel = 1;      // ���� �ʱ�ȭ
    // �ʰ� ���� �ʱ�ȭ
    init_map();
    init_monsters();
    while (1) {
        mainMenu();
    }
}
void battle(int monsterIndex) {
    if (monsterIndex < 0 || monsterIndex >= monsterCount) {
        printf("���� �ε����� �߸��Ǿ����ϴ�!\n");
        return;
    }
    printf("\n���Ϳ� ������ ���۵Ǿ����ϴ�!\n");

    while (player.HP > 0 && monsters[monsterIndex].HP > 0) {
        print_map();
        printf("�÷��̾� HP: %d, ���� HP: %d\n", player.HP, monsters[monsterIndex].HP);
        printf("q Ű�� ���� �����ϼ���!, i Ű�� �κ��丮�� ������!\n");

        // �÷��̾� ���� ó��
        while (1) {
            if (_kbhit()) {
                char input = _getch();
                if (input == 'q') {
                    printf("�÷��̾ ���͸� �����߽��ϴ�!\n");
                    monsters[monsterIndex].HP -= player.physicalAttack;
                    break;
                }
            }
        }

        if (monsters[monsterIndex].HP <= 0) {

            printf("���͸� óġ�߽��ϴ�!\n");
            map[monsterY[monsterIndex]][monsterX[monsterIndex]] = ' ';
            monstersAlive[monsterIndex] = 0;
            gainExperience(&player, monsters[monsterIndex].level * 10);

            if (rand() % 2 == 0) {
                drop_treasure(monsterX[monsterIndex], monsterY[monsterIndex]);
            }

            int allDefeated = 1;
            for (int i = 0; i < monsterCount; i++) {
                if (monstersAlive[i]) {
                    allDefeated = 0;
                    break;
                }
            }
            if (allDefeated) {
                printf("������ ���͸� óġ�߽��ϴ�! ���踦 ����߸��ϴ�.\n");
                drop_key(monsterX[monsterIndex], monsterY[monsterIndex]);
            }

            // ���� �� �÷��̾� ��ġ �ٽ� ǥ��
            map[playerY][playerX] = 'P';
            return;
        }

        // ���� �ݰ� ó��
        printf("���Ͱ� �÷��̾ �����߽��ϴ�!\n");
        player.HP -= monsters[monsterIndex].physicalAttack;

        if (player.HP <= 0) {
            handlePlayerDeath();
            return;
        }

        // ���� �� �÷��̾� ��ġ �ٽ� ǥ��
        map[playerY][playerX] = 'P';
    }
}


void move_monsters(int turn) {
    if (turn % 5 != 0) return; // ���ʹ� 5�ϸ��� �� �� �̵�

    for (int i = 0; i < monsterCount; i++) {
        if (!monstersAlive[i]) continue;

        int direction = rand() % 4; // 0: ��, 1: �Ʒ�, 2: ����, 3: ������
        int newX = monsterX[i];
        int newY = monsterY[i];

        if (direction == 0) newY--;       // ���� �̵�
        else if (direction == 1) newY++;  // �Ʒ��� �̵�
        else if (direction == 2) newX--;  // �������� �̵�
        else if (direction == 3) newX++;  // ���������� �̵�

        // �� �Ǵ� �ٸ� ���Ϳ� �浹���� �ʴ� ��� �̵�
        if (map[newY][newX] == ' ') {
            map[monsterY[i]][monsterX[i]] = ' '; // ���� ��ġ �����
            monsterX[i] = newX;
            monsterY[i] = newY;
            map[monsterY[i]][monsterX[i]] = 'M'; // ���ο� ��ġ�� ���� ǥ��
        }

    }
}
void onKeyCollected() {
    printf("\n���踦 ȹ���߽��ϴ�! �� ũ��� ���� ���ڰ� �����մϴ�.\n");
    currentLevel++;
    // ���� ������ ����
    free_map(height);
    free_monsters();

    // �� ũ��� ���� �� ����
    if (currentLevel <= maxLevel) {
        width += 5;
        height += 3;
        monsterCount += 2;
        // ���ο� ������ �Ҵ�
        allocate_map(width, height);
        allocate_monsters(monsterCount);

        // �� �ʱ�ȭ �� ������ �缳��
        init_map();
        init_monsters();

        clear_screen();
    }
    else {
        saveRanking(&player); // ���� �÷��̾� ��ŷ ����
        showPlayerRankings(); // ����� ��ŷ ���
        printGameClear();
        printf("�ִ� ������ �����߽��ϴ�! ������ �����մϴ�.\n");
        _getch();             // ����� �Է� ���

        currentLevel = 1;
        mainMenu();
    };



}

void move_player(int dx, int dy) {
    int newX = playerX + dx;
    int newY = playerY + dy;

    if (map[newY][newX] == '#') return;

    if (newX == keyX && newY == keyY) {
        printf("���踦 ȹ���߽��ϴ�! ���� �ܰ�� �Ѿ�ϴ�!\n");
        _getch();  // �޽��� Ȯ���� ���� Ű �Է� ���
        onKeyCollected(); // ���� ȹ�� �� onKeyCollected ȣ��
        return;
    }
    // ������ �Ծ��� ��� ó��
    if (map[newY][newX] == '*') {
        printf("������ �߰��߽��ϴ�!\n");
        collectTreasure(&player); // ���� ȹ�� �Լ� ȣ��
    }

    map[playerY][playerX] = ' ';
    playerX = newX;
    playerY = newY;
    map[playerY][playerX] = 'P';

    // ���Ϳ� ����ġ�� ���� ����
    for (int i = 0; i < monsterCount; i++) {
        if (monstersAlive[i] && playerX == monsterX[i] && playerY == monsterY[i]) {
            battle(i);
        }
    }
}

void mainMenu() {
    clear_screen();
    int turn = 0;
    // ����
    int choice;
    do {
        printf("\n=== �޴� ===\n");
        printf("1. �÷��̾� ����\n");
        printf("2. ���� ����\n");
        printf("3. �÷��̾� ���� ����\n");
        printf("4. ���Ӽ���\n");
        printf("5. ����\n");
        printf("����: ");
        scanf("%d", &choice);


        switch (choice) {
        case 1:
            // �Ӽ� ���
            player_print(&player);
            break;
        case 2:
            // ���� ���� �� �÷��̾� �̸� �Է�
            printf("�÷��̾� �̸��� �Է��ϼ���: ");
            scanf("%s", player.name);

            // ���� ����
            hide_cursor();

            // bgm ���
            PlaySound(TEXT("bgm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

            while (player.HP > 0) {
                print_map();
                printf("HP: %d, ����ġ: %d, ���ݷ�: %.1f\n", player.HP, player.experience, player.physicalAttack);

                if (_kbhit()) {
                    char input = _getch();
                    if (input == 'w') move_player(0, -1);
                    else if (input == 's') move_player(0, 1);
                    else if (input == 'a') move_player(-1, 0);
                    else if (input == 'd') move_player(1, 0);
                    else if (input == 'i') {
                        printf("�κ��丮�� ���ϴ�.\n");
                        showInventory(&player);
                    }
                }

                // ���� �ڵ� �̵�
                move_monsters(turn);
                Sleep(100);
                turn++;

            }
            PlaySound(NULL, 0, 0); // BGM ����
            show_cursor();
            break;
        case 3:
            showPlayerRankings(); // ĳ���� ����
            break;
        case 4:
            printf("===���Ӽ���===\n");
            printf("������ �����ϸ� �÷��̾� �̸��� �Է��Ͽ� ��ŷȭ �� �� �ֽ��ϴ�.\n");
            printf("�÷��̾ ����ϸ� ������ ���µ˴ϴ�.\n");
            printf("���ƴٴϰ� �ִ� ����(M)�� �÷��̾�(P)�� ������ q�� ���� �����մϴ�.\n");
            printf("���ʹ� �÷��̾ �����ϱ� ������ �������� �ʽ��ϴ�. \n ������ �÷��̾ �����ϰ� ���� ���͵� ����� ������ �� �ֽ��ϴ�.\n");
            printf("���͸� ���̸� 50������ Ȯ���� ����(*)�� �������ϴ�.\n");
            printf("������ ȹ���ϸ� �������� �������� ���� �� �ֽ��ϴ�.\n");
            printf("���� �߿� e�� ������ �κ��丮�� ���� �������� ����� �� �ֽ��ϴ�.\n");
            printf("������ ���͸� ���̸� �����ܰ�� �� �� �ִ� ����(K)�� ����մϴ�.\n");
            printf("���踦 ������ ���� ������ �� �� �ֽ��ϴ�.\n");
            printf("����� ������ ������ ���ϴ�.\n");
            break;
        case 5:
            printf("���α׷��� �����մϴ�.\n");
            exit(1);
            break;
        default:
            printf("�߸��� �����Դϴ�. �ٽ� �����ϼ���.\n");
        }
    } while (choice != 5);
}

int main() {
    srand((unsigned int)time(NULL));
    init_player(&player);
    init_map();
    init_monsters();

    while (1) { // ���� ����
        mainMenu(); // ���� �޴� ȣ��
        printf("\n������ �����Ϸ��� �ƹ� Ű�� ��������...\n");
        _getch(); // Ű �Է� ���
    }
    free_map(height);
    free_monsters();
    show_cursor();
    return 0;
}
