//int (*LevelCalcStereo)(int low, int high, unsigned char *spectrumData);
int UnionLevelCalcStereo(int low, int high, unsigned char *spectrumData);
int AverageLevelCalcStereo(int low, int high, unsigned char *spectrumData);
//int (*LevelCalcMono)(int low, int high, unsigned char *spectrumDataLeft, unsigned char *spectrumDataRight);
int UnionLevelCalcMono(int low, int high, unsigned char *spectrumDataLeft, unsigned char *spectrumDataRight);
int AverageLevelCalcMono(int low, int high, unsigned char *spectrumDataLeft, unsigned char *spectrumDataRight);
