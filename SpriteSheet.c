/// \file SpriteSheet.c
/// \brief SDAGE - SDL2 Programming Assignment
/// A 2 player Nokia Snake based implementation, the players must eat all the pickups
/// and the winner is the one who eats the most.
/// If a snake collides with itself, the game will end.
///
/// \author Tom Minor
/// \date 13th January 2014 (submission date)
///

/// All extra images (snake.png, background.png, GameOver.png) were created by me
///


#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <time.h>

#include "actor.h"
#include "pickup.h"

#define SNAKE_RADIUS      (64)

#define BODY_OFFSET       (SNAKE_RADIUS*8)
#define BODY_ALT_OFFSET   (SNAKE_RADIUS*9)
#define BODY_EAT_OFFSET   (SNAKE_RADIUS)

#define PLAYER1_SCALE     (1)
#define PLAYER1_SPAWNX    (WIDTH/4)
#define PLAYER1_SPAWNY    (HEIGHT/4)
#define PLAYER1_SEGMENTS  (24)

#define PLAYER2_SCALE     (1)
#define PLAYER2_SPAWNX    (WIDTH/4)
#define PLAYER2_SPAWNY    (HEIGHT/2)
#define PLAYER2_SEGMENTS  (24)

const int WIDTH=800;
const int HEIGHT=600;

// Rendering
void renderBackground(SDL_Renderer *_renderer, SDL_Texture  *_tex);
void displayGameOver(SDL_Renderer *_renderer, SDL_Texture  *_tex, int _firstScore, int _secondScore);
void renderSnakeHead( Node *_head, SDL_Renderer * _renderer, SDL_Texture *_tex);
void renderSnakeBody( Node *_head, Node *_tail, SDL_Renderer *_renderer, SDL_Texture *_tex );

// Input
Move getInputMovement(SDL_Scancode _up, SDL_Scancode _down, SDL_Scancode _left, SDL_Scancode _right, Move _oldDirectio);

int main()
{
  if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
  {
    printf("%s\n",SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Window *win = NULL;
  win = SDL_CreateWindow("Snakes  -  Player 1 = Arrow Keys    Player 2 = WASD", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
  if (!win)
  {
    printf("%s\n",SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer *renderer = NULL;
  renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
  {
    printf("%s\n",SDL_GetError() );
    return EXIT_FAILURE;
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  // SDL image is an abstraction for all images
  SDL_Surface *imageSnake = NULL;
  SDL_Surface *imagePickup = NULL;
  SDL_Surface *imageKnight = NULL;
  SDL_Surface *imageGameOver = NULL;
  SDL_Surface *imageBackground = NULL;

  imageSnake = IMG_Load("snake.png");
  imagePickup = IMG_Load("Gems2.png");
  imageKnight = IMG_Load("Sprite1.png");
  imageGameOver = IMG_Load("GameOver.png");
  imageBackground = IMG_Load("background.png");

  if(!imageSnake || !imagePickup || !imageKnight || !imageGameOver || !imageBackground)
  {
    printf("IMG_Load: %s\n", IMG_GetError());
    return EXIT_FAILURE;
  }

  srand(time(NULL));

  // Load textures from file
  SDL_Texture *gameOver = NULL;
  gameOver = SDL_CreateTextureFromSurface(renderer, imageGameOver);
  SDL_FreeSurface(imageGameOver);

  SDL_Texture *background = NULL;
  background = SDL_CreateTextureFromSurface(renderer, imageBackground);
  SDL_FreeSurface(imageBackground);

  SDL_Texture *pickup = NULL;
  pickup = SDL_CreateTextureFromSurface(renderer, imagePickup);
  SDL_FreeSurface(imagePickup);

  SDL_Texture *special = NULL;
  special = SDL_CreateTextureFromSurface(renderer, imageKnight);
  SDL_FreeSurface(imageKnight);

  SDL_Texture *snakePlayer1 = NULL;
  SDL_Texture *snakePlayer2 = NULL;
  snakePlayer1  = SDL_CreateTextureFromSurface(renderer, imageSnake);
  snakePlayer2 = SDL_CreateTextureFromSurface(renderer, imageSnake);
  SDL_FreeSurface(imageSnake);

  // Set player colours
  SDL_SetTextureColorMod(snakePlayer1, 255, 96, 0);
  SDL_SetTextureColorMod(snakePlayer2, 255, 255, 0);

  // Initialising snake spawns and sizes
  Node player1HeadData;
    setState(&player1HeadData, HEAD);
    player1HeadData.next = NULL;
    player1HeadData.prev = NULL;

    player1HeadData.pos.x = PLAYER1_SPAWNX;
    player1HeadData.pos.y = PLAYER1_SPAWNY;
    player1HeadData.pos.w = SNAKE_RADIUS*PLAYER1_SCALE;
    player1HeadData.pos.h = SNAKE_RADIUS*PLAYER1_SCALE;

    player1HeadData.anim.currentFrame = 0;
    player1HeadData.idleDirection = RIGHT;

  Node player1BodyData = player1HeadData;
    setState(&player1BodyData, BODY);

  Node player2HeadData = player1HeadData;
    player2HeadData.pos.x = PLAYER2_SPAWNX;
    player2HeadData.pos.y = PLAYER2_SPAWNY;
    player2HeadData.pos.w = SNAKE_RADIUS*PLAYER2_SCALE;
    player2HeadData.pos.h = SNAKE_RADIUS*PLAYER2_SCALE;

  Node player2BodyData = player1BodyData;
    setState(&player2BodyData, BODY);


  // Initialise snakes (implemented using a linked list)
  Node *player1Head = createSnake(&player1HeadData, PLAYER1_SEGMENTS, &player1BodyData);
  Node *player1Tail = getLastSegment(player1Head);
  Move player1Direction = NOTMOVING;
  int  player1PickupCount = 0;

  Node *player2Head = createSnake(&player2HeadData, PLAYER2_SEGMENTS, &player2BodyData);
  Node *player2Tail = getLastSegment(player2Head);
  Move player2Direction = NOTMOVING;
  int  player2PickupCount = 0;

  Pickup gems[PICKUP_TOTAL];
  initialisePickups(gems);

  // Timing - ms
  const unsigned int c_playerFrameDelay = 150;
  const unsigned int c_PickupFrameDelay = 50;
  const unsigned int c_knightDirUpdate = 1500;
  const unsigned int c_gameLoopDelay = 30;

  unsigned int currentTime = SDL_GetTicks();
  unsigned int lastPlayerFrameUpdate = 0;
  unsigned int lastPickupFrameUpdate = 0;
  unsigned int lastKnightDirChange = 0;
  unsigned int lastGameUpdate = 0;

  // now we are going to loop forever, process the keys then draw
  int quit=false;

  while (quit != true)
  {
    currentTime = SDL_GetTicks();

    if(currentTime > (lastGameUpdate + c_gameLoopDelay))
    {
      // grab the SDL event (this will be keys etc)
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        // If the window is closed
        if (event.type == SDL_QUIT)
        {
          quit = true;
        }

        if (event.type == SDL_KEYDOWN)
        {
          switch (event.key.keysym.sym)
          {
            // if we have an escape quit
            case SDLK_ESCAPE :
              quit = true;
              break;
          }
        }
      }// end PollEvent loop

      player1Direction = getInputMovement(SDL_SCANCODE_UP,   SDL_SCANCODE_DOWN,
                                          SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
                                          player1Head->idleDirection);

      player2Direction = getInputMovement(SDL_SCANCODE_W,  SDL_SCANCODE_S,
                                          SDL_SCANCODE_A,  SDL_SCANCODE_D,
                                          player2Head->idleDirection);


      // Check if the snakes collect any Pickups
      for(int i = 0; i < PICKUP_TOTAL; i++)
      {
        if(gems[i].isVisible)
        {
          SDL_Rect gemPosition = { gems[i].pos.x,
                                   gems[i].pos.y,
                                   PICKUP_SIZE,
                                   PICKUP_SIZE };

          if(detectCollision(&player1Head->pos, &gemPosition, 6))
          {
            growsnake(player1Head, &player1Tail, &player1BodyData);

            gems[i].isVisible = false;
            player1PickupCount++;
          }

          if(detectCollision(&player2Head->pos, &gemPosition, 6))
          {
            growsnake(player2Head, &player2Tail, &player2BodyData);

            gems[i].isVisible = false;
            player2PickupCount++;
          }
        }
      }// End collision Pickup check


      // Exit the game if all the Pickups have been collected
      // or a player collides with their body
      if( (collidesWithSelf(player1Head) || collidesWithSelf(player2Head)) ||
          (player1PickupCount + player2PickupCount >= PICKUP_TOTAL) )
      {
        // Make the snakes red to make it obvious the player did something wrong
        SDL_RenderClear(renderer);

        SDL_SetTextureColorMod(snakePlayer1, 255, 0, 0);
        SDL_SetTextureColorMod(snakePlayer2, 255, 0, 0);

        renderSnakeBody(player1Head, player1Tail, renderer, snakePlayer1);
        renderSnakeHead(player1Head, renderer, snakePlayer1);
        renderSnakeBody(player2Head, player2Tail, renderer, snakePlayer2);
        renderSnakeHead(player2Head, renderer, snakePlayer2);

        SDL_RenderPresent(renderer);

        SDL_Delay(1000);

        displayGameOver(renderer, gameOver, player1PickupCount, player2PickupCount);

        SDL_Delay(2000);

        quit = true;
      }

      // Update player movement direction and the snake position
      updateSnakePos(player1Head, &player1Tail, player1Direction);
      updateSnakePos(player2Head, &player2Tail, player2Direction);


      // Update ticks and check time specific functions
      currentTime = SDL_GetTicks();

      // Increment the frames only every frameDelay ms
      if(currentTime > (lastPlayerFrameUpdate + c_playerFrameDelay))
      {
        updateSegmentFrames(player1Head);
        updateSegmentFrames(player2Head);

        lastPlayerFrameUpdate = currentTime;
      }
      else
      {
        // Just incase the frame didn't update in time
        // Reset it to 0 if the player isn't moving
        if( !getState(player1Head, MOVING) )
        {
          player1Head->anim.currentFrame = 0;
        }

        if( !getState(player2Head, MOVING) )
        {
          player2Head->anim.currentFrame = 0;
        }
      }

      // Update knight animations and positions
      if(currentTime > (lastPickupFrameUpdate + c_PickupFrameDelay))
      {
        for(int i = 0; i < PICKUP_TOTAL; ++i)
        {
          if(gems[i].canTravel)
          {
             Move dir = gems[i].Anim.offset.y;

             gems[i].Anim.offset.x++;
             gems[i].Anim.offset.x %= KNIGHT_FRAMETOTAL;

             moveSprite(dir, &gems[i].pos, 2);
          }
        }

        lastPickupFrameUpdate = currentTime;
      }

      // Randomise each knights movement every few ms
      if(currentTime > (lastKnightDirChange + c_knightDirUpdate))
      {
        for(int i = 0; i < PICKUP_TOTAL; ++i)
        {
          if(gems[i].canTravel)
          {
            Move direction = NOTMOVING;

            // Push knights away from the edge so they don't get hidden
            if(gems[i].pos.x < KNIGHT_SIZE) { direction = RIGHT; }
            if(gems[i].pos.y < KNIGHT_SIZE) { direction = DOWN;  }

            if(gems[i].pos.x > WIDTH - KNIGHT_SIZE*2) { direction = LEFT; }
            if(gems[i].pos.y > HEIGHT- KNIGHT_SIZE*2) { direction = UP;   }

            if(direction==NOTMOVING)
            {
              direction = getRandomMovement();
            }

            gems[i].Anim.offset.y = direction;
          }
        }

        lastKnightDirChange = currentTime;
      }

      // now we clear the screen (will use the clear colour set previously)
      SDL_RenderClear(renderer);

      renderBackground(renderer, background);

      // Copy every Pickup to renderer, ready for drawing to the screen
      // Any Pickup that has been 'picked up' by the player will not be drawn
      renderPickups(gems, renderer, pickup, special);

      renderSnakeBody(player1Head, player1Tail, renderer, snakePlayer1);
      renderSnakeHead(player1Head, renderer, snakePlayer1);

      renderSnakeBody(player2Head, player2Tail, renderer, snakePlayer2);
      renderSnakeHead(player2Head, renderer, snakePlayer2);


      // Update screen
      SDL_RenderPresent(renderer);

      // Update time
      lastGameUpdate = currentTime;
    } //
  } // end game loop

  // Clean up snake lists
  freeList(&player1Head);
  freeList(&player2Head);

  // exit SDL nicely and free resources
  SDL_Quit();
  return EXIT_SUCCESS;
}

///
/// \brief RenderBackground Tile the background texture until it fills the entire screen
/// \param _renderer The renderer
/// \param _tex
///
void renderBackground(SDL_Renderer *_renderer,
                      SDL_Texture  *_tex)
{
  const int c_bgSize = 128;

  SDL_Rect bgSrc = {0, 0, c_bgSize, c_bgSize};
  SDL_Rect bgDst = {0, 0, c_bgSize, c_bgSize};

  while(bgDst.x < WIDTH)
  {
    bgDst.y = 0;

    while(bgDst.y < HEIGHT)
    {
      SDL_RenderCopy(_renderer, _tex, &bgSrc, &bgDst);

      bgDst.y += c_bgSize;
    }

    bgDst.x += c_bgSize;
  }

}

///
/// \brief DisplayGameOver
/// \param _renderer
/// \param _tex
/// \param _firstScore The first players score
/// \param _secondScore The second players score
///
void displayGameOver(SDL_Renderer *_renderer,
                     SDL_Texture  *_tex,
                     int _firstScore,
                     int _secondScore)
{
  Coord screenCenter = { WIDTH/2, HEIGHT/2};

  const int c_rowHeight = 64;
  const int c_imageWidth = 384;

  SDL_Rect src;
  SDL_Rect dst;

  // Game over text offsets
  src.w = c_imageWidth;
  src.h = c_rowHeight;
  src.x = 0;
  src.y = 0;

  dst.w = src.w * 2;
  dst.h = src.h * 2;
  dst.x = screenCenter.x - c_imageWidth;
  dst.y = screenCenter.y + c_rowHeight;

  SDL_RenderCopy(_renderer, _tex, &src, &dst);

  // Player text offsets
  if(_firstScore != _secondScore)
  {
    // Player 1 or 2 wins
      src.y = (_firstScore > _secondScore) ? c_rowHeight : (c_rowHeight*2);
  }
  else
  {
    // Draw
    src.y += c_rowHeight*3;
  }

  // Next row onscreen
  dst.y += c_rowHeight;

  SDL_RenderCopy(_renderer, _tex, &src, &dst);

  SDL_RenderPresent(_renderer);
}


////
/// \brief GetState Checks a node's state
/// \param _node
/// \param _state
/// \return True if the node contains the state, otherwise false
///
bool getState(Node *_node,
              NodeState _state)
{
  if((_node->state & _state) == _state)
  {
    return true;
  }

  return false;
}


void addState(Node *_node,
              NodeState _state)
{
  _node->state |= _state;
}

void removeState(Node *_node,
                 NodeState _state)
{
  if(getState(_node, _state))
  {
    _node->state -= _state;
  }
}

void setState(Node *_node,
              NodeState _state)
{
  _node->state = _state;
}

///
/// \brief freeList Frees all of the memory used by a list of segments
/// \param io_root
///
void freeList(Node **io_root)
{
  Node *tmp;

  while(*io_root != NULL)
  {
    tmp = (*io_root);
    (*io_root) = (*io_root)->next;

    free(tmp);
  }
}

////
/// \brief LinkSegments
/// Helper function that correctly updates 2 segments to be linked to each other
/// \param Node
/// \param NodeToLinkTo
///
void linkSegments(Node *_node,
                  Node *_nodeToLinkTo)
{
  if(_node!=NULL && _nodeToLinkTo!=NULL)
  {
      _node->next = _nodeToLinkTo;
      _nodeToLinkTo->prev = _node;
  }
}

void unlinkNextSegment(Node *_linkedNode)
{
  if(_linkedNode->next != NULL)
  {
    _linkedNode->next->prev = _linkedNode->prev;
    _linkedNode->next = NULL;
  }
}

void unlinkPrevSegment(Node *_linkedNode)
{
  if(_linkedNode->prev != NULL)
  {
    _linkedNode->prev->next = _linkedNode->next;
    _linkedNode->prev = NULL;
  }
}

////
/// \brief InsertAfterSegment
/// \param _listNode
/// \param _newNode
/// \param _isNewNodeLinked
///
void insertAfterSegment(Node *_listNode,
                        Node *_newNode,
                        bool _isNewNodeLinked)
{
  if(_isNewNodeLinked)
  {
    unlinkNextSegment(_newNode);
    unlinkPrevSegment(_newNode);
  }

  _newNode->prev = _listNode;
  _newNode->next = _listNode->next;

  _listNode->next = _newNode;
}

///
/// \brief Growsnake Adds a new segment to the snake
/// \param _head
/// \param io_tail
/// \param _data Data that will be copied over to the new segment
///
void growsnake(Node *_head,
               Node **io_tail,
               Node *_data)
{
  if(io_tail==NULL || (*io_tail)==NULL)
  {
    return;
  }

  if(_head!=NULL && _head->next!=NULL)
  {
    // The body will use this state to give the impression
    // that the snake is swallowing it's prey
    addState(_head->next, EATING);
  }

  Node *newTail = createSegment(_data);
  // Quick way of ensuring the new tail is hidden until the player moves
  newTail->pos.x = 0 - SNAKE_RADIUS*2;
  newTail->pos.y = 0 - SNAKE_RADIUS*2;

  newTail->prev = (*io_tail);
  (*io_tail) = newTail;
  (*io_tail)->anim.currentFrame = (*io_tail)->prev->anim.currentFrame;
}

///
/// \brief GetLastSegment
/// \param _root
/// \return The last segment in a chain of segments
///
Node *getLastSegment(Node * _root)
{
  if(_root!=NULL)
  {
    while( _root->next != NULL )
    {
      _root = _root->next;
    }
  }

  return _root;
}

/////
/// \brief CreateSegment Allocates memory for a new segment
/// \param _data Data that will be copied over to the new segment
/// \return A pointer to the new segment
///
Node *createSegment(Node * _data)
{
    Node *newSegment = malloc(sizeof(Node));
    *newSegment = *_data;
    newSegment->next = NULL;
    newSegment->prev = NULL;

    return newSegment;
}

////
/// \brief UpdateSegmentFrames Checks the state of every segment in the snake and updates the frame accordingly
/// \param _head The first segment Node in the snake
///
void updateSegmentFrames(Node *_head)
{
  // Frame totals
  const unsigned int c_bodyMove = 1;
  const unsigned int c_headMove = 2;

  bool isMoving = getState(_head, MOVING);

  Node *node = _head;

  while(node!=NULL)
  {
    if(isMoving)
    {
      unsigned int frameTotal = getState(node, HEAD) ? c_headMove : c_bodyMove;

      if(node->anim.currentFrame < frameTotal)
      {
        node->anim.currentFrame++;
      }
      else
      {
        node->anim.currentFrame = 0;
      }
    }

    node = node->next;
  }
}

///
/// \brief RenderSnakeHead
/// \param _head
/// \param _renderer
/// \param _tex The spritesheet to use to render the head
///
void renderSnakeHead( Node *_head,
                      SDL_Renderer * _renderer,
                      SDL_Texture *_tex)
{
  // The spritesheet column to start in,
  // the move animation begins +32 pixels from the left
  int startOffset = getState(_head, MOVING) ? SNAKE_RADIUS : 0;

  SDL_Rect src = getFrameOffset(_head->idleDirection, SNAKE_RADIUS,
                                _head->anim.currentFrame, startOffset);
  SDL_Rect dst = _head->pos;

  SDL_RenderCopy(_renderer, _tex, &src, &dst);
}

////
/// \brief RenderSnake Renders tail first, so the head is placed correctly on top of the other segments
/// \param _tail
/// \param _renderer
/// \param _tex The spritesheet to use to render the body
///
void renderSnakeBody( Node *_head,
                      Node *_tail,
                      SDL_Renderer *_renderer,
                      SDL_Texture *_tex )
{
  _head = _head->next;

  SDL_Rect src;
  src.w = SNAKE_RADIUS;
  src.h = SNAKE_RADIUS;
  src.y = BODY_OFFSET;

  while(_tail != NULL)
  {
    src.x = _tail->anim.currentFrame * SNAKE_RADIUS;

    // Create the lump that moves through the snakes body when it eats
    if(getState(_tail, EATING))
    {
      src.x += BODY_EAT_OFFSET;
    }

    // Set the darker/alternate segments
    src.y = (getState(_tail, ALT)) ? BODY_ALT_OFFSET : BODY_OFFSET;

    SDL_RenderCopy(_renderer, _tex, &src, &_tail->pos);

    _tail = _tail->prev;
  }
}
////
/// \brief GetInputMovement Checks for input from the user, pressing opposing keys will return NOTMOVING
/// \param _up
/// \param _down
/// \param _left
/// \param _right
/// \return A move direction, based on the keys pressed.
///
Move getInputMovement(SDL_Scancode _up,
                      SDL_Scancode _down,
                      SDL_Scancode _left,
                      SDL_Scancode _right,
                      Move _oldDirection)
{
  // Alternate method of reading key input, doesn't produce any delay while holding down a key
  const Uint8 *KEY = SDL_GetKeyboardState(NULL);

  Move newDirection = NOTMOVING;

  if(KEY[_left] )
  {
      newDirection = LEFT;
  }
  if(KEY[_right] )
  {
      newDirection = RIGHT;
  }
  if(KEY[_up] )
  {
      if(KEY[_left])
        newDirection = UPLEFT;
      else if(KEY[_right])
        newDirection = UPRIGHT;
      else
        newDirection = UP;
  }
  if(KEY[_down] )
  {
      if(KEY[_left])
        newDirection = DOWNLEFT;
      else if(KEY[_right])
        newDirection = DOWNRIGHT;
      else
        newDirection = DOWN;
  }

  // Quick and dirty way of checking if the user is trying to move in 2 directions at once
  // (this would mean instant death for them, as the snake collides with itself)
  bool opposingDirection = (_oldDirection == LEFT   && newDirection == RIGHT)
                        || (_oldDirection == RIGHT  && newDirection == LEFT)
                        || (_oldDirection == UP     && newDirection == DOWN)
                        || (_oldDirection == DOWN   && newDirection == UP)
                        || (_oldDirection == UPRIGHT   && newDirection == DOWNLEFT)
                        || (_oldDirection == UPLEFT    && newDirection == DOWNRIGHT)
                        || (_oldDirection == DOWNLEFT  && newDirection == UPRIGHT)
                        || (_oldDirection == DOWNRIGHT && newDirection == UPLEFT);

  return (opposingDirection) ? NOTMOVING : newDirection;
}



