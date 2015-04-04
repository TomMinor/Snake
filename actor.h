#ifndef ACTOR_H
#define ACTOR_H

#include "utils.h"
#include <stdbool.h>

extern const int WIDTH;
extern const int HEIGHT;

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
                  Node *_body);

Node *getLastSegment(Node * _root);
Node *createSegment(Node * _dataSegment);
void insertAfterSegment(Node *_listNode, Node *_newNode, bool _isNewNodeLinked);
void growsnake(Node *_head, Node **io_tail, Node *_data);
void linkSegments(Node *_node, Node *_nodeToLinkTo);
void unlinkNextSegment(Node *_linkedNode);
void unlinkPrevSegment(Node *_linkedNode);
void updateSegmentFrames(Node *_head);
void freeList(Node **io_root);
///
/// \brief CollidesWithSelf Checks if the snake has collided with any part of it's body
/// \param _head The root segment
/// \return True if there is any collision, otherwise false
///
bool collidesWithSelf(Node *_head);

// Movement
void moveSprite(Move _dir, SDL_Rect *io_pos, int _offset);
void updateSnakePos(Node * _head, Node ** io_tail, Move _dir);
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
                    SDL_Rect *_oldHeadPos);

// State
bool getState(Node *_node, NodeState _state);
void addState(Node *_node, NodeState _state);
void removeState(Node *_node, NodeState _state);
void setState(Node *_node, NodeState _state);


#endif // ACTOR_H
