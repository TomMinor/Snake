#include "utils.h"

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
