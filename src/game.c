#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "raylib.h"
#include "game.h"
#include "config.h"
#include "save_browser.h"

#define HUMAN_PLAYER 1
#define BOT_PLAYER 2

#define BOT_MAX_CANDIDATES 24
#define BOT_CANDIDATE_RADIUS 2
#define BOT_WIN_SCORE 10000000
#define BOT_INF_SCORE 100000000
#define PLAYER_NAME_MAX 48
#define SAVE_TIME_MAX 32
#define MAX_MOVE_HISTORY (BOARD_SIZE * BOARD_SIZE)
#define HINT_SEARCH_DEPTH 2

typedef struct CandidateMove
{
    int row;
    int col;
    int score;
} CandidateMove;

typedef struct GameMove
{
    int row;
    int col;
    int player;
} GameMove;

static int board[BOARD_SIZE][BOARD_SIZE];

static GameMode gameMode = GAME_MODE_PVP;
static BotDifficulty botDifficulty = BOT_DIFFICULTY_EASY;

static int currentPlayer = 1;
static int selectedRow = 0;
static int selectedCol = 0;

static int moveCountX = 0;
static int moveCountO = 0;

static int scoreX = 0;
static int scoreO = 0;

static char playerNameX[PLAYER_NAME_MAX] = "X";
static char playerNameO[PLAYER_NAME_MAX] = "O";

static int lastMoveRow = -1;
static int lastMoveCol = -1;

static GameMove moveHistory[MAX_MOVE_HISTORY];
static int moveHistoryCount = 0;
static int moveHistoryCursor = 0;

static int hintRow = -1;
static int hintCol = -1;

static int winner = 0;
// 0: chua ket thuc
// 1: X thang
// 2: O thang
// 3: hoa

static int winningRows[WIN_LENGTH];
static int winningCols[WIN_LENGTH];

static float gameOverTimer = 0.0f;
static int showGameOverPopup = 0;

static char statusMessage[128] = "";
static float statusTimer = 0.0f;

static float turnTimeRemaining = 30.0f;
static int cachedTurnTimerEnabled = -1;
static int cachedTurnTimeLimitSeconds = -1;

static int IsValidCell(int row, int col)
{
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

static int GetOtherPlayer(int player);

static void SetStatusMessage(const char* message)
{
    strcpy(statusMessage, message);
    statusTimer = 2.0f;
}

static int IsBlankText(const char* text)
{
    int i;

    if (text == NULL) return 1;

    for (i = 0; text[i] != '\0'; i++)
    {
        if (text[i] != ' ' && text[i] != '\t' && text[i] != '\r' && text[i] != '\n')
        {
            return 0;
        }
    }

    return 1;
}

static void CopyPlayerName(char* destination, const char* source, const char* defaultName)
{
    if (IsBlankText(source))
    {
        strncpy(destination, defaultName, PLAYER_NAME_MAX - 1);
    }
    else
    {
        strncpy(destination, source, PLAYER_NAME_MAX - 1);
    }

    destination[PLAYER_NAME_MAX - 1] = '\0';
}

static void ResetPlayerNames(void)
{
    CopyPlayerName(playerNameX, "X", "X");
    CopyPlayerName(playerNameO, "O", "O");
}

static const char* GetPlayerName(int player)
{
    if (gameMode == GAME_MODE_BOT)
    {
        return player == HUMAN_PLAYER ? "Bạn" : "Bot";
    }

    return player == HUMAN_PLAYER ? playerNameX : playerNameO;
}

static const char* GetPlayerScoreLabel(int player)
{
    if (gameMode == GAME_MODE_PVP)
    {
        return GetPlayerName(player);
    }

    return player == HUMAN_PLAYER ? "X" : "O";
}

static int IsBoardFull(void)
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (board[row][col] == 0)
            {
                return 0;
            }
        }
    }

    return 1;
}

static int GetCellFromMouse(Vector2 mouse, int* row, int* col)
{
    if (mouse.x < BOARD_X || mouse.x >= BOARD_X + BOARD_PIXEL_SIZE) return 0;
    if (mouse.y < BOARD_Y || mouse.y >= BOARD_Y + BOARD_PIXEL_SIZE) return 0;

    *col = (int)((mouse.x - BOARD_X) / CELL_SIZE);
    *row = (int)((mouse.y - BOARD_Y) / CELL_SIZE);

    return IsValidCell(*row, *col);
}

static void ClearWinningCells(void)
{
    int i;

    for (i = 0; i < WIN_LENGTH; i++)
    {
        winningRows[i] = -1;
        winningCols[i] = -1;
    }
}

static int IsWinningCell(int row, int col)
{
    int i;

    for (i = 0; i < WIN_LENGTH; i++)
    {
        if (winningRows[i] == row && winningCols[i] == col)
        {
            return 1;
        }
    }

    return 0;
}

static int CheckOneDirection(int row, int col, int dRow, int dCol, int player)
{
    int tempRows[BOARD_SIZE * 2];
    int tempCols[BOARD_SIZE * 2];
    int count = 0;
    int i;

    int r = row;
    int c = col;

    while (IsValidCell(r - dRow, c - dCol) && board[r - dRow][c - dCol] == player)
    {
        r -= dRow;
        c -= dCol;
    }

    while (IsValidCell(r, c) && board[r][c] == player)
    {
        tempRows[count] = r;
        tempCols[count] = c;
        count++;

        r += dRow;
        c += dCol;
    }

    if (count >= WIN_LENGTH)
    {
        for (i = 0; i < WIN_LENGTH; i++)
        {
            winningRows[i] = tempRows[i];
            winningCols[i] = tempCols[i];
        }

        return 1;
    }

    return 0;
}

static int CheckWinAtCell(int row, int col, int player)
{
    ClearWinningCells();

    if (CheckOneDirection(row, col, 0, 1, player)) return 1;   // ngang
    if (CheckOneDirection(row, col, 1, 0, player)) return 1;   // doc
    if (CheckOneDirection(row, col, 1, 1, player)) return 1;   // cheo chinh
    if (CheckOneDirection(row, col, 1, -1, player)) return 1;  // cheo phu

    return 0;
}

static void ClearHint(void)
{
    hintRow = -1;
    hintCol = -1;
}

static int IsHintCell(int row, int col)
{
    return row == hintRow && col == hintCol;
}

static void ClearMoveHistory(void)
{
    moveHistoryCount = 0;
    moveHistoryCursor = 0;
}

static void TruncateRedoHistory(void)
{
    if (moveHistoryCursor < moveHistoryCount)
    {
        moveHistoryCount = moveHistoryCursor;
    }
}

static void PushMoveToHistory(int row, int col, int player)
{
    if (moveHistoryCursor >= MAX_MOVE_HISTORY) return;

    TruncateRedoHistory();

    moveHistory[moveHistoryCursor].row = row;
    moveHistory[moveHistoryCursor].col = col;
    moveHistory[moveHistoryCursor].player = player;
    moveHistoryCursor++;
    moveHistoryCount = moveHistoryCursor;
}

static void UpdateLastMoveFromHistory(void)
{
    if (moveHistoryCursor <= 0)
    {
        lastMoveRow = -1;
        lastMoveCol = -1;
        return;
    }

    lastMoveRow = moveHistory[moveHistoryCursor - 1].row;
    lastMoveCol = moveHistory[moveHistoryCursor - 1].col;
}

static void IncrementMoveCountForPlayer(int player)
{
    if (player == HUMAN_PLAYER)
    {
        moveCountX++;
    }
    else if (player == BOT_PLAYER)
    {
        moveCountO++;
    }
}

static void DecrementMoveCountForPlayer(int player)
{
    if (player == HUMAN_PLAYER && moveCountX > 0)
    {
        moveCountX--;
    }
    else if (player == BOT_PLAYER && moveCountO > 0)
    {
        moveCountO--;
    }
}

static void ResetRoundEndState(void)
{
    winner = 0;
    gameOverTimer = 0.0f;
    showGameOverPopup = 0;
    ClearWinningCells();
}

static void PlayRoundEndSound(void)
{
    if (gameMode == GAME_MODE_BOT)
    {
        if (winner == BOT_PLAYER)
        {
            PlayGameSoundEffect(GAME_SOUND_GAME_OVER);
        }
        else if (winner == HUMAN_PLAYER)
        {
            PlayGameSoundEffect(GAME_SOUND_WIN);
        }

        return;
    }

    PlayGameSoundEffect(GAME_SOUND_WIN);
}

static void SyncTurnTimerSettings(void)
{
    int enabled = IsTurnTimerEnabled();
    int limit = GetTurnTimeLimitSeconds();

    if (limit < 1) limit = 1;

    if (cachedTurnTimerEnabled != enabled ||
        cachedTurnTimeLimitSeconds != limit)
    {
        cachedTurnTimerEnabled = enabled;
        cachedTurnTimeLimitSeconds = limit;
        turnTimeRemaining = (float)limit;
    }
}

static void ResetTurnTimer(void)
{
    SyncTurnTimerSettings();
    turnTimeRemaining = (float)GetTurnTimeLimitSeconds();
}

static void SkipTurnByTimeout(void)
{
    int skippedPlayer = currentPlayer;

    currentPlayer = GetOtherPlayer(currentPlayer);
    ClearHint();
    ResetTurnTimer();
    SetStatusMessage(TextFormat("%s hết giờ, bỏ lượt.", GetPlayerName(skippedPlayer)));
}

static int UpdateTurnTimer(void)
{
    SyncTurnTimerSettings();

    if (!IsTurnTimerEnabled()) return 0;
    if (winner != 0) return 0;
    if (gameMode == GAME_MODE_BOT && currentPlayer == BOT_PLAYER) return 0;

    turnTimeRemaining -= GetFrameTime();

    if (turnTimeRemaining <= 0.0f)
    {
        SkipTurnByTimeout();
        return 1;
    }

    return 0;
}

static int ApplyMoveToBoard(int row, int col, int player, AppScreen* currentScreen, int recordHistory)
{
    if (!IsValidCell(row, col)) return 0;
    if (board[row][col] != 0) return 0;
    if (winner != 0) return 0;
    if (player != HUMAN_PLAYER && player != BOT_PLAYER) return 0;

    board[row][col] = player;
    PlayGameSoundEffect(GAME_SOUND_CHECK);

    if (recordHistory)
    {
        PushMoveToHistory(row, col, player);
    }

    lastMoveRow = row;
    lastMoveCol = col;
    IncrementMoveCountForPlayer(player);

    if (CheckWinAtCell(row, col, player))
    {
        winner = player;

        if (winner == HUMAN_PLAYER) scoreX++;
        else if (winner == BOT_PLAYER) scoreO++;

        PlayRoundEndSound();
        gameOverTimer = 0.0f;
        showGameOverPopup = 1;
        if (currentScreen != NULL)
        {
            *currentScreen = SCREEN_GAME_OVER;
        }
        return 1;
    }

    if (IsBoardFull())
    {
        winner = 3;
        ClearWinningCells();
        PlayRoundEndSound();
        gameOverTimer = 0.0f;
        showGameOverPopup = 1;
        if (currentScreen != NULL)
        {
            *currentScreen = SCREEN_GAME_OVER;
        }
        return 1;
    }

    currentPlayer = GetOtherPlayer(player);
    ResetTurnTimer();
    return 1;
}

static void TryPlacePiece(int row, int col, AppScreen* currentScreen)
{
    if (ApplyMoveToBoard(row, col, currentPlayer, currentScreen, 1))
    {
        ClearHint();
    }
}

static int IsBotTurn(void)
{
    return gameMode == GAME_MODE_BOT &&
           currentPlayer == BOT_PLAYER &&
           winner == 0;
}

static int GetBotSearchDepth(void)
{
    if (botDifficulty == BOT_DIFFICULTY_HARD) return 3;
    if (botDifficulty == BOT_DIFFICULTY_MEDIUM) return 2;

    return 1;
}

static const char* GetGameModeName(void)
{
    return gameMode == GAME_MODE_BOT ? "Chơi với máy" : "Chơi với người";
}

static const char* GetBotDifficultyName(void)
{
    if (botDifficulty == BOT_DIFFICULTY_HARD) return "Khó";
    if (botDifficulty == BOT_DIFFICULTY_MEDIUM) return "Vừa";

    return "Dễ";
}

static int GetOtherPlayer(int player)
{
    return player == HUMAN_PLAYER ? BOT_PLAYER : HUMAN_PLAYER;
}

static int HasAnyPiece(void)
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (board[row][col] != 0)
            {
                return 1;
            }
        }
    }

    return 0;
}

static int HasWinAtCellNoHighlight(int row, int col, int player)
{
    int directions[4][2] = {
        { 0, 1 },
        { 1, 0 },
        { 1, 1 },
        { 1, -1 }
    };
    int i;

    for (i = 0; i < 4; i++)
    {
        int dRow = directions[i][0];
        int dCol = directions[i][1];
        int count = 1;
        int r = row + dRow;
        int c = col + dCol;

        while (IsValidCell(r, c) && board[r][c] == player)
        {
            count++;
            r += dRow;
            c += dCol;
        }

        r = row - dRow;
        c = col - dCol;

        while (IsValidCell(r, c) && board[r][c] == player)
        {
            count++;
            r -= dRow;
            c -= dCol;
        }

        if (count >= WIN_LENGTH)
        {
            return 1;
        }
    }

    return 0;
}

static int ScoreRun(int length, int openEnds)
{
    if (length >= WIN_LENGTH) return 1000000;

    if (length == 4)
    {
        if (openEnds >= 2) return 120000;
        if (openEnds == 1) return 25000;
        return 4000;
    }

    if (length == 3)
    {
        if (openEnds >= 2) return 8000;
        if (openEnds == 1) return 1500;
        return 200;
    }

    if (length == 2)
    {
        if (openEnds >= 2) return 650;
        if (openEnds == 1) return 120;
        return 20;
    }

    if (openEnds >= 2) return 35;
    if (openEnds == 1) return 10;

    return 1;
}

static int ScorePointForPlayer(int row, int col, int player)
{
    int directions[4][2] = {
        { 0, 1 },
        { 1, 0 },
        { 1, 1 },
        { 1, -1 }
    };
    int score = 0;
    int i;

    for (i = 0; i < 4; i++)
    {
        int dRow = directions[i][0];
        int dCol = directions[i][1];
        int length = 1;
        int openEnds = 0;
        int r = row + dRow;
        int c = col + dCol;

        while (IsValidCell(r, c) && board[r][c] == player)
        {
            length++;
            r += dRow;
            c += dCol;
        }

        if (IsValidCell(r, c) && board[r][c] == 0)
        {
            openEnds++;
        }

        r = row - dRow;
        c = col - dCol;

        while (IsValidCell(r, c) && board[r][c] == player)
        {
            length++;
            r -= dRow;
            c -= dCol;
        }

        if (IsValidCell(r, c) && board[r][c] == 0)
        {
            openEnds++;
        }

        score += ScoreRun(length, openEnds);
    }

    return score;
}

static int ScoreCountInWindow(int count)
{
    if (count >= 5) return BOT_WIN_SCORE;
    if (count == 4) return 120000;
    if (count == 3) return 7000;
    if (count == 2) return 500;
    if (count == 1) return 25;

    return 0;
}

static int ScoreWindowForCounts(int playerCount, int opponentCount)
{
    if (playerCount > 0 && opponentCount > 0) return 0;
    if (playerCount > 0) return ScoreCountInWindow(playerCount);
    if (opponentCount > 0) return -ScoreCountInWindow(opponentCount) - opponentCount * 5;

    return 0;
}

static int EvaluateBoardForPlayer(int player)
{
    int opponent = GetOtherPlayer(player);
    int score = 0;
    int row, col, i;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col <= BOARD_SIZE - WIN_LENGTH; col++)
        {
            int playerCount = 0;
            int opponentCount = 0;

            for (i = 0; i < WIN_LENGTH; i++)
            {
                if (board[row][col + i] == player) playerCount++;
                else if (board[row][col + i] == opponent) opponentCount++;
            }

            score += ScoreWindowForCounts(playerCount, opponentCount);
        }
    }

    for (col = 0; col < BOARD_SIZE; col++)
    {
        for (row = 0; row <= BOARD_SIZE - WIN_LENGTH; row++)
        {
            int playerCount = 0;
            int opponentCount = 0;

            for (i = 0; i < WIN_LENGTH; i++)
            {
                if (board[row + i][col] == player) playerCount++;
                else if (board[row + i][col] == opponent) opponentCount++;
            }

            score += ScoreWindowForCounts(playerCount, opponentCount);
        }
    }

    for (row = 0; row <= BOARD_SIZE - WIN_LENGTH; row++)
    {
        for (col = 0; col <= BOARD_SIZE - WIN_LENGTH; col++)
        {
            int playerCount = 0;
            int opponentCount = 0;

            for (i = 0; i < WIN_LENGTH; i++)
            {
                if (board[row + i][col + i] == player) playerCount++;
                else if (board[row + i][col + i] == opponent) opponentCount++;
            }

            score += ScoreWindowForCounts(playerCount, opponentCount);
        }
    }

    for (row = 0; row <= BOARD_SIZE - WIN_LENGTH; row++)
    {
        for (col = WIN_LENGTH - 1; col < BOARD_SIZE; col++)
        {
            int playerCount = 0;
            int opponentCount = 0;

            for (i = 0; i < WIN_LENGTH; i++)
            {
                if (board[row + i][col - i] == player) playerCount++;
                else if (board[row + i][col - i] == opponent) opponentCount++;
            }

            score += ScoreWindowForCounts(playerCount, opponentCount);
        }
    }

    return score;
}

static int IsNearExistingPiece(int row, int col)
{
    int dRow, dCol;

    for (dRow = -BOT_CANDIDATE_RADIUS; dRow <= BOT_CANDIDATE_RADIUS; dRow++)
    {
        for (dCol = -BOT_CANDIDATE_RADIUS; dCol <= BOT_CANDIDATE_RADIUS; dCol++)
        {
            int nearRow = row + dRow;
            int nearCol = col + dCol;

            if (dRow == 0 && dCol == 0) continue;

            if (IsValidCell(nearRow, nearCol) && board[nearRow][nearCol] != 0)
            {
                return 1;
            }
        }
    }

    return 0;
}

static int CountNeighborPieces(int row, int col)
{
    int count = 0;
    int dRow, dCol;

    for (dRow = -BOT_CANDIDATE_RADIUS; dRow <= BOT_CANDIDATE_RADIUS; dRow++)
    {
        for (dCol = -BOT_CANDIDATE_RADIUS; dCol <= BOT_CANDIDATE_RADIUS; dCol++)
        {
            int nearRow = row + dRow;
            int nearCol = col + dCol;

            if (dRow == 0 && dCol == 0) continue;

            if (IsValidCell(nearRow, nearCol) && board[nearRow][nearCol] != 0)
            {
                count++;
            }
        }
    }

    return count;
}

static int GetCenterScore(int row, int col)
{
    int center = BOARD_SIZE / 2;
    int rowDistance = row > center ? row - center : center - row;
    int colDistance = col > center ? col - center : center - col;
    int distance = rowDistance + colDistance;

    return BOARD_SIZE - distance;
}

static int ScoreCandidate(int row, int col, int player)
{
    int opponent = GetOtherPlayer(player);
    int score = 0;

    board[row][col] = player;

    if (HasWinAtCellNoHighlight(row, col, player))
    {
        score += BOT_WIN_SCORE;
    }

    score += ScorePointForPlayer(row, col, player) * 3;
    board[row][col] = 0;

    board[row][col] = opponent;

    if (HasWinAtCellNoHighlight(row, col, opponent))
    {
        score += BOT_WIN_SCORE / 2;
    }

    score += ScorePointForPlayer(row, col, opponent) * 2;
    board[row][col] = 0;

    score += CountNeighborPieces(row, col) * 20;
    score += GetCenterScore(row, col);

    return score;
}

static void SortCandidates(CandidateMove* candidates, int count)
{
    int i, j;

    for (i = 0; i < count - 1; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            if (candidates[j].score > candidates[i].score)
            {
                CandidateMove temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }
}

static int GenerateCandidateMoves(int player, CandidateMove* outMoves, int maxMoves)
{
    CandidateMove allMoves[BOARD_SIZE * BOARD_SIZE];
    int allCount = 0;
    int outCount;
    int row, col, i;

    if (maxMoves <= 0) return 0;

    if (!HasAnyPiece())
    {
        outMoves[0].row = BOARD_SIZE / 2;
        outMoves[0].col = BOARD_SIZE / 2;
        outMoves[0].score = 0;
        return 1;
    }

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (board[row][col] == 0 && IsNearExistingPiece(row, col))
            {
                allMoves[allCount].row = row;
                allMoves[allCount].col = col;
                allMoves[allCount].score = ScoreCandidate(row, col, player);
                allCount++;
            }
        }
    }

    SortCandidates(allMoves, allCount);

    outCount = allCount;
    if (outCount > maxMoves) outCount = maxMoves;

    for (i = 0; i < outCount; i++)
    {
        outMoves[i] = allMoves[i];
    }

    return outCount;
}

static int FindImmediateWinningMove(int player, int* outRow, int* outCol)
{
    CandidateMove candidates[BOT_MAX_CANDIDATES];
    int count = GenerateCandidateMoves(player, candidates, BOT_MAX_CANDIDATES);
    int i;

    for (i = 0; i < count; i++)
    {
        int row = candidates[i].row;
        int col = candidates[i].col;

        board[row][col] = player;

        if (HasWinAtCellNoHighlight(row, col, player))
        {
            board[row][col] = 0;
            *outRow = row;
            *outCol = col;
            return 1;
        }

        board[row][col] = 0;
    }

    return 0;
}

static int MinimaxForPlayer(int depth, int alpha, int beta, int maximizing, int lastRow, int lastCol, int lastPlayer, int perspectivePlayer)
{
    CandidateMove candidates[BOT_MAX_CANDIDATES];
    int opponent = GetOtherPlayer(perspectivePlayer);
    int count;
    int i;

    if (IsValidCell(lastRow, lastCol) &&
        HasWinAtCellNoHighlight(lastRow, lastCol, lastPlayer))
    {
        if (lastPlayer == perspectivePlayer)
        {
            return BOT_WIN_SCORE + depth;
        }

        return -BOT_WIN_SCORE - depth;
    }

    if (depth <= 0 || IsBoardFull())
    {
        return EvaluateBoardForPlayer(perspectivePlayer);
    }

    if (maximizing)
    {
        int bestScore = -BOT_INF_SCORE;

        count = GenerateCandidateMoves(perspectivePlayer, candidates, BOT_MAX_CANDIDATES);
        if (count == 0) return EvaluateBoardForPlayer(perspectivePlayer);

        for (i = 0; i < count; i++)
        {
            int row = candidates[i].row;
            int col = candidates[i].col;
            int score;

            board[row][col] = perspectivePlayer;
            score = MinimaxForPlayer(depth - 1, alpha, beta, 0, row, col, perspectivePlayer, perspectivePlayer);
            board[row][col] = 0;

            if (score > bestScore) bestScore = score;
            if (bestScore > alpha) alpha = bestScore;

            if (beta <= alpha)
            {
                break;
            }
        }

        return bestScore;
    }
    else
    {
        int bestScore = BOT_INF_SCORE;

        count = GenerateCandidateMoves(opponent, candidates, BOT_MAX_CANDIDATES);
        if (count == 0) return EvaluateBoardForPlayer(perspectivePlayer);

        for (i = 0; i < count; i++)
        {
            int row = candidates[i].row;
            int col = candidates[i].col;
            int score;

            board[row][col] = opponent;
            score = MinimaxForPlayer(depth - 1, alpha, beta, 1, row, col, opponent, perspectivePlayer);
            board[row][col] = 0;

            if (score < bestScore) bestScore = score;
            if (bestScore < beta) beta = bestScore;

            if (beta <= alpha)
            {
                break;
            }
        }

        return bestScore;
    }
}

static int FindBestMoveForPlayer(int player, int depth, int* outRow, int* outCol)
{
    CandidateMove candidates[BOT_MAX_CANDIDATES];
    int opponent = GetOtherPlayer(player);
    int count;
    int bestScore = -BOT_INF_SCORE;
    int bestCandidateScore = -BOT_INF_SCORE;
    int i;

    if (depth < 1) depth = 1;

    if (FindImmediateWinningMove(player, outRow, outCol))
    {
        return 1;
    }

    if (FindImmediateWinningMove(opponent, outRow, outCol))
    {
        return 1;
    }

    count = GenerateCandidateMoves(player, candidates, BOT_MAX_CANDIDATES);
    if (count == 0) return 0;

    *outRow = candidates[0].row;
    *outCol = candidates[0].col;

    for (i = 0; i < count; i++)
    {
        int row = candidates[i].row;
        int col = candidates[i].col;
        int score;

        board[row][col] = player;
        score = MinimaxForPlayer(depth - 1, -BOT_INF_SCORE, BOT_INF_SCORE, 0, row, col, player, player);
        board[row][col] = 0;

        if (score > bestScore ||
            (score == bestScore && candidates[i].score > bestCandidateScore))
        {
            bestScore = score;
            bestCandidateScore = candidates[i].score;
            *outRow = row;
            *outCol = col;
        }
    }

    return 1;
}

static int FindBestBotMove(int* outRow, int* outCol)
{
    return FindBestMoveForPlayer(BOT_PLAYER, GetBotSearchDepth(), outRow, outCol);
}

static void MakeBotMove(AppScreen* currentScreen)
{
    int row = -1;
    int col = -1;

    if (!IsBotTurn()) return;

    if (FindBestBotMove(&row, &col) && IsValidCell(row, col) && board[row][col] == 0)
    {
        selectedRow = row;
        selectedCol = col;
        TryPlacePiece(row, col, currentScreen);
    }
}

static int GetUndoStepCount(void)
{
    if (moveHistoryCursor <= 0) return 0;

    if (gameMode == GAME_MODE_BOT &&
        moveHistoryCursor >= 2 &&
        moveHistory[moveHistoryCursor - 1].player == BOT_PLAYER &&
        moveHistory[moveHistoryCursor - 2].player == HUMAN_PLAYER)
    {
        return 2;
    }

    return 1;
}

static int GetRedoStepCount(void)
{
    if (moveHistoryCursor >= moveHistoryCount) return 0;

    if (gameMode == GAME_MODE_BOT &&
        currentPlayer == HUMAN_PLAYER &&
        moveHistoryCursor + 1 < moveHistoryCount &&
        moveHistory[moveHistoryCursor].player == HUMAN_PLAYER &&
        moveHistory[moveHistoryCursor + 1].player == BOT_PLAYER)
    {
        return 2;
    }

    return 1;
}

static void UndoMove(void)
{
    int steps = GetUndoStepCount();
    int i;

    if (steps <= 0)
    {
        SetStatusMessage("Không còn nước để hoàn tác.");
        return;
    }

    if (winner == HUMAN_PLAYER && scoreX > 0) scoreX--;
    else if (winner == BOT_PLAYER && scoreO > 0) scoreO--;

    ResetRoundEndState();
    ClearHint();

    for (i = 0; i < steps && moveHistoryCursor > 0; i++)
    {
        GameMove move = moveHistory[moveHistoryCursor - 1];

        board[move.row][move.col] = 0;
        DecrementMoveCountForPlayer(move.player);
        currentPlayer = move.player;
        selectedRow = move.row;
        selectedCol = move.col;
        moveHistoryCursor--;
    }

    UpdateLastMoveFromHistory();
    ResetTurnTimer();
    SetStatusMessage(steps == 2 ? "Đã hoàn tác lượt người và bot." : "Đã hoàn tác một nước.");
}

static void RedoMove(AppScreen* currentScreen)
{
    int steps = GetRedoStepCount();
    int applied = 0;
    int i;

    if (steps <= 0)
    {
        SetStatusMessage("Không còn nước để làm lại.");
        return;
    }

    ClearHint();

    for (i = 0; i < steps && moveHistoryCursor < moveHistoryCount; i++)
    {
        GameMove move = moveHistory[moveHistoryCursor];

        if (!ApplyMoveToBoard(move.row, move.col, move.player, currentScreen, 0))
        {
            break;
        }

        selectedRow = move.row;
        selectedCol = move.col;
        moveHistoryCursor++;
        applied++;

        if (winner != 0)
        {
            break;
        }
    }

    if (applied > 0)
    {
        SetStatusMessage(applied == 2 ? "Đã làm lại lượt người và bot." : "Đã làm lại một nước.");
    }
    else
    {
        SetStatusMessage("Không thể làm lại nước này.");
    }
}

static void ShowSuggestedMove(void)
{
    int row = -1;
    int col = -1;

    if (winner != 0)
    {
        SetStatusMessage("Ván đã kết thúc, không thể gợi ý.");
        return;
    }

    if (IsBotTurn())
    {
        SetStatusMessage("Đang tới lượt bot.");
        return;
    }

    if (FindBestMoveForPlayer(currentPlayer, HINT_SEARCH_DEPTH, &row, &col) &&
        IsValidCell(row, col) &&
        board[row][col] == 0)
    {
        hintRow = row;
        hintCol = col;
        selectedRow = row;
        selectedCol = col;
        SetStatusMessage(TextFormat("Gợi ý: hàng %d, cột %d.", row + 1, col + 1));
    }
    else
    {
        ClearHint();
        SetStatusMessage("Không tìm thấy nước gợi ý.");
    }
}

void InitGame(void)
{
    gameMode = GAME_MODE_PVP;
    botDifficulty = BOT_DIFFICULTY_EASY;

    ResetPlayerNames();
    ResetMatchScore();
    ResetGame();
}

void ResetMatchScore(void)
{
    scoreX = 0;
    scoreO = 0;
}

void StartNewGame(GameMode mode, BotDifficulty difficulty)
{
    StartNewGameWithNames(mode, difficulty, "X", "O");
}

void StartNewGameWithNames(GameMode mode, BotDifficulty difficulty, const char* nameX, const char* nameO)
{
    gameMode = mode;
    botDifficulty = difficulty;

    if (gameMode != GAME_MODE_BOT)
    {
        gameMode = GAME_MODE_PVP;
        botDifficulty = BOT_DIFFICULTY_EASY;
    }

    if (gameMode == GAME_MODE_PVP)
    {
        CopyPlayerName(playerNameX, nameX, "X");
        CopyPlayerName(playerNameO, nameO, "O");
    }
    else
    {
        ResetPlayerNames();
    }

    ResetMatchScore();
    ResetGame();
}

void ResetGame(void)
{
    int row, col;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            board[row][col] = 0;
        }
    }

    currentPlayer = 1;
    selectedRow = 0;
    selectedCol = 0;

    moveCountX = 0;
    moveCountO = 0;

    lastMoveRow = -1;
    lastMoveCol = -1;

    winner = 0;
    gameOverTimer = 0.0f;
    showGameOverPopup = 0;

    ClearMoveHistory();
    ClearHint();
    ResetTurnTimer();
    ClearWinningCells();
}

void UpdateGame(AppScreen* currentScreen)
{
    int ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    int shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    int redoPressed = IsKeyPressed(KEY_Y) || (ctrlDown && shiftDown && IsKeyPressed(KEY_Z));
    int undoPressed = IsKeyPressed(KEY_Z) && !redoPressed;

    if (statusTimer > 0.0f)
    {
        statusTimer -= GetFrameTime();

        if (statusTimer <= 0.0f)
        {
            statusMessage[0] = '\0';
        }
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
        *currentScreen = SCREEN_PAUSE;
        return;
    }

    if (IsKeyPressed(KEY_L))
    {
        OpenSaveBrowser(currentScreen, BROWSER_SAVE, SCREEN_GAME);
        return;
    }

    if (IsKeyPressed(KEY_T))
    {
        OpenSaveBrowser(currentScreen, BROWSER_LOAD, SCREEN_GAME);
        return;
    }

    if (undoPressed)
    {
        UndoMove();
        return;
    }

    if (redoPressed)
    {
        RedoMove(currentScreen);
        return;
    }

    if (IsKeyPressed(KEY_H))
    {
        ShowSuggestedMove();
        return;
    }

    if (UpdateTurnTimer())
    {
        return;
    }

    if (IsBotTurn())
    {
        MakeBotMove(currentScreen);
        return;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
    {
        if (selectedCol > 0) selectedCol--;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
    {
        if (selectedCol < BOARD_SIZE - 1) selectedCol++;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        if (selectedRow > 0) selectedRow--;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        if (selectedRow < BOARD_SIZE - 1) selectedRow++;
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        TryPlacePiece(selectedRow, selectedCol, currentScreen);
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        int row;
        int col;
        Vector2 mouse = GetVirtualMousePosition();

        if (GetCellFromMouse(mouse, &row, &col))
        {
            selectedRow = row;
            selectedCol = col;
            TryPlacePiece(row, col, currentScreen);
        }
    }
}

static Rectangle GetGameOverButtonRect(int index)
{
    float width = 190.0f;
    float height = 44.0f;
    float gap = 16.0f;
    float totalWidth = width * 3.0f + gap * 2.0f;
    float x = SCREEN_WIDTH / 2.0f - totalWidth / 2.0f + index * (width + gap);
    float y = SCREEN_HEIGHT / 2.0f + 4.0f;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static int IsGameOverButtonClicked(int index)
{
    Vector2 mouse = GetVirtualMousePosition();
    Rectangle button = GetGameOverButtonRect(index);

    return CheckCollisionPointRec(mouse, button) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static Rectangle GetPostGamePanelButtonRect(int index)
{
    float width = 260.0f;
    float height = 44.0f;
    float x = BOARD_X + BOARD_PIXEL_SIZE + 45.0f;
    float y = BOARD_Y + 350.0f + index * 56.0f;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static int IsPostGamePanelButtonClicked(int index)
{
    Vector2 mouse = GetVirtualMousePosition();
    Rectangle button = GetPostGamePanelButtonRect(index);

    return CheckCollisionPointRec(mouse, button) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void DrawPanelActionButton(Rectangle rect, const char* text)
{
    Vector2 mouse = GetVirtualMousePosition();
    int hover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRec(rect, hover ? SKYBLUE : LIGHTGRAY);
    DrawRectangleLinesEx(rect, 2, hover ? BLUE : DARKGRAY);
    DrawUIFont(UI_FONT_BUTTON,
               text,
               (int)(rect.x + rect.width / 2 - MeasureUIFont(UI_FONT_BUTTON, text, 20) / 2),
               (int)(rect.y + rect.height / 2 - 10),
               20,
               DARKBLUE);
}

void UpdateGameOver(AppScreen* currentScreen)
{
    int ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    int shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    int redoPressed = IsKeyPressed(KEY_Y) || (ctrlDown && shiftDown && IsKeyPressed(KEY_Z));
    int undoPressed = IsKeyPressed(KEY_Z) && !redoPressed;

    gameOverTimer += GetFrameTime();

    if (undoPressed)
    {
        UndoMove();
        *currentScreen = SCREEN_GAME;
        return;
    }

    if (redoPressed)
    {
        RedoMove(currentScreen);
        return;
    }

    if (showGameOverPopup)
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsGameOverButtonClicked(0))
        {
            showGameOverPopup = 0;
            return;
        }

        if (IsGameOverButtonClicked(1))
        {
            ResetGame();
            *currentScreen = SCREEN_GAME;
            return;
        }

        if (IsGameOverButtonClicked(2))
        {
            ResetMatchScore();
            *currentScreen = SCREEN_MENU;
            return;
        }
    }
    else
    {
        if (IsPostGamePanelButtonClicked(0))
        {
            ResetGame();
            *currentScreen = SCREEN_GAME;
            return;
        }

        if (IsPostGamePanelButtonClicked(1))
        {
            OpenSaveBrowser(currentScreen, BROWSER_SAVE, SCREEN_GAME_OVER);
            return;
        }

        if (IsPostGamePanelButtonClicked(2))
        {
            ResetMatchScore();
            *currentScreen = SCREEN_MENU;
            return;
        }
    }
}

static void DrawPieceX(int x, int y)
{
    Color color = RED;

    Vector2 p1 = { x + 9, y + 9 };
    Vector2 p2 = { x + CELL_SIZE - 9, y + CELL_SIZE - 9 };
    Vector2 p3 = { x + CELL_SIZE - 9, y + 9 };
    Vector2 p4 = { x + 9, y + CELL_SIZE - 9 };

    DrawLineEx(p1, p2, 4, color);
    DrawLineEx(p3, p4, 4, color);
}

static void DrawPieceO(int x, int y)
{
    int centerX = x + CELL_SIZE / 2;
    int centerY = y + CELL_SIZE / 2;
    int radius = CELL_SIZE / 2 - 9;

    DrawCircleLines(centerX, centerY, radius, BLUE);
    DrawCircleLines(centerX, centerY, radius - 1, BLUE);
    DrawCircleLines(centerX, centerY, radius + 1, BLUE);
}

static void DrawBoard(void)
{
    int row, col;

    DrawRectangle(
        BOARD_X - 8,
        BOARD_Y - 8,
        BOARD_PIXEL_SIZE + 16,
        BOARD_PIXEL_SIZE + 16,
        (Color){ 220, 205, 170, 255 }
    );

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            int x = BOARD_X + col * CELL_SIZE;
            int y = BOARD_Y + row * CELL_SIZE;

            Color cellColor = (Color){ 245, 235, 200, 255 };

            if (row == lastMoveRow && col == lastMoveCol)
            {
                cellColor = (Color){ 255, 230, 150, 255 };
            }

            if (winner == 0 && IsHintCell(row, col))
            {
                cellColor = (Color){ 200, 235, 255, 255 };
            }

            if (IsWinningCell(row, col))
            {
                cellColor = (Color){ 150, 255, 150, 255 };
            }

            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, cellColor);
            DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, DARKBROWN);

            if (board[row][col] == 1)
            {
                DrawPieceX(x, y);
            }
            else if (board[row][col] == 2)
            {
                DrawPieceO(x, y);
            }
        }
    }

    if (winner == 0)
    {
        int selectedX = BOARD_X + selectedCol * CELL_SIZE;
        int selectedY = BOARD_Y + selectedRow * CELL_SIZE;

        Rectangle selectedRect = {
            (float)selectedX,
            (float)selectedY,
            (float)CELL_SIZE,
            (float)CELL_SIZE
        };

        DrawRectangleLinesEx(selectedRect, 4, GREEN);
    }
}

static void DrawSidePanel(void)
{
    int panelX = BOARD_X + BOARD_PIXEL_SIZE + 45;
    int panelY = BOARD_Y - 25;
    int titleSize = 30;
    int sectionTitleSize = 26;
    int textSize = 18;
    int smallTextSize = 17;
    int timerEnabled = IsTurnTimerEnabled();

    int infoY = panelY + 60;
    int turnY = panelY + 122;
    int moveY = timerEnabled ? panelY + 182 : panelY + 156;
    int scoreY = timerEnabled ? panelY + 236 : panelY + 210;
    int sectionY = timerEnabled ? panelY + 302 : panelY + 280;
    int statusY = panelY + 545;

    DrawUIFont(UI_FONT_HEADING, "THÔNG TIN VÁN ĐẤU", panelX, panelY, titleSize, DARKBLUE);

    DrawText(TextFormat("Chế độ: %s", GetGameModeName()),
             panelX, infoY, textSize, DARKGRAY);

    if (gameMode == GAME_MODE_BOT)
    {
        DrawText(TextFormat("Độ khó: %s", GetBotDifficultyName()),
                 panelX, infoY + 26, textSize, DARKGRAY);
    }

    if (winner == 0)
    {
        if (gameMode == GAME_MODE_BOT)
        {
            DrawText(currentPlayer == HUMAN_PLAYER ? "Lượt của bạn: X" : "Lượt của bot: O",
                     panelX, turnY, textSize, DARKGRAY);
        }
        else
        {
            DrawText(TextFormat("Lượt hiện tại: %s", GetPlayerName(currentPlayer)),
                     panelX, turnY, textSize, DARKGRAY);
        }
    }
    else
    {
        DrawText("Ván đấu đã kết thúc", panelX, turnY, textSize, MAROON);
    }

    if (timerEnabled && winner == 0)
    {
        int secondsLeft = (int)ceilf(turnTimeRemaining);
        Color timerColor = secondsLeft <= 5 ? MAROON : DARKGRAY;

        if (secondsLeft < 0) secondsLeft = 0;

        DrawText(TextFormat("Thời gian: %d giây", secondsLeft),
                 panelX,
                 turnY + 28,
                 textSize,
                 timerColor);
    }

    DrawText(TextFormat("Số bước X: %d", moveCountX),
             panelX, moveY, textSize, RED);

    DrawText(TextFormat("Số bước O: %d", moveCountO),
             panelX, moveY + 26, textSize, BLUE);

    DrawText(TextFormat("%s thắng: %d", GetPlayerScoreLabel(HUMAN_PLAYER), scoreX),
             panelX, scoreY, textSize, RED);

    DrawText(TextFormat("%s thắng: %d", GetPlayerScoreLabel(BOT_PLAYER), scoreO),
             panelX, scoreY + 26, textSize, BLUE);

    if (winner == 0)
    {
        DrawUIFont(UI_FONT_HEADING, "Điều khiển:", panelX, sectionY, sectionTitleSize, DARKBLUE);
        DrawText("W/A/S/D hoặc mũi tên", panelX, sectionY + 40, smallTextSize, DARKGRAY);
        DrawText("Enter/Space/Click để đánh", panelX, sectionY + 66, smallTextSize, DARKGRAY);
        DrawText("L mở màn hình lưu", panelX, sectionY + 92, smallTextSize, DARKGRAY);
        DrawText("T mở màn hình tải", panelX, sectionY + 118, smallTextSize, DARKGRAY);
        DrawText("Z hoàn tác", panelX, sectionY + 144, smallTextSize, DARKGRAY);
        DrawText("Y làm lại", panelX, sectionY + 170, smallTextSize, DARKGRAY);
        DrawText("H gợi ý nước đi", panelX, sectionY + 196, smallTextSize, DARKGRAY);
        DrawText("ESC/P tạm dừng", panelX, sectionY + 222, smallTextSize, DARKGRAY);
    }
    else
    {
        if (showGameOverPopup)
        {
            DrawText("Kết quả ván đấu", panelX, sectionY + 40, smallTextSize, DARKGRAY);
        }
        else
        {
            DrawText("Đang xem lại bàn cờ", panelX, sectionY + 40, smallTextSize, DARKGRAY);
            DrawText("Ô thắng được tô màu xanh", panelX, sectionY + 66, smallTextSize, DARKGRAY);
            DrawPanelActionButton(GetPostGamePanelButtonRect(0), "Chơi mới");
            DrawPanelActionButton(GetPostGamePanelButtonRect(1), "Lưu ván");
            DrawPanelActionButton(GetPostGamePanelButtonRect(2), "Màn hình chính");
        }
    }

    if (statusMessage[0] != '\0')
    {
        DrawUIFont(UI_FONT_HEADING, "Thông báo:", panelX, statusY, 24, DARKBLUE);
        DrawText(statusMessage, panelX, statusY + 34, 16, MAROON);
    }
}

static void DrawGameOverOverlay(void)
{
    const char* message;
    Color messageColor;
    Vector2 mouse;
    const char* buttonTexts[3] = {
        "Xem lại",
        "Chơi mới",
        "Màn hình chính"
    };
    int i;

    int boxWidth = 680;
    int boxHeight = 275;
    int boxX = SCREEN_WIDTH / 2 - boxWidth / 2;
    int boxY = SCREEN_HEIGHT / 2 - boxHeight / 2;

    float scale = 1.0f + 0.05f * (float)sin(gameOverTimer * 5.0f);

    if (winner == 1)
    {
        if (gameMode == GAME_MODE_BOT)
        {
            message = "BẠN THẮNG!";
        }
        else
        {
            message = TextFormat("%s THẮNG!", GetPlayerName(HUMAN_PLAYER));
        }
        messageColor = RED;
    }
    else if (winner == 2)
    {
        if (gameMode == GAME_MODE_BOT)
        {
            message = "BOT THẮNG!";
        }
        else
        {
            message = TextFormat("%s THẮNG!", GetPlayerName(BOT_PLAYER));
        }
        messageColor = BLUE;
    }
    else
    {
        message = "HAI BÊN HÒA NHAU!";
        messageColor = DARKGRAY;
    }

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 90 });

    DrawRectangle(boxX, boxY, boxWidth, boxHeight, RAYWHITE);
    DrawRectangleLinesEx(
        (Rectangle){ (float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight },
        4,
        DARKBLUE
    );

    int fontSize = (int)(34 * scale);
    int textWidth = MeasureUIFont(UI_FONT_HEADING, message, fontSize);

    DrawUIFont(
        UI_FONT_HEADING,
        message,
        SCREEN_WIDTH / 2 - textWidth / 2,
        boxY + 45,
        fontSize,
        messageColor
    );

    mouse = GetVirtualMousePosition();

    for (i = 0; i < 3; i++)
    {
        Rectangle button = GetGameOverButtonRect(i);
        int hover = CheckCollisionPointRec(mouse, button);
        Color bg = hover ? SKYBLUE : LIGHTGRAY;
        Color border = hover ? BLUE : DARKGRAY;

        DrawRectangleRec(button, bg);
        DrawRectangleLinesEx(button, 2, border);
        DrawUIFont(UI_FONT_BUTTON,
                   buttonTexts[i],
                   (int)(button.x + button.width / 2 - MeasureUIFont(UI_FONT_BUTTON, buttonTexts[i], 20) / 2),
                   (int)(button.y + button.height / 2 - 10),
                   20,
                   DARKBLUE);
    }

}

static Rectangle GetPauseButtonRect(int index)
{
    float width = 300.0f;
    float height = 48.0f;
    float x = SCREEN_WIDTH / 2.0f - width / 2.0f;
    float y = 255.0f + index * 62.0f;

    Rectangle rect = { x, y, width, height };
    return rect;
}

static int IsPauseButtonClicked(Rectangle rect)
{
    Vector2 mouse = GetVirtualMousePosition();

    return CheckCollisionPointRec(mouse, rect) &&
           IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void DrawPauseButton(Rectangle rect, const char* text)
{
    Vector2 mouse = GetVirtualMousePosition();
    int hover = CheckCollisionPointRec(mouse, rect);

    Color bg = hover ? SKYBLUE : LIGHTGRAY;
    Color border = hover ? BLUE : DARKGRAY;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2, border);

    DrawUIFont(UI_FONT_BUTTON,
               text,
               (int)(rect.x + rect.width / 2 - MeasureUIFont(UI_FONT_BUTTON, text, 22) / 2),
               (int)(rect.y + rect.height / 2 - 11),
               22,
               DARKBLUE);
}

void UpdatePauseMenu(AppScreen* currentScreen)
{
    Rectangle resumeButton = GetPauseButtonRect(0);
    Rectangle saveButton = GetPauseButtonRect(1);
    Rectangle loadButton = GetPauseButtonRect(2);
    Rectangle menuButton = GetPauseButtonRect(3);
    Rectangle exitButton = GetPauseButtonRect(4);

    if (IsPauseButtonClicked(resumeButton))
    {
        *currentScreen = SCREEN_GAME;
        return;
    }

    if (IsPauseButtonClicked(saveButton))
    {
        OpenSaveBrowser(currentScreen, BROWSER_SAVE, SCREEN_PAUSE);
        return;
    }

    if (IsPauseButtonClicked(loadButton))
    {
        OpenSaveBrowser(currentScreen, BROWSER_LOAD, SCREEN_PAUSE);
        return;
    }

    if (IsPauseButtonClicked(menuButton))
    {
        ResetMatchScore();
        *currentScreen = SCREEN_MENU;
        return;
    }

    if (IsPauseButtonClicked(exitButton))
    {
        *currentScreen = SCREEN_EXIT;
        return;
    }
}

void DrawPauseMenu(void)
{
    int boxWidth = 500;
    int boxHeight = 545;
    int boxX = SCREEN_WIDTH / 2 - boxWidth / 2;
    int boxY = 80;
    const char* title = "TẠM DỪNG";

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 120 });

    DrawRectangle(boxX, boxY, boxWidth, boxHeight, RAYWHITE);
    DrawRectangleLinesEx(
        (Rectangle){ (float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight },
        4,
        DARKBLUE
    );

    DrawUIFont(UI_FONT_HEADING,
               title,
               SCREEN_WIDTH / 2 - MeasureUIFont(UI_FONT_HEADING, title, 46) / 2,
               boxY + 35,
               46,
               DARKBLUE);

    DrawPauseButton(GetPauseButtonRect(0), "Tiếp tục");
    DrawPauseButton(GetPauseButtonRect(1), "Lưu ván");
    DrawPauseButton(GetPauseButtonRect(2), "Tải ván");
    DrawPauseButton(GetPauseButtonRect(3), "Về màn hình chính");
    DrawPauseButton(GetPauseButtonRect(4), "Thoát trò chơi");
}

void DrawGame(void)
{
    DrawUIFont(UI_FONT_TITLE, "CỜ CARO", BOARD_X, 20, 52, DARKBLUE);

    DrawBoard();
    DrawSidePanel();

    if (winner != 0 && showGameOverPopup)
    {
        DrawGameOverOverlay();
    }
}

static void BuildCurrentSaveTime(char* outTime, int outSize)
{
    time_t now = time(NULL);
    struct tm* localTime = localtime(&now);

    if (localTime == NULL)
    {
        strncpy(outTime, "Không rõ", outSize - 1);
        outTime[outSize - 1] = '\0';
        return;
    }

    strftime(outTime, outSize, "%Y-%m-%d %H:%M", localTime);
}

int SaveGameToFile(const char* fileName)
{
    FILE* file = fopen(fileName, "w");
    char saveTime[SAVE_TIME_MAX];
    int row, col;

    if (file == NULL)
    {
        SetStatusMessage("Không thể lưu ván! Kiểm tra thư mục saves.");
        return 0;
    }

    BuildCurrentSaveTime(saveTime, sizeof(saveTime));

    fprintf(file, "CARO_SAVE_FILE_V4\n");
    fprintf(file, "%d\n", BOARD_SIZE);

    fprintf(file, "%d %d %d %d\n",
            currentPlayer,
            selectedRow,
            selectedCol,
            winner);

    fprintf(file, "%d %d %d\n",
            (int)gameMode,
            (int)botDifficulty,
            HUMAN_PLAYER);

    fprintf(file, "%s\n", playerNameX);
    fprintf(file, "%s\n", playerNameO);
    fprintf(file, "%s\n", saveTime);

    fprintf(file, "%d %d\n", moveCountX, moveCountO);
    fprintf(file, "%d %d\n", scoreX, scoreO);
    fprintf(file, "%d %d\n", lastMoveRow, lastMoveCol);
    fprintf(file, "%d %d\n", moveHistoryCursor, moveHistoryCount);

    for (row = 0; row < moveHistoryCount; row++)
    {
        fprintf(file,
                "%d %d %d\n",
                moveHistory[row].row,
                moveHistory[row].col,
                moveHistory[row].player);
    }

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            fprintf(file, "%d ", board[row][col]);
        }
        fprintf(file, "\n");
    }

    fclose(file);

    SetStatusMessage("Lưu ván thành công.");
    return 1;
}

int LoadGameFromFile(const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    char header[64];
    int savedBoardSize;
    int saveVersion = 1;
    int savedGameMode = GAME_MODE_PVP;
    int savedBotDifficulty = BOT_DIFFICULTY_EASY;
    int savedHumanPlayer = HUMAN_PLAYER;
    char savedPlayerNameX[PLAYER_NAME_MAX] = "X";
    char savedPlayerNameO[PLAYER_NAME_MAX] = "O";
    char savedTime[SAVE_TIME_MAX] = "";
    int row, col;
    int historyIndex;

    if (file == NULL)
    {
        SetStatusMessage("Không tìm thấy bản lưu!");
        return 0;
    }

    if (fscanf(file, "%63s", header) != 1)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị lỗi!");
        return 0;
    }

    if (strcmp(header, "CARO_SAVE_FILE_V4") == 0)
    {
        saveVersion = 4;
    }
    else if (strcmp(header, "CARO_SAVE_FILE_V3") == 0)
    {
        saveVersion = 3;
    }
    else if (strcmp(header, "CARO_SAVE_FILE_V2") == 0)
    {
        saveVersion = 2;
    }
    else if (strcmp(header, "CARO_SAVE_FILE") == 0)
    {
        saveVersion = 1;
    }
    else
    {
        fclose(file);
        SetStatusMessage("Bản lưu không hợp lệ!");
        return 0;
    }

    if (fscanf(file, "%d", &savedBoardSize) != 1)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị thiếu kích thước bàn cờ!");
        return 0;
    }

    if (savedBoardSize != BOARD_SIZE)
    {
        fclose(file);
        SetStatusMessage("Bản lưu khác kích thước bàn cờ hiện tại!");
        return 0;
    }

    if (fscanf(file, "%d %d %d %d",
               &currentPlayer,
               &selectedRow,
               &selectedCol,
               &winner) != 4)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị thiếu thông tin lượt chơi!");
        return 0;
    }

    if (saveVersion >= 2)
    {
        if (fscanf(file, "%d %d %d",
                   &savedGameMode,
                   &savedBotDifficulty,
                   &savedHumanPlayer) != 3)
        {
            fclose(file);
            SetStatusMessage("Bản lưu bị thiếu chế độ chơi!");
            return 0;
        }
    }

    if (saveVersion >= 3)
    {
        if (fscanf(file, " %47[^\n]", savedPlayerNameX) != 1 ||
            fscanf(file, " %47[^\n]", savedPlayerNameO) != 1 ||
            fscanf(file, " %31[^\n]", savedTime) != 1)
        {
            fclose(file);
            SetStatusMessage("Bản lưu bị thiếu thông tin người chơi!");
            return 0;
        }
    }

    if (fscanf(file, "%d %d", &moveCountX, &moveCountO) != 2)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị thiếu số bước!");
        return 0;
    }

    if (fscanf(file, "%d %d", &scoreX, &scoreO) != 2)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị thiếu điểm số!");
        return 0;
    }

    if (fscanf(file, "%d %d", &lastMoveRow, &lastMoveCol) != 2)
    {
        fclose(file);
        SetStatusMessage("Bản lưu bị thiếu ô vừa đánh!");
        return 0;
    }

    ClearMoveHistory();

    if (saveVersion >= 4)
    {
        if (fscanf(file, "%d %d", &moveHistoryCursor, &moveHistoryCount) != 2)
        {
            fclose(file);
            SetStatusMessage("Bản lưu bị thiếu lịch sử nước đi!");
            return 0;
        }

        if (moveHistoryCursor < 0 ||
            moveHistoryCount < 0 ||
            moveHistoryCursor > moveHistoryCount ||
            moveHistoryCount > MAX_MOVE_HISTORY)
        {
            fclose(file);
            ClearMoveHistory();
            SetStatusMessage("Lịch sử nước đi trong bản lưu không hợp lệ!");
            return 0;
        }

        for (historyIndex = 0; historyIndex < moveHistoryCount; historyIndex++)
        {
            if (fscanf(file,
                       "%d %d %d",
                       &moveHistory[historyIndex].row,
                       &moveHistory[historyIndex].col,
                       &moveHistory[historyIndex].player) != 3)
            {
                fclose(file);
                ClearMoveHistory();
                SetStatusMessage("Bản lưu bị thiếu chi tiết lịch sử nước đi!");
                return 0;
            }

            if (!IsValidCell(moveHistory[historyIndex].row, moveHistory[historyIndex].col) ||
                (moveHistory[historyIndex].player != HUMAN_PLAYER &&
                 moveHistory[historyIndex].player != BOT_PLAYER))
            {
                fclose(file);
                ClearMoveHistory();
                SetStatusMessage("Chi tiết lịch sử nước đi không hợp lệ!");
                return 0;
            }
        }
    }

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (fscanf(file, "%d", &board[row][col]) != 1)
            {
                fclose(file);
                SetStatusMessage("Bản lưu bị thiếu dữ liệu bàn cờ!");
                return 0;
            }
        }
    }

    fclose(file);

    if (!IsValidCell(selectedRow, selectedCol))
    {
        selectedRow = 0;
        selectedCol = 0;
    }

    if (currentPlayer != HUMAN_PLAYER && currentPlayer != BOT_PLAYER)
    {
        currentPlayer = HUMAN_PLAYER;
    }

    if (savedGameMode == GAME_MODE_BOT && savedHumanPlayer == HUMAN_PLAYER)
    {
        gameMode = GAME_MODE_BOT;
    }
    else
    {
        gameMode = GAME_MODE_PVP;
    }

    if (savedBotDifficulty == BOT_DIFFICULTY_EASY ||
        savedBotDifficulty == BOT_DIFFICULTY_MEDIUM ||
        savedBotDifficulty == BOT_DIFFICULTY_HARD)
    {
        botDifficulty = (BotDifficulty)savedBotDifficulty;
    }
    else
    {
        botDifficulty = BOT_DIFFICULTY_EASY;
    }

    if (gameMode != GAME_MODE_BOT)
    {
        botDifficulty = BOT_DIFFICULTY_EASY;
    }

    if (gameMode == GAME_MODE_PVP)
    {
        CopyPlayerName(playerNameX, savedPlayerNameX, "X");
        CopyPlayerName(playerNameO, savedPlayerNameO, "O");
    }
    else
    {
        ResetPlayerNames();
    }

    ClearWinningCells();
    ClearHint();

    if (saveVersion >= 4)
    {
        UpdateLastMoveFromHistory();
    }

    if (winner != 0)
    {
        winner = 0;
    }

    showGameOverPopup = 0;
    ResetTurnTimer();

    SetStatusMessage("Tải ván thành công.");
    return 1;
}
