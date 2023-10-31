#include "chess.hpp"
#include <future>
#include <thread>
#include <cmath>
using namespace chess;

float eval(Board board, Movelist moves);
float pieceeval(Board board);
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
          board =
              Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
          std::async(mode, [board]() mutable {return start_negamax(board, 5);});
          // create a thread-local copy

        searching = true;
      } else if (command == "stop") {
        searching = false;
        std::cout << "bestmove " << uci::moveToUci(searchTask.get()) << std::endl;
      } else if (command == "quit") {
        running = false;
      } else {
        std::cout << "Error: invalid command" << std::endl;
        running = false;
      }
    }
    if (searching && searchTask.valid() &&
        searchTask.wait_for(zeroseconds) == std::future_status::ready) {
      std::cout << "bestmove " << uci::moveToUci(searchTask.get()) << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return 0;
}

float eval(Board board, Movelist moves) {
  board.makeNullMove();
  Movelist enemymoves;
  movegen::legalmoves(enemymoves, board);
  board.unmakeNullMove();
  float mymobility = moves.size();
  float enemymobility = enemymoves.size();
  float materialadvantage = pieceeval(board);
  return std::log2(materialadvantage * (mymobility / enemymobility));
}

float pieceeval(Board board) {
  uint whiteeval = 0;
  uint blackeval = 0;
  float materialEval;
  whiteeval += builtin::popcount(board.pieces(PieceType::PAWN, Color::WHITE));
  whiteeval += 3 * builtin::popcount(board.pieces(PieceType::KNIGHT, Color::WHITE) |
                                board.pieces(PieceType::BISHOP, Color::WHITE));
  whiteeval += 5 * builtin::popcount(board.pieces(PieceType::ROOK, Color::WHITE));
  whiteeval += 9 * builtin::popcount(board.pieces(PieceType::QUEEN, Color::WHITE));
  blackeval += builtin::popcount(board.pieces(PieceType::PAWN, Color::BLACK));
  blackeval += 3 * builtin::popcount(board.pieces(PieceType::KNIGHT, Color::BLACK) |
                                board.pieces(PieceType::BISHOP, Color::BLACK));
  blackeval += 5 * builtin::popcount(board.pieces(PieceType::ROOK, Color::BLACK));
  blackeval += 9 * builtin::popcount(board.pieces(PieceType::QUEEN, Color::BLACK));
  if (board.sideToMove() == Color::WHITE) {
    materialEval = (float)whiteeval / (float)blackeval;
  } else {
    materialEval = (float)blackeval / (float)whiteeval;
  }
  return materialEval;
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

  if (depth == 0) {
    return eval(board, moves);
  }

  if (moves.empty()) {
    if (board.inCheck()) {
      if (board.sideToMove() == Color::WHITE) {
        return -999;
      } else {
        return 999;
      }
    }
    return 0;
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