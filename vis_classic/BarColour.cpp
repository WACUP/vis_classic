extern int draw_height;

unsigned char BarColourClassic(int, int y)
{
          return (unsigned char)(y);  // classic
}

unsigned char BarColourLines(int i, int)
{
          return (unsigned char)i;  // whole line fading
}

unsigned char BarColourElevator(int i, int y)
{
  int high_colour_position = i * i / 255;

  // y correction so bottom colour is not always 0;
  if(y == 0)
   y = 255 / draw_height;

  if(high_colour_position > 0) {
    if(y <= high_colour_position)
      return (unsigned char)(y * 255 / high_colour_position);
    else {
      if(i < 127)
        return (unsigned char)((i - y) * 255 / (i - high_colour_position));
      else
        return (unsigned char)((1 + high_colour_position - (y - high_colour_position)) * 255 / high_colour_position);
    }
  } else
    return (unsigned char)y;
}

unsigned char BarColourFire(int i, int y)
{
  // y correction so bottom colour is not always 0;
  if(y == 0)
   y = 255 / draw_height;

   return (unsigned char)(y * 255 / i);  // Faded Bars - my fire
}

unsigned char BarColourWinampFire(int i, int y)
{
          return (unsigned char)(y + 255 - i); // Winamp fire style;
}
