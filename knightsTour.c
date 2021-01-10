#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

int xMove[8] = { -1, -2, -2, -1, 1, 2, 2, 1};     // The clockwise order of the movements to check for
int yMove[8] = { -2, -1, 1, 2, 2, 1, -1, -2};     // each knight movement
int max_squares = 0;                              // Maximum number of squares covered by Sonny
int thread_num = 0;
pthread_t tid[100000];
struct chessboard* dead_end_boards[100000] = { NULL };
pthread_mutex_t lock;
int current_dead = 0;

struct chessboard
{
  int m;                // size m of the board (m*n)
  int n;                // size n of the board (m*n)
  int x;                // optional x value
  char** board;         // the 2d array representing the board
  int current_move;     // the current move Sonny is on on this board
  int positionX;        // the current x position of Sonny
  int positionY;        // the current y position of Sonny
  int visited;          // the number of visited squares of the board
  int is_thread;        // a boolean balue, representing if the board is in a child thread
                        // i.e., if it needs to call exit_thread() when terminating (more than
                        // one move available) ... or return as normal (if there is only one move available)
};

/*********************************************************************************
newChessboard(): Creates a new chessboard given the passed parameters
*********************************************************************************/
struct chessboard* newChessboard(int m, int n, int x)
{
  struct chessboard *temp = calloc(1, sizeof(struct chessboard));   // Allocates space for the new board
  char** t = calloc(m, sizeof(char*));
  for(int i=0; i<m; i++)
  {
    t[i] = calloc(n, sizeof(char));
    for(int j=0; j<n; j++)  t[i][j] = '.';                          // Sets all of the values to '.'
  }

  temp->m = m;                                                      // Initializes all of the variables of the chessboard
  temp->n = n;                                                      // struct, including passed parameters
  temp->x = x;
  temp->board = t;
  temp->current_move = 0;
  temp->board[0][0] = 'S';
  temp->positionX = 0;
  temp->positionY = 0;
  temp->visited = 1;
  temp->is_thread = 0;

  return temp;
}

/*********************************************************************************
copyChessboard(): Takes a chessboard object, and copys it into a brand new
chessboard. Returns that new chessboard
*********************************************************************************/
struct chessboard* copyChessboard(struct chessboard* original)
{
  struct chessboard *new = calloc(1, sizeof(struct chessboard));    // Allocates space for the new board
  char** t = calloc(original->m, sizeof(char*));
  for(int i=0; i<original->m; i++)
  {
    t[i] = calloc(original->n, sizeof(char));
    for(int j=0; j<original->n; j++)
    {
      t[i][j] = original->board[i][j];                              // Sets all of the values in the original board
    }                                                               // to the new board
  }

  new->m = original->m;                                             // Sets all of the variables of the original board
  new->n = original->n;                                             // to the new board
  new->x = original->x;
  new->board = t;
  new->current_move = original->current_move;
  new->positionX = original->positionX;
  new->positionY = original->positionY;
  new->visited = original->visited;
  new->is_thread = original->is_thread;

  return new;
}

/*********************************************************************************
printboard(): Called to print a given chessboard, i.e. a dynamic 2D array of
char*, or a 3D array
*********************************************************************************/
void printBoard(struct chessboard* board, int thread)
{
  printf("THREAD %d: > ", thread);
  for(int i=0; i<board->m; i++)
  {
    if(i!=0) printf("THREAD %d:   ", thread);
    for(int j=0; j<board->n; j++)printf("%c", board->board[i][j]);
    printf("\n");
  }
}

/*********************************************************************************
isAllowed(): Checks to see if a given move is allowed, i.e. matches the given
bounds of the board, and if that position has not been visited yet
*********************************************************************************/
int isAllowed(int x, int y, struct chessboard* board)
{
  if(x >= 0 && x < board->m && y>=0 && y < board->n && board->board[x][y] != 'S') return 1;
  else return 0;
}

/*********************************************************************************
possibleMoves(): Determines the number of valid moves for a given position per
chess board. i.e. how many locations can Sonny move to??
*********************************************************************************/
int possibleMoves(struct chessboard* board)
{
  int currentX = board->positionX;                            // Obtain the current position of Sonny
  int currentY = board->positionY;
  int num_moves = 0;

  for(int i=0; i<8; i++){                                    // Loop through all of the possible moves
    if(isAllowed(currentX + xMove[i], currentY + yMove[i], board) == 1) num_moves +=1; // If the move is valid... add it to the counter
  }

  return num_moves;
}

/*********************************************************************************
recursiveRoutine(): Called when a new possible move is found, and is threaded
off from a parent thread. Allows for calling in the same thread, or spawning
a new thread
*********************************************************************************/
void * recursiveRoutine(void * arg)
{
  struct chessboard* parent = (struct chessboard*)arg;
  int m = parent->m;
  int n = parent->n;

  parent->current_move +=1;
  parent->board[parent->positionX][parent->positionY] = 'S';


  int moves_todo = possibleMoves(parent);
  //if(moves_todo != 0) moves_todo = 1;
  //printf("Possible moves %d\n", moves_todo);
  if(parent->visited > max_squares) max_squares = parent->visited;
  if(parent->visited == m*n)                                    // 1. A knights tour is found when all of the spaces have been visited
  {
    printf("THREAD %lu: Sonny found a full knight's tour!\n", pthread_self());
    // printBoard(parent);
    // printf("\n");
    int *moves= calloc(1, sizeof(int));
    *moves = parent->current_move;
    if(parent->is_thread == 1) pthread_exit((void *) moves);        // If this is a thread process, I need to exit the thread
    else return (void *) moves;                                     // Otherwise ... I can simply return
  }
  else if(moves_todo == 0 && parent->visited != m*n)            // 2. A dead-end is found when there are no available moves, and the board is not visited
  {
    printf("THREAD %lu: Dead end after move #%d\n", pthread_self(), parent->current_move);
    dead_end_boards[current_dead] = parent;
    current_dead +=1;

    int *moves= calloc(1, sizeof(int));
    *moves = parent->current_move;

    if(parent->is_thread == 1) pthread_exit((void *) moves);        // If this is a thread process, I need to exit the thread
    else return (void *) moves;                                     // Otherwise ... I can simply return
  }
  else if(moves_todo == 1 && parent->visited != m*n)            // 3. There is one move to perform, so no threads need to be created
  {
    for(int i=7; i>=0; i--){                                        // Loop through all of the possible moves
      if(isAllowed(parent->positionX + xMove[i], parent->positionY + yMove[i], parent) == 1)
      {
        parent->positionX = parent->positionX + xMove[i];
        parent->positionY = parent->positionY + yMove[i];
        parent->visited += 1;
        parent->is_thread = 0;
        break;
      }
    }
    int* moves = recursiveRoutine(parent);
    return (void*) moves;
  }
  else if(moves_todo > 1 && parent->visited != m*n)             // 4. There is more than one move to perform, so multiple threads are spawned
  {
    printf("THREAD %lu: %d moves possible after move #%d; creating threads...\n", pthread_self(), moves_todo, parent->current_move);
    int biggest_return = 0;
    for(int i=0; i<8; i++){                                              // Loop through all of the possible moves
      if(isAllowed(parent->positionX + xMove[i], parent->positionY + yMove[i], parent) == 1)
      {
        struct chessboard* child = copyChessboard(parent);
        child->positionX = parent->positionX + xMove[i];
        child->positionY = parent->positionY + yMove[i];
        child->visited += 1;
        child->is_thread = 1;

        pthread_create(&tid[thread_num], NULL, recursiveRoutine, child); // Spawns off a child process

        #ifdef NO_PARALLEL
        int* moves;
        pthread_join(tid[thread_num], (void**)&moves);
        if(*moves > biggest_return) biggest_return = *moves;              // As this iteration needs to return the largest ret. from all thread calls....
        printf("THREAD %ld: Thread [%ld] joined (returned %d)\n", pthread_self(), tid[thread_num], *moves);
        #endif
        thread_num += 1;
      }
    }

    int* max_moves = calloc(1, sizeof(int));
    *max_moves = biggest_return;
    return (void *) max_moves;
  }
  else
  {
    int* moves = 0;
    return (void *) moves;
  }
}

/*********************************************************************************
main(): Takes input from the command line, m, n, and x. Calls recursive functions
that solve the knights problem.
*********************************************************************************/
int main(int argc, char *argv[])
{
  setvbuf( stdout, NULL, _IONBF, 0 );

  if(argc < 3)                                      // Some error checking
  {
    fprintf(stderr, "ERROR: Invalid argument(s)\n");
    fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
    exit(EXIT_FAILURE);
  }
  int x = 0;
  int m = atoi(argv[1]);
  int n = atoi(argv[2]);

  if( m <= 2 || n <= 2)
  {
    fprintf(stderr, "ERROR: Invalid argument(s)\n");
    fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
    exit(EXIT_FAILURE);
  }

  if(argc == 4)
  {
      x = atoi(argv[3]);
      if(x > m * n || x < 0)
      {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> [<x>]\n");
        exit(EXIT_FAILURE);
      }
  }

  struct chessboard* parent = newChessboard( m, n, x );

  printf("THREAD %lu: Solving Sonny's knight's tour problem for a %dx%d board\n", pthread_self(), m, n);
  recursiveRoutine(parent);

  printf("THREAD %ld: Best solution(s) found visit %d squares (out of %d)\n",pthread_self(), max_squares, m*n);
  printf("THREAD %ld: Dead end boards:\n", pthread_self());
  for(int i = 0; i < current_dead; i++){
    if(dead_end_boards[i]->visited >= x) printBoard(dead_end_boards[i], pthread_self());
  }

  pthread_join(pthread_self(), NULL);
  pthread_mutex_lock(&lock);
  pthread_mutex_unlock(&lock);
  return EXIT_SUCCESS;
}
