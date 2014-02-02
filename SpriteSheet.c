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

#define PICKUP_TOTAL      (24)
#define PICKUP_SIZE       (28)
#define KNIGHT_SIZE       (64)
#define KNIGHT_FRAMETOTAL (9)

const int WIDTH=800;
const int HEIGHT=600;

// These correspond to each row in the knight/snake spritesheets
typedef enum{
    NOTMOVING = -1,
    UP        = 0,
    LEFT      = 1,
    DOWN      = 2,
    RIGHT     = 3,
    UPLEFT    = 4,
    UPRIGHT   = 5,
    DOWNLEFT  = 6,
    DOWNRIGHT = 7
} Move;

// Flags that will be OR'd together, such as BODY|EAT of HEAD|MOVING,
// and used to determine the correct animation
typedef enum{
  NONE   = 0x00,
  BODY   = 0x01,
  ALT    = 0x02,  // Alternate body colour
  HEAD   = 0x04,
  MOVING = 0x08,
  EATING = 0x0F
} NodeState;

typedef enum{
  BLUE,
  GREEN,
  RED,
  CRYSTAL
} Gem;

typedef struct Coord
{
  int x;
  int y;
} Coord;

// Can store either a regular static Pickup (gem)
// or a mobile knight sprite
typedef struct {
  union
  {
    Gem type;       // Used by gem
    Coord offset;   // Used by knight
  } Anim;

  SDL_Rect pos;

  bool canTravel;   // True if the Pickup is a knight

  bool isVisible;
} Pickup;

// The snake is implemented using a doubly-linked list
typedef struct Node{
  NodeState state;
  SDL_Rect pos;

  struct SEQUENCE{
      unsigned int xOffset;
      unsigned int yOffset;
      unsigned int currentFrame;
  }anim;
  Move idleDirection;

  struct Node *next;
  struct Node *prev;
} Node;


// Snake
Node *createSnake(Node *_head, int _count, Node *_body);
Node *getLastSegment(Node * _root);
Node *createSegment(Node * _dataSegment);
void insertAfterSegment(Node *_listNode, Node *_newNode, bool _isNewNodeLinked);
void growsnake(Node *_head, Node **io_tail, Node *_data);
void linkSegments(Node *_node, Node *_nodeToLinkTo);
void unlinkNextSegment(Node *_linkedNode);
void unlinkPrevSegment(Node *_linkedNode);
void updateSegmentFrames(Node *_head);
void freeList(Node **io_root);
bool collidesWithSelf(Node *_head);

// State
bool getState(Node *_node, NodeState _state);
void addState(Node *_node, NodeState _state);
void removeState(Node *_node, NodeState _state);
void setState(Node *_node, NodeState _state);

// Rendering
SDL_Rect getFrameOffset(Move _dir, int _size, int _frame, int _startOffset);
void renderBackground(SDL_Renderer *_renderer, SDL_Texture  *_tex);
void displayGameOver(SDL_Renderer *_renderer, SDL_Texture  *_tex, int _firstScore, int _secondScore);
void renderSnakeHead( Node *_head, SDL_Renderer * _renderer, SDL_Texture *_tex);
void renderSnakeBody( Node *_head, Node *_tail, SDL_Renderer *_renderer, SDL_Texture *_tex );
void renderPickups(Pickup *_array, SDL_Renderer *_renderer, SDL_Texture* _pickupTex, SDL_Texture* _specialTex);

// Movement
void moveSprite(Move _dir, SDL_Rect *io_pos, int _offset);
void updateSnakePos(Node * _head, Node ** io_tail, Move _dir);
void shiftSnakeBody(Node *_head, Node **_tail, SDL_Rect *_oldHeadPos);

// Input
Move getRandomMovement();
Move getInputMovement(SDL_Scancode _up, SDL_Scancode _down, SDL_Scancode _left, SDL_Scancode _right);

// Pickups
void initialisePickups(Pickup *_array);

bool detectCollision(const SDL_Rect *_a, const SDL_Rect *_b, int _clipRadius);
int randRange(int _Min, int _Max);


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
  SDL_Surface *imageSnake;
  SDL_Surface *imagePickup;
  SDL_Surface *imageKnight;
  SDL_Surface *imageGameOver;
  SDL_Surface *imageBackground;

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

  // SDL texture converts the image to a texture suitable for SDL rendering  / blitting
  // once we have the texture it will be store in hardware and we don't need the image data anymore
  SDL_Texture *snakePlayer1 = NULL;
  SDL_Texture *snakePlayer2 = NULL;
  SDL_Texture *pickup = NULL;
  SDL_Texture *special = NULL;
  SDL_Texture *gameOver = NULL;
  SDL_Texture *background = NULL;

  // Create the textures
  pickup = SDL_CreateTextureFromSurface(renderer, imagePickup);
  special = SDL_CreateTextureFromSurface(renderer, imageKnight);
  gameOver = SDL_CreateTextureFromSurface(renderer, imageGameOver);
  background = SDL_CreateTextureFromSurface(renderer, imageBackground);

  snakePlayer1  = SDL_CreateTextureFromSurface(renderer, imageSnake);
  snakePlayer2 = SDL_CreateTextureFromSurface(renderer, imageSnake);

  // Set player colours
  SDL_SetTextureColorMod(snakePlayer1, 255, 96, 0);
  SDL_SetTextureColorMod(snakePlayer2, 255, 255, 0);

  // Free the image
  SDL_FreeSurface(imageSnake);
  SDL_FreeSurface(imagePickup);
  SDL_FreeSurface(imageKnight);
  SDL_FreeSurface(imageGameOver);

  srand(time(NULL));

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


      // Read the input from the players
      player1Direction = getInputMovement(SDL_SCANCODE_UP,   SDL_SCANCODE_DOWN,
                                          SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);

      player2Direction = getInputMovement(SDL_SCANCODE_W,  SDL_SCANCODE_S,
                                          SDL_SCANCODE_A,  SDL_SCANCODE_D);


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


////
/// \brief GetFrameOffset Calculates the x/y frame offset
/// based on move direction and the current frame
/// \param _dir The move direction
/// \param _size
/// \param _frame The current frame
/// \param _startOffset Which column to start the animation from
/// \return An SDL_Rect with the correct animation offsets
///
SDL_Rect getFrameOffset(Move _dir,
                        int _size,
                        int _frame,
                        int _startOffset)
{
  SDL_Rect src;
  src.w = _size;
  src.h = _size;
  src.x = _startOffset + _size*_frame;
  src.y = _size*_dir;

  return src;
}

////
/// \brief CreateSnake
///  Creates the head, and optionally a specified amount of body, of a snake.
/// \param _headSegment Template data to use to make the head segment
/// \param _bodySegmentCount How many body segments to create
/// \param _bodySegment Template data to use to make the body segments
/// \return A pointer to the root/head segment of a new snake
///
Node *createSnake(Node *_head,
                  int _count,
                  Node *_body)
{
  Node *root = createSegment(_head);
  setState(root, HEAD);
  setState(_body, BODY);

  const int StripeSize = 3;
  int counter = 0;

  Node *listptr = root;
  if(_count > 0)
  {
    //Create and position the body segments
    for(int i = 0; i < _count; ++i)
    {
      if(listptr!=NULL)
      {  
        // Set the initial strip pattern on the snake
        // and change the starting frame for a ripple effect
        counter++;
        if(counter <= StripeSize)
        {
          addState(_body, ALT);
          _body->anim.currentFrame = 1;
        }
        else if(counter < (StripeSize*2))
        {
          removeState(_body, ALT);
          _body->anim.currentFrame = 0;
        }
        else
        {
          counter = 0;
        }

        insertAfterSegment(listptr, createSegment(_body), false);
        updateSnakePos(root, &listptr->next, RIGHT);

        listptr = getLastSegment(root);
      }
    }
  }

  return root;
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
      // Update head segment
      if(getState(node, HEAD))
      {
        if(node->anim.currentFrame < c_headMove)
        {
          node->anim.currentFrame++;
        }
        else
        {
          node->anim.currentFrame = 0;
        }
      }
      else
      {
        // Update body segment
        if(node->anim.currentFrame < c_bodyMove)
        {
          node->anim.currentFrame++;
        }
        else
        {
          node->anim.currentFrame = 0;
        }
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
/// \brief RandomMovement
///
/// \return A random move direction
///
Move getRandomMovement()
{
  switch(randRange(0, 3))
  {
    case 0:  { return UP;    break;}
    case 1:  { return DOWN;  break;}
    case 2:  { return LEFT;  break;}
    default: { return RIGHT; break;}
  }
}

////
/// \brief GetInputMovement Checks for input from the user
/// \param _up
/// \param _down
/// \param _left
/// \param _right
/// \return A move direction, based on the keys pressed
///
Move getInputMovement(SDL_Scancode _up,
                      SDL_Scancode _down,
                      SDL_Scancode _left,
                      SDL_Scancode _right)
{
  // Alternate method of reading key input, doesn't produce any delay while holding down a key
  const Uint8 *KEY = SDL_GetKeyboardState(NULL);

  if(KEY[_up]   &&  KEY[_left] ) { return UPLEFT;    }
  if(KEY[_up]   &&  KEY[_right]) { return UPRIGHT;   }
  if(KEY[_down] &&  KEY[_left] ) { return DOWNLEFT;  }
  if(KEY[_down] &&  KEY[_right]) { return DOWNRIGHT; }

  if(KEY[_up]   ) { return UP;   }
  if(KEY[_down] ) { return DOWN; }
  if(KEY[_left] ) { return LEFT; }
  if(KEY[_right]) { return RIGHT;}

  return NOTMOVING;
}

///
/// \brief MoveSprite Moves an SDL_Rect in the direction passed,
/// supports diagonal movement
/// \param _dir The move direction
/// \param io_pos
/// \param _offset How much to offset in the direction
///
void moveSprite(Move _dir,
                SDL_Rect *io_pos,
                int _offset)
{
  // Put me in a function, reuse me for the sprite knights
  if(_dir == LEFT)  { io_pos->x -= _offset; }
  if(_dir == RIGHT) { io_pos->x += _offset; }

  if(_dir == UP   || _dir == UPLEFT   || _dir == UPRIGHT)
  {
    io_pos->y -= _offset;
    if(_dir == UPLEFT)    { io_pos->x -= _offset; }
    if(_dir == UPRIGHT)   { io_pos->x += _offset; }
  }

  if(_dir == DOWN || _dir == DOWNLEFT || _dir == DOWNRIGHT)
  {
    io_pos->y += _offset;
    if(_dir == DOWNLEFT)  { io_pos->x -= _offset; }
    if(_dir == DOWNRIGHT) { io_pos->x += _offset; }
  }

  // If the player attempts to walk offscreen, wrap their position around to the opposite side
  if(io_pos->y <= -io_pos->h)    { io_pos->y += (HEIGHT + io_pos->h * 2); }
  if(io_pos->y >= HEIGHT)        { io_pos->y -= (HEIGHT + io_pos->h * 2); }

  if(io_pos->x <= -io_pos->w)    { io_pos->x += (WIDTH + io_pos->w * 2); }
  if(io_pos->x >= WIDTH)         { io_pos->x -= (WIDTH + io_pos->w * 2); }

}

///
/// \brief UpdateSnakePos Offsets the snake, sets up the state of the head
/// and moves the rest of the body
/// \param _head
/// \param io_tail
/// \param _dir The direction the head should move
///
void updateSnakePos(Node * _head,
                    Node ** io_tail,
                    Move _dir)
{
  if(_head != NULL && *io_tail!=NULL)
  {
    if(_dir != NOTMOVING)
    {
      const int segmentRadius = _head->pos.h;
      const int moveOffset = segmentRadius/4;
      Node newNeck = *_head; // Keep track of the head's old position

      addState(_head, MOVING);

      moveSprite(_dir, &_head->pos, moveOffset);

      // Store the last move direction so the head
      // points in the right direction when there is no input
      _head->idleDirection = _dir;

      shiftSnakeBody(_head, io_tail, &newNeck.pos);
    }
    else
    {
      removeState(_head, MOVING);
    }
  }
}

///
/// \brief shiftSnakeBody
/// Shifts the tail end of the snake into the old position of the head,
/// giving the effect that the snake is sliding without having to iterate
/// through the entire list every movement
///
/// \param _head
/// \param _tail
/// \param _oldHeadPos The position of the head before it was offset
///
void shiftSnakeBody(Node *_head,
                    Node **io_tail,
                    SDL_Rect *_oldHeadPos)
{
  if(_head == NULL || io_tail == NULL)
  {
    return;
  }

  // We must only be dealing with 2 segments,
  // so don't attempt to rearrange the linked segments
  if((*io_tail)->prev == _head)
  {
    (*io_tail)->pos = *_oldHeadPos;
    return;
  }

  Node *head = _head;
  Node *neck = _head->next;
  Node *newneck = *io_tail;
  newneck->pos = *_oldHeadPos;

  // The new tail will be the second to last segment
  *io_tail = (*io_tail)->prev;

  // Remove the swallow effect when it reaches the tail
  if(getState(*io_tail, EATING))
  {
    removeState(*io_tail, EATING);
  }

  unlinkNextSegment(newneck->prev);

  linkSegments(head, newneck);
  linkSegments(newneck, neck);
}

///
/// \brief InitialisePickups Sets up random position and type/direction for each pickup
/// \param _array Array of pickups
///
void initialisePickups(Pickup *_array)
{
  // Populate Pickup array with randomised positions/types
  for(int i = 0; i < PICKUP_TOTAL; ++i)
  {
    // A hacky way of setting a 1/5 chance of creating a moving Pickup (knight)
    _array[i].canTravel = !(randRange(0, 4));

    if(_array[i].canTravel)
    {
      _array[i].pos.x = randRange(0, WIDTH - KNIGHT_SIZE);
      _array[i].pos.y = randRange(0, HEIGHT - KNIGHT_SIZE);

      _array[i].Anim.offset.x = 0;
      _array[i].Anim.offset.y = getRandomMovement();
      //Randomly choose a knight direction
    }
    else
    {
      _array[i].pos.x = randRange(0, WIDTH - PICKUP_SIZE);
      _array[i].pos.y = randRange(0, HEIGHT - PICKUP_SIZE);

      //Randomly choose a type of gem
      _array[i].Anim.type = randRange(BLUE, CRYSTAL);
    }

    _array[i].isVisible = true;
  }
}

///
/// \brief RenderPickups Renders gems and knights onto _renderer, the type is automatically
/// determined
/// \param _array Array of pickups
/// \param _renderer The renderer to RenderCopy() to
/// \param _pickupTex The sprite sheet to use for regular pickups (gems)
/// \param _specialTex The sprite sheet to use for moving pickups (knights)
///
void renderPickups(Pickup *_array,
                   SDL_Renderer *_renderer,
                   SDL_Texture *_pickupTex,
                   SDL_Texture *_specialTex)
{
  for(int i=0; i < PICKUP_TOTAL; i++)
  {
    SDL_Rect src;
    SDL_Rect dst;

    if(_array[i].isVisible)
    {
      dst.x = _array[i].pos.x;
      dst.y = _array[i].pos.y;

      if(_array[i].canTravel)
      {
        src = getFrameOffset(_array[i].Anim.offset.y, KNIGHT_SIZE,
                             _array[i].Anim.offset.x, 0);

        dst.w = KNIGHT_SIZE;
        dst.h = KNIGHT_SIZE;

        SDL_RenderCopy(_renderer, _specialTex, &src, &dst);
      }
      else
      {
        src = getFrameOffset(0, PICKUP_SIZE, _array[i].Anim.type, 0);

        dst.w = PICKUP_SIZE;
        dst.h = PICKUP_SIZE;

        SDL_RenderCopy(_renderer, _pickupTex, &src, &dst);
      }
    }
  }
}

///
/// \brief CollidesWithSelf Checks if the snake has collided with any part of it's body
/// \param _head The root segment
/// \return True if there is any collision, otherwise false
///
bool collidesWithSelf(Node *_head)
{
  // Only check every few segments, as they overlap
  const int c_segmentsToSkip = 8;
  int counter = 0;

  Node *tmp = _head;

  while(tmp != NULL)
  {
    counter++;

    if((counter % c_segmentsToSkip) == 0)
    {
      if(detectCollision(&_head->pos, &tmp->pos, 14))
      {
        return true;
      }
    }

    tmp = tmp->next;
  }

  return false;
}

///
/// \brief DetectCollision Used to determine if two rectangles collide
/// \param _a First rectangle
/// \param _b Second rectangle
/// \param _clipRadius How many pixels within the pickup the rect must be before the collision is valid,
/// prevents the player from collecting a pick up by merely clipping it's edge
/// \return True if collision, False if not
///
bool detectCollision(const SDL_Rect *_a, const SDL_Rect *_b, int _clipRadius)
{
  //Checks all the edges for cases where it is impossible to intersect
  //such as the left edge of rect _b being to the right of rect _a's right edge

  const int c_aLeft   = _a->x + _clipRadius;
  const int c_aRight  = _a->x + _a->w - _clipRadius;
  const int c_aTop    = _a->y + _clipRadius;
  const int c_aBottom = _a->y + _a->h - _clipRadius;

  const int c_bLeft   = _b->x + _clipRadius;
  const int c_bRight  = _b->x + _b->w - _clipRadius;
  const int c_bTop    = _b->y + _clipRadius;
  const int c_bBottom = _b->y + _b->h - _clipRadius;

  // No collision possible if any of these cases are true
  if((c_aLeft > c_bRight) || (c_aRight < c_bLeft) ||
     (c_aBottom < c_bTop) || (c_aTop > c_bBottom))
  {

    return false;
  }

  return true;
}

/// @brief RandRange Returns a random value between _min and _max
/// Modified : Removed srand initialisation, it's now part of main()
/// 'tripplet' (December 9, 2011). Stack Overflow.
/// [Accessed 2013]. Available from: <http://stackoverflow.com/questions/8449234/c-random-number-between-2-numbers-reset>.
int randRange(const int _min, const int _max)
{
  return (rand()%((_max+1)-_min)+_min);
}
