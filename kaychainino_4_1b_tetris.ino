// KeyChainino 144 Tetris Game (12x12 matrix)
// created by Martin Klocke
// and help by Grok

#include <KeyChainino.h>

#define MATRIX_ROW 12
#define MATRIX_COL 12
#define BUTTON_A PD2  // Left movement (from Arkanoid)
#define BUTTON_B PD3  // Rotate (from Arkanoid)

// Game state
byte matrixState[MATRIX_ROW][MATRIX_COL]; // 0=off, 1=stack, 2=falling piece
struct Point {
  byte x, y;
};
Point piecePos;
byte currentPiece[4][4];
byte pieceType;
byte rotation;
unsigned long lastMove = 0;
unsigned long lastInput = 0;
unsigned long lastActivity = 0;
int dropInterval = 500; // Drop speed (ms)
byte score = 0; // Tracks completed lines
volatile bool gameStarted = false;
volatile bool gameOver = false;

// Timer1 reload value (set for ~81 ms period on an 8 MHz internal oscillator)
unsigned int timer1_count_ = 64900;

// Tetromino shapes (I, O, T, S, Z, J, L)
const byte tetrominoes[7][4][4][4] = {
  {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}},
  {{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}},
  {{{0,0,0,0},{0,1,1,1},{0,0,1,0},{0,0,0,0}}, {{0,0,1,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}}, {{0,0,1,0},{0,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,1},{0,0,1,0},{0,0,0,0}}},
  {{{0,0,0,0},{0,0,1,1},{0,1,1,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,1},{0,0,0,1},{0,0,0,0}}, {{0,0,0,0},{0,0,1,1},{0,1,1,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,1},{0,0,0,1},{0,0,0,0}}},
  {{{0,0,0,0},{0,1,1,0},{0,0,1,1},{0,0,0,0}}, {{0,0,0,1},{0,0,1,1},{0,0,1,0},{0,0,0,0}}, {{0,0,0,0},{0,1,1,0},{0,0,1,1},{0,0,0,0}}, {{0,0,0,1},{0,0,1,1},{0,0,1,0},{0,0,0,0}}},
  {{{0,0,0,0},{0,1,1,1},{0,0,0,1},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,1,1,0},{0,0,0,0}}, {{0,1,0,0},{0,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,1},{0,0,1,0},{0,0,1,0},{0,0,0,0}}},
  {{{0,0,0,0},{0,1,1,1},{0,1,0,0},{0,0,0,0}}, {{0,1,1,0},{0,0,1,0},{0,0,1,0},{0,0,0,0}}, {{0,0,0,1},{0,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,1},{0,0,0,0}}}
};

// Start screen bitmap 
const byte KeyChaininoFace[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Interrupt Service Routines (ISRs)

ISR(TIMER1_OVF_vect) {
  TIMSK1 &= ~(1 << TOIE1); // Disable Timer1 interrupts

  if (gameStarted && !gameOver) {
    // Drop piece periodically
  if (millis() - lastMove >= dropInterval) {
    if (!movePiece(0, 1)) {
      settlePiece();
      clearLines();
      if (!spawnPiece()) {
        gameOver = true;
      }
    }
    lastMove = millis();
    lastActivity = millis();
  }
  }
  TCNT1 = timer1_count_;
  TIMSK1 |= (1 << TOIE1); // Re-enable Timer1
}

// --- External Interrupts for Buttons ---
// These ISR functions are intentionally left empty so that they
// can be used solely to wake the device from sleep.
ISR(INT0_vect) {
  // Empty ISR for BUTTON_B (INT0)
}

ISR(INT1_vect) {
  // Empty ISR for BUTTON_A (INT1)
}


//------------------------------
//Functions
//------------------------------

void clearMatrix() {
  KC.clear();
  for (byte i = 0; i < MATRIX_ROW; i++) {
    for (byte j = 0; j < MATRIX_COL; j++) {
      matrixState[i][j] = 0;
    }
  }
}

bool spawnPiece() {
  pieceType = random(7);
  rotation = 0;
  piecePos = {4, 0}; // Spawn at top center
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      currentPiece[i][j] = tetrominoes[pieceType][rotation][i][j];
      if (currentPiece[i][j] && (piecePos.x + i >= MATRIX_COL || piecePos.y + j >= MATRIX_ROW || matrixState[piecePos.x + i][piecePos.y + j])) {
        return false; // Collision at spawn
      }
    }
  }
  return true;
}

bool movePiece(int dx, int dy) {
  Point newPos = {piecePos.x + dx, piecePos.y + dy};
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int newX = newPos.x + i;
        int newY = newPos.y + j;
        if (newX < 0 || newX >= MATRIX_COL || newY >= MATRIX_ROW || matrixState[newX][newY] == 1) {
          return false;
        }
      }
    }
  }
  piecePos = newPos;
  return true;
}

bool movePiece_new(int dx, int dy) {
  Point newPos = {piecePos.x + dx, piecePos.y + dy};
  
  // Find the leftmost active block in the piece
  int minI = 4; // Initialize to max possible
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        if (i < minI) minI = i;
      }
    }
  }
  if (minI == 4) return false; // No active blocks (shouldn’t happen)

  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int newX = newPos.x + i;
        int newY = newPos.y + j;
        // Ensure leftmost block can reach x = 0
        if (newX < 0 || newX >= MATRIX_COL || newY < 0 || newY >= MATRIX_ROW || matrixState[newX][newY] == 1) {
          return false;
        }
      }
    }
  }
  
  // Allow move if leftmost block can reach x = 0
  if (newPos.x + minI < 0 && dx < 0) {
    return false; // Prevent moving left if leftmost block would go beyond x = 0
  }
  
  piecePos = newPos;
  return true;
}

void rotatePiece() {
  byte newRotation = (rotation + 1) % 4;
  byte tempPiece[4][4];
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      tempPiece[i][j] = tetrominoes[pieceType][newRotation][i][j];
    }
  }
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (tempPiece[i][j]) {
        int newX = piecePos.x + i;
        int newY = piecePos.y + j;
        if (newX < 0 || newX >= MATRIX_COL || newY >= MATRIX_ROW || matrixState[newX][newY] == 1) {
          return;
        }
      }
    }
  }
  rotation = newRotation;
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      currentPiece[i][j] = tempPiece[i][j];
    }
  }
}

void settlePiece() {
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (currentPiece[i][j]) {
        int x = piecePos.x + i;
        int y = piecePos.y + j;
        matrixState[x][y] = 1;
        if (y == 0) { // Check if piece settles in top row
          gameOver = true;
        }
      }
    }
  }
}

void clearLines() {
  byte linesCleared = 0;
  for (byte y = 0; y < MATRIX_ROW; y++) {
    bool full = true;
    for (byte x = 0; x < MATRIX_COL; x++) {
      if (matrixState[x][y] == 0) {
        full = false;
        break;
      }
    }
    if (full) {
      linesCleared++;
      // Flash line
      for (byte t = 0; t < 3; t++) {
        for (byte x = 0; x < MATRIX_COL; x++) {
          KC.pixel(x, y, 0);
        }
        KC.display();
        delay(100);
        for (byte x = 0; x < MATRIX_COL; x++) {
          KC.pixel(x, y, 1);
        }
        KC.display();
        delay(100);
      }
      // Shift lines down
      for (byte yy = y; yy > 0; yy--) {
        for (byte x = 0; x < MATRIX_COL; x++) {
          matrixState[x][yy] = matrixState[x][yy - 1];
        }
      }
      for (byte x = 0; x < MATRIX_COL; x++) {
        matrixState[x][0] = 0;
      }
    }
  }
  if (linesCleared > 0) {
    score += linesCleared;
    dropInterval = max(200, dropInterval - 50 * linesCleared);
    // Clear display and update with new matrix state
    KC.clear();
    for (byte x = 0; x < MATRIX_COL; x++) {
      for (byte y = 0; y < MATRIX_ROW; y++) {
        KC.pixel(x, y, matrixState[x][y] == 1 ? 1 : 0);
      }
    }
    KC.display();
  }
}

void updateDisplay() {
  // Clear falling piece
  for (byte i = 0; i < MATRIX_COL; i++) {
    for (byte j = 0; j < MATRIX_ROW; j++) {
      if (matrixState[i][j] == 2) {
        matrixState[i][j] = 0;
        KC.pixel(i, j, 0);
      }
    }
  }
  // Add falling piece (blinking)
  bool blink = (millis() / 250) % 2;
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 4; j++) {
      if (currentPiece[i][j] && piecePos.x + i < MATRIX_COL && piecePos.y + j < MATRIX_ROW) {
        matrixState[piecePos.x + i][piecePos.y + j] = blink ? 2 : 0;
        KC.pixel(piecePos.x + i, piecePos.y + j, blink);
      }
    }
  }
  // Update stack
  for (byte i = 0; i < MATRIX_COL; i++) {
    for (byte j = 0; j < MATRIX_ROW; j++) {
      if (matrixState[i][j] == 1) {
        KC.pixel(i, j, 1);
      }
    }
  }
}

void resetGame() {
  KC.clear();
  clearMatrix();
  spawnPiece();
  
  lastActivity = millis();
  KC.drawBitmap(KeyChaininoFace);
  KC.display();
  delay(2000);
  KC.clear();
  //KC.scrollText("1TETRIS", 1);
  //delay(2000);
  //KC.display();
  score = 0;
  dropInterval = 500;
  gameStarted = true; // or true?
  gameOver = false;
}

void Game() {
  if (gameOver) return; // Exit if game is over
// Handle buttons (Arkanoid-style debouncing)
if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {
    delay(80); // software debounce
    if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
      rotatePiece(); // Rotate clockwise
      lastInput = millis();
      lastActivity = millis();
    } else if (!digitalRead(BUTTON_A)) {
        movePiece(1, 0); // Move left
        lastInput = millis();
        lastActivity = millis();
    } else if (!digitalRead(BUTTON_B)) {
      movePiece(-1, 0); // Move right
      lastInput = millis();
      lastActivity = millis();
    }
 }
  
  // Update display
  updateDisplay();
  KC.display();
}

void endGame() {
  KC.clear();
    char scoreText[21];
    String str = "Game Over. Score: " + String(score);
    str.toCharArray(scoreText, 21);
    KC.scrollText(scoreText, 1);
    delay(5000);
    KC.clear();
    KC.goSleep();
    resetGame();
 }   
//--------------------------------
// Arduino Setup and Loop
//--------------------------------

void setup() {
  KC.init(); // Initialize KeyChainino
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  randomSeed(analogRead(A0));

  // Initialize game
  clearMatrix();
  gameStarted = false; // Wait for button press to start
  gameOver = false;
  lastActivity = millis();

  // Setup Timer1 (mimics Arkanoid)
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = timer1_count_; // Same as Arkanoid for consistent timing
  TCCR1B |= (1 << CS10); // Prescaler 1024
  TIMSK1 |= (1 << TOIE1);
  interrupts();
  // Enable external interrupts for the buttons.
  // (Their ISR functions are empty so they only help wake the MCU from sleep.)
  EIMSK |= (1 << INT0) | (1 << INT1);
  // Optionally, go to sleep at startup.
  KC.goSleep();
  resetGame();
}

void loop() {
  if (!gameStarted) {
    //Wait for button press to start the game
    if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {
      delay(80); // Debounce
     if (!digitalRead(BUTTON_A) || !digitalRead(BUTTON_B)) {
        gameStarted = true;
       resetGame(); // Reset game state
      }
    }
  } else if (gameOver) {
    endGame(); // Show game-over screen and reset
  } else {
    Game(); // Run the game
  }
}
