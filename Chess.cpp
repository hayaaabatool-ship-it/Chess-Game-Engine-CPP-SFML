#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

const int TILE_SIZE = 80;
const int BOARD_SIZE = 64;

enum Piece { EMPTY = 0, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
enum Color { NONE = 0, WHITE, BLACK };

struct Square {
    Piece piece = EMPTY;
    Color color = NONE;
};

// Use a one-dimensional array to represent the chessboard
Square board[BOARD_SIZE];
sf::Texture textures[12];

int enPassantIdx = -1;
Color enPassantColor = NONE;

// Replace all 2D arrays with 1D arrays
bool whiteKingMoved = false;
bool blackKingMoved = false;
bool whiteLeftRookMoved = false;
bool whiteRightRookMoved = false;
bool blackLeftRookMoved = false;
bool blackRightRookMoved = false;

struct Move {
    int fromIdx;
    int toIdx;
};

std::vector<Move> legalMoves;

// Auxiliary function: index conversion
inline int getRow(int idx) { return idx / 8; }
inline int getCol(int idx) { return idx % 8; }
inline int getIdx(int row, int col) { return row * 8 + col; }

sf::Texture loadTexture(const std::string& path) {
    sf::Texture t;
    if (!t.loadFromFile(path)) {
        std::cerr << "Failed to load " << path << "\n";
    }
    return t;
}

sf::Texture& getTexture(Piece p, Color c) {
    int index = (c == BLACK ? 6 : 0) + (p - 1);
    if (index < 0 || index >= 12) index = 0;
    return textures[index];
}

void initializeBoard() {
    // Clear the chessboard
    for (int i = 0; i < BOARD_SIZE; i++) {
        board[i] = { EMPTY, NONE };
    }

    // Initialize Black's pawn (line 1)
    for (int col = 0; col < 8; col++) {
        board[getIdx(1, col)] = { PAWN, BLACK };
    }

    // Initialize White's pawn (line 6)
    for (int col = 0; col < 8; col++) {
        board[getIdx(6, col)] = { PAWN, WHITE };
    }

    // Initialize other chess pieces
    Piece order[8] = { ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };

    // Black(row 0)
    for (int col = 0; col < 8; col++) {
        board[getIdx(0, col)] = { order[col], BLACK };
    }

    // White (line 7)
    for (int col = 0; col < 8; col++) {
        board[getIdx(7, col)] = { order[col], WHITE };
    }

    enPassantIdx = -1;
    enPassantColor = NONE;

    whiteKingMoved = false;
    blackKingMoved = false;
    whiteLeftRookMoved = false;
    whiteRightRookMoved = false;
    blackLeftRookMoved = false;
    blackRightRookMoved = false;
}

bool pathClear(int fromIdx, int toIdx) {
    int fromRow = getRow(fromIdx);
    int fromCol = getCol(fromIdx);
    int toRow = getRow(toIdx);
    int toCol = getCol(toIdx);

    int rowStep = 0;
    int colStep = 0;

    if (toRow > fromRow) rowStep = 1;
    else if (toRow < fromRow) rowStep = -1;

    if (toCol > fromCol) colStep = 1;
    else if (toCol < fromCol) colStep = -1;

    int currentRow = fromRow + rowStep;
    int currentCol = fromCol + colStep;

    while (currentRow != toRow || currentCol != toCol) {
        if (board[getIdx(currentRow, currentCol)].piece != EMPTY) {
            return false;
        }
        currentRow += rowStep;
        currentCol += colStep;
    }

    return true;
}

bool kingInCheck(Color c) {
    // Find the king's index
    int kingIdx = -1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i].piece == KING && board[i].color == c) {
            kingIdx = i;
            break;
        }
    }
    if (kingIdx == -1) return false;

    int kingRow = getRow(kingIdx);
    int kingCol = getCol(kingIdx);

    // Check if all enemy pieces attack the king
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i].color != NONE && board[i].color != c) {
            Piece p = board[i].piece;
            Color attackerColor = board[i].color;

            int fromRow = getRow(i);
            int fromCol = getCol(i);
            int rowDiff = kingRow - fromRow;
            int colDiff = kingCol - fromCol;

            bool isAttack = false;

            switch (p) {
            case PAWN: {
                int dir = (attackerColor == WHITE) ? -1 : 1;
                if (abs(colDiff) == 1 && rowDiff == dir) isAttack = true;
                break;
            }
            case ROOK:
                if ((fromRow == kingRow || fromCol == kingCol) && pathClear(i, kingIdx)) {
                    isAttack = true;
                }
                break;
            case KNIGHT:
                if ((abs(rowDiff) == 2 && abs(colDiff) == 1) ||
                    (abs(rowDiff) == 1 && abs(colDiff) == 2)) {
                    isAttack = true;
                }
                break;
            case BISHOP:
                if (abs(rowDiff) == abs(colDiff) && pathClear(i, kingIdx)) {
                    isAttack = true;
                }
                break;
            case QUEEN:
                if (((fromRow == kingRow || fromCol == kingCol) && pathClear(i, kingIdx)) ||
                    (abs(rowDiff) == abs(colDiff) && pathClear(i, kingIdx))) {
                    isAttack = true;
                }
                break;
            case KING:
                if (abs(rowDiff) <= 1 && abs(colDiff) <= 1) {
                    isAttack = true;
                }
                break;
            default:
                break;
            }

            if (isAttack) return true;
        }
    }
    return false;
}

bool wouldCauseCheck(int fromIdx, int toIdx) {
    Square fromSquare = board[fromIdx];
    Square toSquare = board[toIdx];

    // Simulate movement
    board[toIdx] = fromSquare;
    board[fromIdx] = { EMPTY, NONE };

    bool check = kingInCheck(fromSquare.color);

    // Saving
    board[fromIdx] = fromSquare;
    board[toIdx] = toSquare;

    return check;
}

bool isMoveLegal(int fromIdx, int toIdx) {
    if (fromIdx < 0 || fromIdx >= BOARD_SIZE || toIdx < 0 || toIdx >= BOARD_SIZE) {
        return false;
    }

    Square fromSquare = board[fromIdx];
    Square toSquare = board[toIdx];

    if (fromSquare.piece == EMPTY || fromSquare.color == NONE) return false;
    if (toSquare.color == fromSquare.color) return false;

    int fromRow = getRow(fromIdx);
    int fromCol = getCol(fromIdx);
    int toRow = getRow(toIdx);
    int toCol = getCol(toIdx);

    int rowDiff = toRow - fromRow;
    int colDiff = toCol - fromCol;
    Piece p = fromSquare.piece;
    Color c = fromSquare.color;

    switch (p) {
    case PAWN: {
        int dir = (c == WHITE) ? -1 : 1;
        int startRow = (c == WHITE) ? 6 : 1;

        //Move forward one space
        if (colDiff == 0 && rowDiff == dir && toSquare.piece == EMPTY) return true;

        //Move forward two spaces (starting position)
        if (colDiff == 0 && rowDiff == 2 * dir && fromRow == startRow) {
            int middleIdx = getIdx(fromRow + dir, fromCol);
            if (board[middleIdx].piece == EMPTY && toSquare.piece == EMPTY) {
                return true;
            }
        }
        if (abs(colDiff) == 1 && rowDiff == dir &&
            toSquare.color != c && toSquare.color != NONE) {
            return true;
        }

        // Eat passing soldiers
        if (abs(colDiff) == 1 && rowDiff == dir &&
            toIdx == enPassantIdx && enPassantColor != c) {
            return true;
        }
        return false;
    }
    case ROOK:
        return (fromRow == toRow || fromCol == toCol) && pathClear(fromIdx, toIdx);
    case KNIGHT:
        return (abs(rowDiff) == 2 && abs(colDiff) == 1) ||
            (abs(rowDiff) == 1 && abs(colDiff) == 2);
    case BISHOP:
        return abs(rowDiff) == abs(colDiff) && pathClear(fromIdx, toIdx);
    case QUEEN:
        return ((fromRow == toRow || fromCol == toCol) ||
            abs(rowDiff) == abs(colDiff)) && pathClear(fromIdx, toIdx);
    case KING: {
        if (abs(rowDiff) <= 1 && abs(colDiff) <= 1) {
            return !wouldCauseCheck(fromIdx, toIdx);
        }

        // castling
        bool kingHasMoved = (c == WHITE) ? whiteKingMoved : blackKingMoved;
        bool leftRookMoved = (c == WHITE) ? whiteLeftRookMoved : blackLeftRookMoved;
        bool rightRookMoved = (c == WHITE) ? whiteRightRookMoved : blackRightRookMoved;

        if (!kingHasMoved && rowDiff == 0 && abs(colDiff) == 2) {
            if (colDiff > 0) { // Short Castling (King Wing)
                if (rightRookMoved) return false;
                int rookIdx = getIdx(fromRow, 7);
                if (!pathClear(fromIdx, rookIdx)) return false;

                // Check if the path is safe
                for (int col = fromCol; col <= fromCol + 2; col++) {
                    int checkIdx = getIdx(fromRow, col);
                    Square temp = board[checkIdx];
                    board[checkIdx] = { KING, c };
                    board[fromIdx] = { EMPTY, NONE };

                    if (kingInCheck(c)) {
                        board[checkIdx] = temp;
                        board[fromIdx] = { KING, c };
                        return false;
                    }

                    board[checkIdx] = temp;
                }

                board[fromIdx] = { KING, c };
                return true;
            }
            else { // Long castling 
                if (leftRookMoved) return false;
                int rookIdx = getIdx(fromRow, 0);
                if (!pathClear(fromIdx, rookIdx)) return false;

                //Check if the path is safe
                for (int col = fromCol; col >= fromCol - 2; col--) {
                    int checkIdx = getIdx(fromRow, col);
                    Square temp = board[checkIdx];
                    board[checkIdx] = { KING, c };
                    board[fromIdx] = { EMPTY, NONE };

                    if (kingInCheck(c)) {
                        board[checkIdx] = temp;
                        board[fromIdx] = { KING, c };
                        return false;
                    }

                    board[checkIdx] = temp;
                }

                board[fromIdx] = { KING, c };
                return true;
            }
        }
        return false;
    }
    default:
        return false;
    }
}

void computeLegalMoves(int fromIdx, Color turn) {
    legalMoves.clear();

    if (fromIdx < 0 || fromIdx >= BOARD_SIZE) return;
    if (board[fromIdx].color != turn) return;

    for (int toIdx = 0; toIdx < BOARD_SIZE; toIdx++) {
        if (isMoveLegal(fromIdx, toIdx) && !wouldCauseCheck(fromIdx, toIdx)) {
            legalMoves.push_back({ fromIdx, toIdx });
        }
    }
}

bool hasLegalMoves(Color turn) {
    for (int fromIdx = 0; fromIdx < BOARD_SIZE; fromIdx++) {
        if (board[fromIdx].color == turn) {
            for (int toIdx = 0; toIdx < BOARD_SIZE; toIdx++) {
                if (isMoveLegal(fromIdx, toIdx) && !wouldCauseCheck(fromIdx, toIdx)) {
                    return true;
                }
            }
        }
    }
    return false;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(640, 640), "Chess");
    srand(static_cast<unsigned int>(time(nullptr)));

    std::string files[12] = {
        "white_pawn.png", "white_rook.png", "white_knight.png",
        "white_bishop.png", "white_queen.png", "white_king.png",
        "black_pawn.png", "black_rook.png", "black_knight.png",
        "black_bishop.png", "black_queen.png", "black_king.png"
    };

    for (int i = 0; i < 12; i++) {
        textures[i] = loadTexture(files[i]);
    }

    initializeBoard();

    bool dragging = false;
    Square held;
    int heldIdx = -1;
    sf::Sprite sprite;
    Color turn = WHITE;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
            }

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
                int col = e.mouseButton.x / TILE_SIZE;
                int row = e.mouseButton.y / TILE_SIZE;

                if (row < 0 || row >= 8 || col < 0 || col >= 8) continue;

                int idx = getIdx(row, col);
                if (board[idx].color == turn) {
                    dragging = true;
                    held = board[idx];
                    heldIdx = idx;
                    computeLegalMoves(heldIdx, turn);
                    sprite.setTexture(getTexture(held.piece, held.color));
                    float scaleX = static_cast<float>(TILE_SIZE) / sprite.getTexture()->getSize().x;
                    float scaleY = static_cast<float>(TILE_SIZE) / sprite.getTexture()->getSize().y;
                    sprite.setScale(scaleX, scaleY);
                    board[idx] = { EMPTY, NONE };
                }
            }

            if (e.type == sf::Event::MouseButtonReleased && dragging) {
                int col = e.mouseButton.x / TILE_SIZE;
                int row = e.mouseButton.y / TILE_SIZE;

                if (row < 0 || row >= 8 || col < 0 || col >= 8) {
                    // Invalid position, put the piece back
                    board[heldIdx] = held;
                    dragging = false;
                    heldIdx = -1;
                    continue;
                }

                int targetIdx = getIdx(row, col);

                bool valid = false;
                for (auto& m : legalMoves) {
                    if (m.toIdx == targetIdx) {
                        valid = true;
                        break;
                    }
                }

                if (valid) {
                    enPassantIdx = -1;
                    enPassantColor = NONE;

                    int fromRow = getRow(heldIdx);
                    int fromCol = getCol(heldIdx);

                    // Dealing with passing soldiers
                    if (held.piece == PAWN) {
                        if (abs(row - fromRow) == 2) {
                            int epRow = (row + fromRow) / 2;
                            enPassantIdx = getIdx(epRow, col);
                            enPassantColor = held.color;
                        }

                        // Eat passing soldiers
                        if (col != fromCol && board[targetIdx].piece == EMPTY) {
                            int capRow = (held.color == WHITE) ? row + 1 : row - 1;
                            if (capRow >= 0 && capRow < 8) {
                                board[getIdx(capRow, col)] = { EMPTY, NONE };
                            }
                        }
                    }

                    // Dealing with castling of the king's rook
                    if (held.piece == KING && abs(col - fromCol) == 2) {
                        if (col > fromCol) { //short castling
                            int rookFromIdx = getIdx(row, 7);
                            int rookToIdx = getIdx(row, col - 1);
                            board[rookToIdx] = board[rookFromIdx];
                            board[rookFromIdx] = { EMPTY, NONE };

                            if (held.color == WHITE) whiteRightRookMoved = true;
                            else blackRightRookMoved = true;
                        }
                        else { // long castling
                            int rookFromIdx = getIdx(row, 0);
                            int rookToIdx = getIdx(row, col + 1);
                            board[rookToIdx] = board[rookFromIdx];
                            board[rookFromIdx] = { EMPTY, NONE };

                            if (held.color == WHITE) whiteLeftRookMoved = true;
                            else blackLeftRookMoved = true;
                        }
                    }

                    // Execute move
                    board[targetIdx] = held;

                    // Update mobile status
                    if (held.piece == KING) {
                        if (held.color == WHITE) whiteKingMoved = true;
                        else blackKingMoved = true;
                    }

                    if (held.piece == ROOK) {
                        if (fromCol == 0) {
                            if (held.color == WHITE) whiteLeftRookMoved = true;
                            else blackLeftRookMoved = true;
                        }
                        else if (fromCol == 7) {
                            if (held.color == WHITE) whiteRightRookMoved = true;
                            else blackRightRookMoved = true;
                        }
                    }

                    // pawn promotion
                    if (held.piece == PAWN && (row == 0 || row == 7)) {
                        board[targetIdx].piece = QUEEN;
                    }

                    //switch turns
                    turn = (turn == WHITE) ? BLACK : WHITE;

                    //check status
                    if (kingInCheck(turn)) {
                        std::cout << (turn == WHITE ? "White" : "Black") << " is in check!\n";
                    }
                    if (!hasLegalMoves(turn)) {
                        std::cout << (turn == WHITE ? "White" : "Black") << " has no legal moves!\n";
                    }
                }
                else {
                    // Invalid move, put back to original position
                    board[heldIdx] = held;
                }

                dragging = false;
                heldIdx = -1;
            }
        }

        window.clear();

        // Draw the chessboard
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                sf::RectangleShape sq(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                sq.setPosition(col * TILE_SIZE, row * TILE_SIZE);
                sq.setFillColor((row + col) % 2 ? sf::Color(150, 75, 0) : sf::Color::White);
                window.draw(sq);
            }
        }

        // Highlight legal moves
        if (dragging) {
            for (auto& m : legalMoves) {
                int r = getRow(m.toIdx);
                int c = getCol(m.toIdx);
                sf::RectangleShape highlight(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                highlight.setPosition(c * TILE_SIZE, r * TILE_SIZE);
                highlight.setFillColor(sf::Color(0, 255, 0, 100));
                window.draw(highlight);
            }
        }

        //Draw chess pieces
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (board[i].piece != EMPTY) {
                int row = getRow(i);
                int col = getCol(i);

                sf::Sprite s;
                s.setTexture(getTexture(board[i].piece, board[i].color));
                float scaleX = static_cast<float>(TILE_SIZE) / s.getTexture()->getSize().x;
                float scaleY = static_cast<float>(TILE_SIZE) / s.getTexture()->getSize().y;
                s.setScale(scaleX, scaleY);
                s.setPosition(col * TILE_SIZE, row * TILE_SIZE);
                window.draw(s);
            }
        }

        if (dragging) {
            auto m = sf::Mouse::getPosition(window);
            sprite.setPosition(m.x - TILE_SIZE / 2, m.y - TILE_SIZE / 2);
            window.draw(sprite);
        }

        window.display();
    }

    return 0;
}