#include "actor.h"

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

// Movement
void moveSprite(Move _dir, SDL_Rect *io_pos, int _offset);
void updateSnakePos(Node * _head, Node ** io_tail, Move _dir);
void shiftSnakeBody(Node *_head, Node **_tail, SDL_Rect *_oldHeadPos);

// State
bool getState(Node *_node, NodeState _state);
void addState(Node *_node, NodeState _state);
void removeState(Node *_node, NodeState _state);
void setState(Node *_node, NodeState _state);

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

bool collidesWithSelf(Node *_head)
{
  // Only check every few segments, as they overlap
  const int c_segmentsToSkip = 8;
  const int c_segmentPadding = 14;
  int counter = 1; // Start at first segment

  Node *tmp = _head;

  while(tmp != NULL)
  {
    if((counter++ % c_segmentsToSkip) == 0)
    {
      if(detectCollision(&_head->pos, &tmp->pos, c_segmentPadding))
      {
        return true;
      }
    }

    tmp = tmp->next;
  }

  return false;
}

