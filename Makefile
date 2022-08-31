all: ascii_game

ascii_game: ascii_game.cpp
	g++ -pthread -o ascii_game ascii_game.cpp -lncurses
