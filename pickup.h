#ifndef PICKUP_H
#define PICKUP_H

#include <stdbool.h>

#include "utils.h"
#include "actor.h"


#define PICKUP_TOTAL      (32)
#define PICKUP_SIZE       (28)
#define KNIGHT_SIZE       (64)
#define KNIGHT_FRAMETOTAL (9)

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
                        int _startOffset);
///
/// \brief InitialisePickups Sets up random position and type/direction for each pickup
/// \param _array Array of pickups
///
void initialisePickups(Pickup *_array);

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
                   SDL_Texture *_specialTex);

////
/// \brief RandomMovement
///
/// \return A random move direction
///
Move getRandomMovement();

#endif // PICKUP_H
