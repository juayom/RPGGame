#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <windows.h> // Sleep 사용 및 PlaySound 사용
#include <mmsystem.h> // PlaySound 사용
#pragma comment(lib, "winmm.lib") // 링크할 라이브러리 지정

#define BASE_WIDTH 20
#define BASE_HEIGHT 10
#define MONSTER_COUNT_BASE 3
#define INVENTORY_SIZE 10
#define MAX_PLAYERS 10

int currentLevel = 1; // 현재 게임 레벨
const int maxLevel = 3; // 최대 레벨

void mainMenu();
//아이템 유형 열거
typedef enum {
    WEAPON,  // 공격력 강화 (검, 공격력 + 10)
    HELMET,  // 공격력 상승 (모자, 공격력 + 5)
    SHOES,   // 스피드 (레벨 스피드 올려줌, 스피드 + 10)
    CLOTHES, // 방아력 강화 (갑옷, 방어력 + 10)
    SHIELD,  //방아력 강화 (방패, 방어력 + 10)
    JEWEL,  // 체력 회복 (생명 보석, HP 풀게이지)
    POTION  // 체력 회복 체력 물약, HP + 20)
}ItemType;

// 아이템 구조체 정의
typedef struct {
    ItemType type;
    char name[50];  // 아이템 이름 추가
    int attack;     // 공격
    int defense;    // 방어력
    int speed;      // 스피드
    int hp;         // 체력 회복
}Item;
// 캐릭터 구조체 정의
typedef struct {
    char name[20];           // 플레이어 이름
    int HP;                  // 체력
    int level;               // 레벨
    int experience;          // 경험치
    int agility;             // 민첩
    double physicalAttack;   // 물리 공격력
    int defense;             // 방어력
    double speed;            // 스피드 
    int strength;            // 힘
    Item inventory[INVENTORY_SIZE]; // 인벤토리 (아이템 배열)
    int inventoryCount;      // 현재 인벤토리에 있는 아이템 수
} Player;

//몬스터 구조체 정의
typedef struct {
    int level;              // 레벨
    int strength;           // 힘
    int agility;            // 민첩 
    int HP;                 // 체력
    double physicalAttack;  // 물리 공격력
    double defense;         // 방어력
    double speed;           // 스피드
} Monster;

// 플레이어 랭킹 구조체
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

// ===== 전역 변수 =====


int width = BASE_WIDTH, height = BASE_HEIGHT;
int monsterCount = MONSTER_COUNT_BASE;  // 현재 레벨에 따라 달라지는 몬스터 수

char** map;
int* monsterX;
int* monsterY;
int* monstersAlive;
Monster* monsters;

int playerX, playerY;
int keyX = -1, keyY = -1; // 열쇠 위치 (-1이면 없음)
int keyDropped = 0;       // 열쇠가 이미 드랍되었는지 여부

Player player;
PlayerRanking rankings[MAX_PLAYERS];
int rankingCount = 0;

void allocate_map(int new_width, int new_height) {
    map = (char**)malloc(new_height * sizeof(char*));
    if (!map) { // 할당 실패 확인
        fprintf(stderr, "Error allocating memory for map\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < new_height; i++) {
        map[i] = (char*)malloc(new_width * sizeof(char));
        if (!map[i]) { // 할당 실패 확인
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
    map = NULL; // 해제 후 포인터를 NULL로 설정
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

// 아이템 유형 문자열
const char* itemTypeNames[] = { "검", "모자", "신발", "갑옷", "방패", "생명 보석", "물약" };

// 공통 아이템 효과 적용 함수
void applyItemEffect(Player* player, Item item) {
    switch (item.type) {
    case WEAPON:
        player->physicalAttack += 10;
        printf("공격력이 10 증가했습니다! 현재 공격력: %d\n", player->physicalAttack);
        break;
    case HELMET:
        player->physicalAttack += 5;
        printf("공격력이 5 증가했습니다! 현재 공격력: %d\n", player->physicalAttack);
        break;
    case SHOES:
        player->speed += 10;
        printf("스피드가 10 증가했습니다! 현재 스피드: %d\n", player->speed);
        break;
    case CLOTHES:
        player->defense += 10;
        printf("방어력이 10 증가했습니다! 현재 방어력: %d\n", player->defense);
        break;
    case SHIELD:
        player->defense += 10;
        printf("방어력이 10 증가했습니다! 현재 방어력: %d\n", player->defense);
        break;
    case JEWEL:
        player->HP += 100; // 체력 풀게이지
        printf("체력이 100 회복되었습니다! 현재 체력: %d\n", player->HP);
        break;
    case POTION:
        player->HP += 20; // 체력 +20
        printf("체력이 20 증가했습니다! 현재 체력: %d\n", player->HP);
        break;
    default:
        printf("알 수 없는 아이템 타입입니다.\n");
        break;
    }
}
void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
// 콘솔 화면을 지우기
void clear_screen() {
    system("cls"); // Windows에서 화면 지우기
}
// 아이템 생성 함수
Item createItem(ItemType type) {
    Item newItem;
    newItem.type = type;
    strcpy(newItem.name, itemTypeNames[type]); // 이름 설정
    newItem.attack = (type == WEAPON) ? 10 : (type == HELMET) ? 5 : 0;
    newItem.defense = (type == CLOTHES || type == SHIELD) ? 10 : 0;
    newItem.speed = (type == SHOES) ? 10 : 0;
    newItem.hp = (type == JEWEL) ? 100 : (type == POTION) ? 20 : 0;
    return newItem;
}

// 인벤토리에 아이템 추가
void addItemToInventory(Player* player, Item item) {
    if (player->inventoryCount < INVENTORY_SIZE) {
        player->inventory[player->inventoryCount++] = item;
        printf("'%s' 아이템을 인벤토리에 추가했습니다.\n", item.name);
    }
    else {
        printf("인벤토리가 가득 찼습니다! 아이템을 추가할 수 없습니다.\n");
    }
}
// 보물 획득 함수
void collectTreasure(Player* player) {
    if (map[playerY][playerX] == '*') {  // 플레이어 위치에 보물이 있는지 확인
        printf("보물을 획득했습니다!\n");
    }
    int randomIndex = rand() % 7; // 0~6 중 하나를 랜덤으로 선택
    Item randomItem = createItem((ItemType)randomIndex);
    addItemToInventory(player, randomItem);

    // 보물 제거
    map[playerY][playerX] = ' ';
}


// 인벤토리 보기 및 아이템 사용
void showInventory(Player* player) {
    int inventoryStartColumn = width + 10;  // 맵의 오른쪽에 공간을 두고 인벤토리를 출력
    setCursorPosition(inventoryStartColumn, 0); // 인벤토리를 출력할 위치로 커서 이동

    printf("\n=== 인벤토리 ===\n");
    if (player->inventoryCount == 0) {
        printf("인벤토리가 비어 있습니다.\n");
        _getch(); // 사용자 확인 후 인벤토리 닫기
        return;
    }

    for (int i = 0; i < player->inventoryCount; i++) {
        printf("%d. %s (공격력 +%d, 방어력 +%d, 스피드 +%d, 체력 +%d)\n",
            i + 1, player->inventory[i].name,
            player->inventory[i].attack,
            player->inventory[i].defense,
            player->inventory[i].speed,
            player->inventory[i].hp);
    }

    // 아이템 사용 옵션
    int choice;
    printf("사용할 아이템 번호를 선택하세요 (취소: 0): ");
    scanf("%d", &choice);

    if (choice > 0 && choice <= player->inventoryCount) {
        Item item = player->inventory[choice - 1];
        printf("'%s' 아이템을 사용합니다.\n", item.name);
        applyItemEffect(player, item); // 효과 적용

        // 인벤토리에서 아이템 제거
        for (int i = choice - 1; i < player->inventoryCount - 1; i++) {
            player->inventory[i] = player->inventory[i + 1];
        }
        player->inventoryCount--;
    }
    else if (choice != 0) {
        printf("잘못된 선택입니다.\n");
    }

    _getch(); // 사용자 확인 후 인벤토리 닫기
}

// 아이템 정보 출력
void item_print(Item* item) {
    printf("\n=== 아이템 정보 ===\n");
    printf("이름: %s\n", item->name);
    printf("유형: %s\n", itemTypeNames[item->type]);
    printf("공격력: %d\n", item->attack);
    printf("방어력: %d\n", item->defense);
    printf("스피드: %d\n", item->speed);
    printf("체력 회복: %d\n", item->hp);
}

// 순위 저장 함수
void saveRanking(Player* player) {
    PlayerNode* newNode = (PlayerNode*)malloc(sizeof(PlayerNode));
    if (!newNode) {
        printf("메모리 할당 실패!\n");
        return;
    }
    newNode->player = *player;
    newNode->next = NULL;

    // 리스트가 비어있는 경우
    if (rankingHead == NULL) {
        rankingHead = newNode;
        return;
    }

    // 삽입할 위치를 찾아 리스트에 추가 (정렬된 상태 유지)
    PlayerNode* current = rankingHead;
    PlayerNode* prev = NULL;

    while (current != NULL &&
        (current->player.level > player->level ||
            (current->player.level == player->level && current->player.experience > player->experience))) {
        prev = current;
        current = current->next;
    }

    if (prev == NULL) { // 맨 앞에 삽입
        newNode->next = rankingHead;
        rankingHead = newNode;
    }
    else { // 중간 또는 끝에 삽입
        prev->next = newNode;
        newNode->next = current;
    }
}
// 플레이어 순위 보기 함수
void showPlayerRankings() {
    printf("\n=== 플레이어 순위 ===\n");

    if (rankingHead == NULL) {
        printf("저장된 순위가 없습니다.\n");
        return;
    }

    PlayerNode* current = rankingHead;
    int rank = 1;

    while (current != NULL) {
        printf("%d위: %s (레벨: %d, 경험치: %d)\n", rank, current->player.name, current->player.level, current->player.experience);
        rank++;
        current = current->next;
    }
}

// 랭킹 리스트 해제 함수
void freeRankings() {
    PlayerNode* current = rankingHead;

    while (current != NULL) {
        PlayerNode* temp = current;
        current = current->next;
        free(temp);
    }

    rankingHead = NULL;
}

// 커서 숨기기 함수
void hide_cursor() {
    printf("\033[?25l");
}

// 커서 보이기 함수
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
    player->inventoryCount = 0; // 인벤토리 초기화
}

void init_monsters() {
    allocate_monsters(monsterCount); // 동적 메모리 할당
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
    if (map != NULL) { // 기존 맵 데이터 해제
        free_map(height);
    }
    allocate_map(width, height); // 동적 메모리 할당

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0 || i == height - 1 || j == 0 || j == width - 1) {
                map[i][j] = '#'; // 벽
            }
            else {
                map[i][j] = ' '; // 빈 공간
            }
        }
    }

    playerX = width / 2;
    playerY = height / 2;
    map[playerY][playerX] = 'P'; // 플레이어 표시
    // 열쇠 상태 초기화
    keyX = -1;
    keyY = -1;
    keyDropped = 0;
}

// 플레이어 정보 출력
void player_print(Player* player) {
    printf("=== %s 상태 ===\n", player->name);
    printf("레벨: %d", player->level);
    printf("경험치: %d", player->experience);
    printf("HP: %d\n", player->HP);
    printf("공격력: %.1f\n", player->physicalAttack);
    printf("방어력: %d\n", player->defense);
    printf("스피드: %.1f\n", player->speed);
    printf("힘: %d, 민첩: %d\n", player->strength, player->agility);
}


// 경험치 지급 및 레벨업 함수
void gainExperience(Player* player, int exp) {
    player->experience += exp;
    printf("\n%d 경험치를 획득했습니다!\n", exp);

    // 경험치가 100 이상일 때 레벨업
    while (player->experience >= 100) {
        player->experience -= 100;
        player->level++;
        player->physicalAttack += 5; // 레벨업 시 공격력 증가
        printf("레벨업! 현재 레벨: %d, 공격력: %.1f, 체력: %d\n", player->level, player->physicalAttack, player->HP);
    }
}


void print_map() {
    printf("\033[H");// 커서를 맨 위로 이동 (화면 전체 새로고침 대신)

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
            printf("몬스터가 보물(*)을 떨어뜨렸습니다!\n");
            return;
        }
    }
    // 모든 방향이 막혀있으면 현재 위치에 드롭
    map[y][x] = '*';
    printf("몬스터가 보물(*)을 떨어뜨렸습니다!\n");
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
                printf("몬스터가 열쇠(K)를 떨어뜨렸습니다!\n");
                keyDropped = 1;
                return;
            }
        }
        // 모든 방향이 막혀있으면 현재 위치에 드롭
        if (map[y][x] == ' ') {
            map[y][x] = 'K';
            keyX = x;
            keyY = y;
            printf("몬스터가 열쇠(K)를 떨어뜨렸습니다!\n");
            keyDropped = 1;
        }
    }
}
// 큰 글자로 GAME OVER 출력
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
// 큰 글자로 GAME CLAER 출력
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
// 플레이어 사망 처리
void handlePlayerDeath() {
    PlaySound(NULL, 0, 0); // 현재 재생 중인 사운드를 정지
    PlaySound(TEXT("gameover.wav"), NULL, SND_FILENAME | SND_ASYNC); // 게임 오버 사운드 재생

    printGameOver();
    saveRanking(&player); // 현재 플레이어 랭킹 저장
    showPlayerRankings(); // 저장된 랭킹 출력

    printf("\n아무 키나 눌러 메인 메뉴로 돌아가세요...\n");
    _getch(); // 키 입력 대기
    // 플레이어 정보와 게임 상태 초기화

    init_player(&player); // 플레이어 정보 초기화
    width = BASE_WIDTH;   // 맵 크기 초기화
    height = BASE_HEIGHT;
    monsterCount = MONSTER_COUNT_BASE;

    currentLevel = 1;      // 레벨 초기화
    // 맵과 몬스터 초기화
    init_map();
    init_monsters();
    while (1) {
        mainMenu();
    }
}
void battle(int monsterIndex) {
    if (monsterIndex < 0 || monsterIndex >= monsterCount) {
        printf("몬스터 인덱스가 잘못되었습니다!\n");
        return;
    }
    printf("\n몬스터와 전투가 시작되었습니다!\n");

    while (player.HP > 0 && monsters[monsterIndex].HP > 0) {
        print_map();
        printf("플레이어 HP: %d, 몬스터 HP: %d\n", player.HP, monsters[monsterIndex].HP);
        printf("q 키를 눌러 공격하세요!, i 키로 인벤토리를 여세요!\n");

        // 플레이어 공격 처리
        while (1) {
            if (_kbhit()) {
                char input = _getch();
                if (input == 'q') {
                    printf("플레이어가 몬스터를 공격했습니다!\n");
                    monsters[monsterIndex].HP -= player.physicalAttack;
                    break;
                }
            }
        }

        if (monsters[monsterIndex].HP <= 0) {

            printf("몬스터를 처치했습니다!\n");
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
                printf("마지막 몬스터를 처치했습니다! 열쇠를 떨어뜨립니다.\n");
                drop_key(monsterX[monsterIndex], monsterY[monsterIndex]);
            }

            // 전투 후 플레이어 위치 다시 표시
            map[playerY][playerX] = 'P';
            return;
        }

        // 몬스터 반격 처리
        printf("몬스터가 플레이어를 공격했습니다!\n");
        player.HP -= monsters[monsterIndex].physicalAttack;

        if (player.HP <= 0) {
            handlePlayerDeath();
            return;
        }

        // 전투 후 플레이어 위치 다시 표시
        map[playerY][playerX] = 'P';
    }
}


void move_monsters(int turn) {
    if (turn % 5 != 0) return; // 몬스터는 5턴마다 한 번 이동

    for (int i = 0; i < monsterCount; i++) {
        if (!monstersAlive[i]) continue;

        int direction = rand() % 4; // 0: 위, 1: 아래, 2: 왼쪽, 3: 오른쪽
        int newX = monsterX[i];
        int newY = monsterY[i];

        if (direction == 0) newY--;       // 위로 이동
        else if (direction == 1) newY++;  // 아래로 이동
        else if (direction == 2) newX--;  // 왼쪽으로 이동
        else if (direction == 3) newX++;  // 오른쪽으로 이동

        // 벽 또는 다른 몬스터와 충돌하지 않는 경우 이동
        if (map[newY][newX] == ' ') {
            map[monsterY[i]][monsterX[i]] = ' '; // 기존 위치 지우기
            monsterX[i] = newX;
            monsterY[i] = newY;
            map[monsterY[i]][monsterX[i]] = 'M'; // 새로운 위치에 몬스터 표시
        }

    }
}
void onKeyCollected() {
    printf("\n열쇠를 획득했습니다! 맵 크기와 몬스터 숫자가 증가합니다.\n");
    currentLevel++;
    // 기존 데이터 해제
    free_map(height);
    free_monsters();

    // 맵 크기와 몬스터 수 증가
    if (currentLevel <= maxLevel) {
        width += 5;
        height += 3;
        monsterCount += 2;
        // 새로운 데이터 할당
        allocate_map(width, height);
        allocate_monsters(monsterCount);

        // 맵 초기화 및 데이터 재설정
        init_map();
        init_monsters();

        clear_screen();
    }
    else {
        saveRanking(&player); // 현재 플레이어 랭킹 저장
        showPlayerRankings(); // 저장된 랭킹 출력
        printGameClear();
        printf("최대 레벨에 도달했습니다! 게임을 종료합니다.\n");
        _getch();             // 사용자 입력 대기

        currentLevel = 1;
        mainMenu();
    };



}

void move_player(int dx, int dy) {
    int newX = playerX + dx;
    int newY = playerY + dy;

    if (map[newY][newX] == '#') return;

    if (newX == keyX && newY == keyY) {
        printf("열쇠를 획득했습니다! 다음 단계로 넘어갑니다!\n");
        _getch();  // 메시지 확인을 위해 키 입력 대기
        onKeyCollected(); // 열쇠 획득 시 onKeyCollected 호출
        return;
    }
    // 보물을 먹었을 경우 처리
    if (map[newY][newX] == '*') {
        printf("보물을 발견했습니다!\n");
        collectTreasure(&player); // 보물 획득 함수 호출
    }

    map[playerY][playerX] = ' ';
    playerX = newX;
    playerY = newY;
    map[playerY][playerX] = 'P';

    // 몬스터와 마주치면 전투 시작
    for (int i = 0; i < monsterCount; i++) {
        if (monstersAlive[i] && playerX == monsterX[i] && playerY == monsterY[i]) {
            battle(i);
        }
    }
}

void mainMenu() {
    clear_screen();
    int turn = 0;
    // 선택
    int choice;
    do {
        printf("\n=== 메뉴 ===\n");
        printf("1. 플레이어 정보\n");
        printf("2. 전투 시작\n");
        printf("3. 플레이어 순위 보기\n");
        printf("4. 게임설명\n");
        printf("5. 종료\n");
        printf("선택: ");
        scanf("%d", &choice);


        switch (choice) {
        case 1:
            // 속성 출력
            player_print(&player);
            break;
        case 2:
            // 전투 시작 시 플레이어 이름 입력
            printf("플레이어 이름을 입력하세요: ");
            scanf("%s", player.name);

            // 전투 시작
            hide_cursor();

            // bgm 재생
            PlaySound(TEXT("bgm.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

            while (player.HP > 0) {
                print_map();
                printf("HP: %d, 경험치: %d, 공격력: %.1f\n", player.HP, player.experience, player.physicalAttack);

                if (_kbhit()) {
                    char input = _getch();
                    if (input == 'w') move_player(0, -1);
                    else if (input == 's') move_player(0, 1);
                    else if (input == 'a') move_player(-1, 0);
                    else if (input == 'd') move_player(1, 0);
                    else if (input == 'i') {
                        printf("인벤토리를 엽니다.\n");
                        showInventory(&player);
                    }
                }

                // 몬스터 자동 이동
                move_monsters(turn);
                Sleep(100);
                turn++;

            }
            PlaySound(NULL, 0, 0); // BGM 정지
            show_cursor();
            break;
        case 3:
            showPlayerRankings(); // 캐릭터 순위
            break;
        case 4:
            printf("===게임설명===\n");
            printf("전투를 시작하면 플레이어 이름을 입력하여 랭킹화 할 수 있습니다.\n");
            printf("플레이어가 사망하면 게임은 리셋됩니다.\n");
            printf("돌아다니고 있는 몬스터(M)와 플레이어(P)가 만나면 q를 눌러 공격합니다.\n");
            printf("몬스터는 플레이어가 공격하기 전까지 공격하지 않습니다. \n 하지만 플레이어가 공격하고 나면 몬스터도 당신을 공격할 수 있습니다.\n");
            printf("몬스터를 죽이면 50프로의 확률로 보물(*)이 떨어집니다.\n");
            printf("보물을 획득하면 랜덤으로 아이템을 얻을 수 있습니다.\n");
            printf("전투 중에 e를 누르면 인벤토리를 열어 아이템을 사용할 수 있습니다.\n");
            printf("마지막 몬스터를 죽이면 다음단계로 갈 수 있는 열쇠(K)를 드롭합니다.\n");
            printf("열쇠를 얻으면 다음 전투를 할 수 있습니다.\n");
            printf("당신의 전투에 무운을 빕니다.\n");
            break;
        case 5:
            printf("프로그램을 종료합니다.\n");
            exit(1);
            break;
        default:
            printf("잘못된 선택입니다. 다시 선택하세요.\n");
        }
    } while (choice != 5);
}

int main() {
    srand((unsigned int)time(NULL));
    init_player(&player);
    init_map();
    init_monsters();

    while (1) { // 메인 루프
        mainMenu(); // 메인 메뉴 호출
        printf("\n게임을 종료하려면 아무 키나 누르세요...\n");
        _getch(); // 키 입력 대기
    }
    free_map(height);
    free_monsters();
    show_cursor();
    return 0;
}
