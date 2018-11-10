unsigned char PeakColourFade(int, int y)
{
      return (unsigned char)(y); // fade-out
}

unsigned char PeakColourLevel(int i, int)
{
      return (unsigned char)i; // colour by level
}

unsigned char PeakColourLevelFade(int i, int y)
{
      return (unsigned char)(i - (int)((255 - y) / (255.0 / i)));
}
