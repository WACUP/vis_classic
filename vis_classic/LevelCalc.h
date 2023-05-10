//int (*LevelCalcStereo)(const int low, const int high, const unsigned char *spectrumData);
int UnionLevelCalcStereo(const int low, const int high, const unsigned char *spectrumData);
int AverageLevelCalcStereo(const int low, const int high, const unsigned char *spectrumData);
//int (*LevelCalcMono)(const int low, const int high, const unsigned char *spectrumDataLeft, const unsigned char *spectrumDataRight);
int UnionLevelCalcMono(const int low, const int high, const unsigned char *spectrumDataLeft, const unsigned char *spectrumDataRight);
int AverageLevelCalcMono(const int low, const int high, const unsigned char *spectrumDataLeft, const unsigned char *spectrumDataRight);
