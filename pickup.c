#include "pickup.h"

void initialisePickups(Pickup *_array)
{
  const int WIDTH=800;
  const int HEIGHT=600;

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



Move getRandomMovement()
{
  return (Move)(randRange(UP, RIGHT));
}
