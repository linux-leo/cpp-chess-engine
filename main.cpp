#include "chess.hpp"
#include <future>
#include <thread>
#include <cmath>
using namespace chess;

float eval(Board board, Movelist moves);
Move start_negamax(Board board, int depth);
int negamax(Board board, int depth, int alpha, int beta);

int main() {
  constexpr
  std::chrono::duration<int> zeroseconds = std::chrono::duration<int>::zero();
  std::future<void> ioTask;     // This will handle the async IO task
  std::future<Move> searchTask; // This will handle the async Search task
  std::string stdin;
  std::string command;
  std::launch mode = std::launch::async;

  Board board;
  bool running = true;
  bool prompting = true;
  bool searching = false;
  while (running) {
    if (prompting) { // True in the first iteration, thereafter only if the old
                     // IO Task finished
      ioTask = std::async(mode, [&]() { std::getline(std::cin, stdin); });
    }

    // Prepare starting a new IO Task in the next iteration if the old IO Task
    // finished
    prompting = (ioTask.wait_for(zeroseconds) == std::future_status::ready);

    // If the old IO Task finished, a command has been received that needs to be
    // handled
    if (prompting) {
      std::istringstream commandline(stdin);
      commandline >> command;
      if (command == "uci") {
        std::cout << "id name Leo" << std::endl;
        std::cout << "uciok" << std::endl;
      } else if (command == "isready") {
        std::cout << "readyok" << std::endl;
      } else if (command == "position") {
        commandline >> command;
        if (command == "startpos") {
          board = Board(constants::STARTPOS);
          commandline >> command;
          if (command == "moves") {
            while (commandline >> command) {
              board.makeMove(uci::uciToMove(board, command));
            }
          }
        } else {
          std::cout << "Error: Custom Positions Not Supported" << std::endl;
        }
      } else if (command == "ucinewgame") {
      } else if (command == "go") {
        searchTask =
          std::async(mode, [board]() mutable {return start_negamax(board, 7);});
          // create a thread-local copy

        searching = true;
      } else if (command == "stop") {
        searching = false;
        std::cout << "bestmove " << uci::moveToUci(searchTask.get())<<std::endl;
      } else if (command == "quit") {
        running = false;
      } else {
        std::cout << "Error: invalid command" << std::endl;
        running = false;
      }
    }
    if (searching && searchTask.valid() &&
        searchTask.wait_for(zeroseconds) == std::future_status::ready) {
      std::cout << "bestmove " << uci::moveToUci(searchTask.get())<<std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return 0;
}

float eval(Board board, Movelist legalmoves,  Movelist opponentmoves) {
  constexpr auto WHITE = Color::WHITE; // alias
  constexpr auto BLACK = Color::BLACK; // alias
  Bitboard wPawns = board.pieces(PieceType::PAWN, WHITE);
  Bitboard bPawns = board.pieces(PieceType::PAWN, BLACK);
  uint wMaterial = 1 * builtin::popcount(wPawns)
                 + 3 * builtin::popcount(board.pieces(PieceType::KNIGHT, WHITE)|
                                         board.pieces(PieceType::BISHOP, WHITE))
                 + 5 * builtin::popcount(board.pieces(PieceType::ROOK, WHITE))
                 + 9 * builtin::popcount(board.pieces(PieceType::QUEEN, WHITE));
  uint bMaterial = 1 * builtin::popcount(bPawns)
                 + 3 * builtin::popcount(board.pieces(PieceType::KNIGHT, BLACK)|
                                         board.pieces(PieceType::BISHOP, BLACK))
                 + 5 * builtin::popcount(board.pieces(PieceType::ROOK, BLACK))
                 + 9 * builtin::popcount(board.pieces(PieceType::QUEEN, BLACK));

  // Detect white doubled pawns
  float weval = -builtin::popcount(wPawns & (wPawns << 8))*.5f + wMaterial;

  // Detect black doubled pawns
  float beval = -builtin::popcount(bPawns & (bPawns << 8))*.5f + bMaterial;

  float eval = (board.sideToMove() == WHITE) ? weval/beval : beval/weval;

  float mobility = static_cast<float>(legalmoves.size()) / opponentmoves.size();

  return std::log2(eval * mobility);
}

Move start_negamax(Board board, int depth) {
  Movelist moves;
  movegen::legalmoves(moves, board);
  int bestEval = -999;
  Move bestMove = NULL;

  for (int i = 0; i < moves.size(); i++) {
    const Move move = moves[i];
    board.makeMove(move);
    int eval = -negamax(board, depth - 1, -999, 999);
    board.unmakeMove(move);
    if (eval > bestEval) {
      bestEval = eval;
      bestMove = move;
    }
  }

  return bestMove;
}

int negamax(Board board, int depth, int alpha, int beta) {
  Movelist moves;
  movegen::legalmoves(moves, board);

  if (moves.empty()) {
    if (board.inCheck()) {
      return -999;
    }
    return 0;
  }
  
  if (depth == 0) {
    board.makeNullMove();
    Movelist enemymoves;
    movegen::legalmoves(enemymoves, board);
    board.unmakeNullMove();
    return eval(board, moves, enemymoves);
  }

  for (int i = 0; i < moves.size(); i++) {
    const Move move = moves[i];
    board.makeMove(move);
    int eval = -negamax(board, depth - 1, -beta, -alpha);
    board.unmakeMove(move);
    if (eval >= beta) {
      return beta;
    }
    alpha = std::max(alpha, eval);
  }

  return alpha;
}