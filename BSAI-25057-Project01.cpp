#include<iostream>
#include<fstream>
#include<windows.h>
#include<conio.h>
#include<math.h>
using namespace std;

void getRowColbyLeftClick(int& rpos, int& cpos)
{
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD Events;
    INPUT_RECORD InputRecord;
    SetConsoleMode(hInput, ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);

    do
    {
        ReadConsoleInput(hInput, &InputRecord, 1, &Events);
        if (InputRecord.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
        {
            cpos = InputRecord.Event.MouseEvent.dwMousePosition.X;
            rpos = InputRecord.Event.MouseEvent.dwMousePosition.Y;
            break;
        }
    } while (true);
}

void gotoRowCol(int rpos, int cpos)
{
    COORD scrn;
    HANDLE hOuput = GetStdHandle(STD_OUTPUT_HANDLE);
    scrn.X = cpos;
    scrn.Y = rpos;
    SetConsoleCursorPosition(hOuput, scrn);
}

void color(int k)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, k);
}

void hideConsoleCursor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}


void drawLine(int p1r, int p1c, int p2r, int p2c, double alpha = 0.01, char sym = -37)
{
    for (double a = 0; a <= 1; a += alpha)
    {
        int pr = (int)ceil(p1r * a + p2r * (1 - a));

        if (p1r == p2r)
            pr = p1r;

        int pc = (int)ceil(p1c * a + p2c * (1 - a));

        if (p1c == p2c)
            pc = p1c;

        gotoRowCol(pr, pc);
        cout << sym;
    }
}

struct Cell
{
    bool isMine;
    bool isFlagged;
    int  nCount;
    bool isOpen;
};

struct QueueNode
{
    int row;
    int col;
    QueueNode* next;
};

struct Queue
{
    QueueNode* front;
    QueueNode* back;
};

void initQueue(Queue& q)
{
    q.front = nullptr;
    q.back = nullptr;
}

bool isQueueEmpty(Queue& q)
{
    return q.front == nullptr;
}

void enqueue(Queue& q, int row, int col)
{
    QueueNode* node = new QueueNode;
    node->row = row;
    node->col = col;
    node->next = nullptr;

    if (q.back == nullptr)
    {
        q.front = node;
        q.back = node;
    }
    else
    {
        q.back->next = node;
        q.back = node;
    }
}

void dequeue(Queue& q, int& row, int& col)
{
    QueueNode* temp = q.front;
    row = temp->row;
    col = temp->col;

    q.front = q.front->next;
    if (q.front == nullptr)
        q.back = nullptr;

    delete temp;
}

const int cell_width = 4;
const int cell_height = 2;
const int grid_leftt = 0;

const int row_title = 0;
const int title_space = 6;
const int status_row_num = 1;
const int grid_top = 4;

const char* SAVE_FILE = "minesweeper_save.dat";
const char  flag_icon = '+';


Cell** createGrid(int rows, int cols)
{
    Cell** grid = new Cell * [rows];
    for (int r = 0; r < rows; r++)
    {
        grid[r] = new Cell[cols];
        for (int c = 0; c < cols; c++)
        {
            grid[r][c].isMine = false;
            grid[r][c].isFlagged = false;
            grid[r][c].nCount = 0;
            grid[r][c].isOpen = false;
        }
    }
    return grid;
}

void destroyGrid(Cell** grid, int rows)
{
    for (int r = 0; r < rows; r++)
        delete[] grid[r];

    delete[] grid;
}

bool isValidCell(int row, int col, int rows, int cols)
{
    return row >= 0 and row < rows and col >= 0 and col < cols;
}

void updateNeighborCounts(Cell** grid, int rows, int cols, int mineRow, int mineCol)
{
    for (int r = mineRow - 1; r <= mineRow + 1; r++)
    {
        for (int c = mineCol - 1; c <= mineCol + 1; c++)
        {
            if (r == mineRow and c == mineCol)
                continue;

            if (isValidCell(r, c, rows, cols) and !grid[r][c].isMine)
                grid[r][c].nCount++;
        }
    }
}

void placeMines(Cell** grid, int rows, int cols, int mines)
{
    int placed = 0;

    while (placed < mines)
    {
        int r = rand() % rows;
        int c = rand() % cols;

        if (!grid[r][c].isMine)
        {
            grid[r][c].isMine = true;
            updateNeighborCounts(grid, rows, cols, r, c);
            placed++;
        }
    }
}

void openCell(Cell** grid, int row, int col, int& openedCount)
{
    if (grid[row][col].isFlagged or grid[row][col].isOpen)
        return;

    grid[row][col].isOpen = true;

    if (!grid[row][col].isMine)
        openedCount++;
}

void cascadeOpenCells(Cell** grid, int rows, int cols, int startRow, int startCol, int& openedCount)
{
    openCell(grid, startRow, startCol, openedCount);

    if (grid[startRow][startCol].nCount != 0)
        return;

    Queue q;
    initQueue(q);

    for (int r = startRow - 1; r <= startRow + 1; r++)
    {
        for (int c = startCol - 1; c <= startCol + 1; c++)
        {
            if (r == startRow and c == startCol)
                continue;

            if (isValidCell(r, c, rows, cols) and !grid[r][c].isOpen and !grid[r][c].isFlagged)
            {
                openCell(grid, r, c, openedCount);
                enqueue(q, r, c);
            }
        }
    }

    while (!isQueueEmpty(q))
    {
        int row, col;
        dequeue(q, row, col);

        if (grid[row][col].nCount != 0)
            continue;

        for (int r = row - 1; r <= row + 1; r++)
        {
            for (int c = col - 1; c <= col + 1; c++)
            {
                if (r == row and c == col)
                    continue;

                if (isValidCell(r, c, rows, cols) and !grid[r][c].isOpen and !grid[r][c].isFlagged)
                {
                    openCell(grid, r, c, openedCount);
                    enqueue(q, r, c);
                }
            }
        }
    }
}

void showMessage(const char* msg)
{
    gotoRowCol(status_row_num, 0);
    color(14);
    cout << "                                                                                                ";
    gotoRowCol(status_row_num, 0);
    cout << msg;
    color(7);
}

void toggleFlag(Cell** grid, int row, int col, int mines, int& currentFlagCount)
{
    if (grid[row][col].isOpen)
        return;

    if (grid[row][col].isFlagged)
    {
        grid[row][col].isFlagged = false;
        currentFlagCount--;
    }
    else if (currentFlagCount < mines)
    {
        grid[row][col].isFlagged = true;
        currentFlagCount++;
    }
    else
    {
        showMessage("Out of flags! Unflag a cell first.");
    }
}

bool checkWinCondition(int rows, int cols, int mines, int openedCount)
{
    int totalSafeCells = (rows * cols) - mines;
    return openedCount == totalSafeCells;
}

void getCellContentPos(int row, int col, int& r, int& c)
{
    r = grid_top + row * cell_height + 1;
    c = grid_leftt + col * cell_width + 2;
}

bool screenToCell(int rpos, int cpos, int rows, int cols, int& row, int& col)
{
    if (rpos < grid_top or cpos < grid_leftt)
        return false;

    row = (rpos - grid_top) / cell_height;
    col = (cpos - grid_leftt) / cell_width;

    return isValidCell(row, col, rows, cols);
}

void drawTitle(int totalWidth)
{
    const char* title = "MINESWEEPER";
    int len = 11;

    int startCol = (totalWidth - len) / 2 + title_space;
    if (startCol < 0) startCol = 0;

    gotoRowCol(row_title, startCol);
    color(15);
    cout << title;
    color(7);
}

void drawFullGrid(int rows, int cols)
{
    color(8);

    int gridRight = grid_leftt + cols * cell_width;
    int gridBottom = grid_top + rows * cell_height;

    for (int i = 0; i <= rows; i++)
    {
        int r = grid_top + i * cell_height;
        drawLine(r, grid_leftt, r, gridRight);
    }

    for (int j = 0; j <= cols; j++)
    {
        int c = grid_leftt + j * cell_width;
        drawLine(grid_top, c, gridBottom, c);
    }

    gotoRowCol(grid_top, grid_leftt);
    color(8);
    cout << (char)-37;
    color(7);
}

void refreshContent(Cell** grid, int rows, int cols)
{
    int currentColor = -1;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int neededColor;
            char sym;

            if (grid[r][c].isFlagged and !grid[r][c].isOpen)
            {
                neededColor = 12;
                sym = flag_icon;
            }
            else if (!grid[r][c].isOpen)
            {
                neededColor = 8;
                sym = '.';
            }
            else if (grid[r][c].isMine)
            {
                neededColor = 12;
                sym = '*';
            }
            else if (grid[r][c].nCount == 0)
            {
                neededColor = 7;
                sym = ' ';
            }
            else
            {
                neededColor = 11;
                sym = '0' + grid[r][c].nCount;
            }

            if (neededColor != currentColor)
            {
                color(neededColor);
                currentColor = neededColor;
            }

            int sr, sc;
            getCellContentPos(r, c, sr, sc);
            gotoRowCol(sr, sc);
            cout << sym;
        }
    }
    color(7);
}

void saveGame(Cell** grid, int rows, int cols, int mines, int openedCount, int currentFlagCount)
{
    ofstream out(SAVE_FILE, ios::binary);
    if (!out)
    {
        showMessage("Could not open save file!");
        return;
    }

    out.write((char*)(&rows), sizeof(int));
    out.write((char*)(&cols), sizeof(int));
    out.write((char*)(&mines), sizeof(int));
    out.write((char*)(&openedCount), sizeof(int));
    out.write((char*)(&currentFlagCount), sizeof(int));

    for (int r = 0; r < rows; r++)
        out.write((char*)(grid[r]), sizeof(Cell) * cols);

    out.close();
    showMessage("Game saved!");
}

Cell** loadGame(int& rows, int& cols, int& mines, int& openedCount, int& currentFlagCount)
{
    ifstream in(SAVE_FILE, ios::binary);
    if (!in)
    {
        showMessage("No save file found.");
        return nullptr;
    }

    in.read((char*)(&rows), sizeof(int));
    in.read((char*)(&cols), sizeof(int));
    in.read((char*)(&mines), sizeof(int));
    in.read((char*)(&openedCount), sizeof(int));
    in.read((char*)(&currentFlagCount), sizeof(int));

    Cell** grid = new Cell * [rows];
    for (int r = 0; r < rows; r++)
    {
        grid[r] = new Cell[cols];
        in.read((char*)(grid[r]), sizeof(Cell) * cols);
    }

    in.close();
    return grid;
}


const int buttons_row = 2;

const int mode_col = 0;
const int mode_width = 14;

const int save_col = mode_col + mode_width + 2;
const int save_width = 8;

void drawButtons(bool flagMode)
{
    gotoRowCol(buttons_row, mode_col);
    color(11);
    cout << (flagMode ? "[ MODE: FLAG ]" : "[ MODE: OPEN ]");

    gotoRowCol(buttons_row, save_col);
    color(10);
    cout << "[ SAVE ]";

    color(7);
}

bool isClickOnModeButton(int rpos, int cpos)
{
    return rpos == buttons_row and cpos >= mode_col and cpos < mode_col + mode_width;
}

bool isClickOnSaveButton(int rpos, int cpos)
{
    return rpos == buttons_row and cpos >= save_col and cpos < save_col + save_width;
}

void chooseDifficulty(int& rows, int& cols, int& mines)
{
    cout << "Choose a difficulty:\n";
    cout << "  E - Easy   (9x9,   10 mines)\n";
    cout << "  M - Medium (12x12, 20 mines)\n";
    cout << "  H - Hard   (16x16, 45 mines)\n";
    cout << "Enter choice: ";

    char level;
    cin >> level;

    if (level == 'M' or level == 'm')
    {
        rows = 12; cols = 12; mines = 20;
    }
    else if (level == 'H' or level == 'h')
    {
        rows = 16; cols = 16; mines = 45;
    }
    else
    {
        rows = 9; cols = 9; mines = 10;
    }
}

void runGameLoop(Cell** grid, int rows, int cols, int mines, int openedCount, int currentFlagCount)
{
    bool gameOver = false;
    bool win = false;
    bool flagMode = false;

    hideConsoleCursor();
    system("cls");

    drawTitle(cols * cell_width);
    drawButtons(flagMode);
    drawFullGrid(rows, cols);
    refreshContent(grid, rows, cols);
    showMessage("Click a cell to open it. Use the buttons above to flag or save.");

    while (!gameOver)
    {
        int rpos, cpos;
        getRowColbyLeftClick(rpos, cpos);

        if (isClickOnModeButton(rpos, cpos))
        {
            flagMode = !flagMode;
            drawButtons(flagMode);
            showMessage(flagMode ? "Flag mode: click a cell to flag/unflag it."
                : "Open mode: click a cell to open it.");
            continue;
        }

        if (isClickOnSaveButton(rpos, cpos))
        {
            saveGame(grid, rows, cols, mines, openedCount, currentFlagCount);
            continue;
        }

        int row, col;
        if (!screenToCell(rpos, cpos, rows, cols, row, col))
        {
            showMessage("That click was outside the grid - try again.");
            continue;
        }

        if (flagMode)
        {
            toggleFlag(grid, row, col, mines, currentFlagCount);
            refreshContent(grid, rows, cols);
            continue;
        }

        if (grid[row][col].isFlagged)
        {
            showMessage("That cell is flagged - switch to flag mode to unflag it.");
            continue;
        }

        if (grid[row][col].isMine)
        {
            grid[row][col].isOpen = true;
            gameOver = true;
            win = false;
        }
        else
        {
            cascadeOpenCells(grid, rows, cols, row, col, openedCount);

            if (checkWinCondition(rows, cols, mines, openedCount))
            {
                gameOver = true;
                win = true;
            }
            else
            {
                showMessage("Click a cell to open it. Use the buttons above to flag or save.");
            }
        }

        refreshContent(grid, rows, cols);
    }

    showMessage(win ? "You win! Well played." : "Boom! Game over.");
}

int main()
{
    srand(time(0));
    int rows, cols, mines, openedCount = 0, currentFlagCount = 0;
    Cell** grid;

    cout << "Type L to load your last saved game, or N to start a new one: ";
    char choice;
    cin >> choice;

    if (choice == 'L' or choice == 'l')
    {
        grid = loadGame(rows, cols, mines, openedCount, currentFlagCount);
        if (grid == nullptr)
        {
            chooseDifficulty(rows, cols, mines);
            grid = createGrid(rows, cols);
            placeMines(grid, rows, cols, mines);
        }
    }
    else
    {
        chooseDifficulty(rows, cols, mines);
        grid = createGrid(rows, cols);
        placeMines(grid, rows, cols, mines);
    }

    runGameLoop(grid, rows, cols, mines, openedCount, currentFlagCount);

    gotoRowCol(grid_top + rows * cell_height + 2, 0);
    cout << "Press any key to exit...";
    _getch();

    destroyGrid(grid, rows);
    return 0;
}
