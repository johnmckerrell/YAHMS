
#include "YAHMS_Defines.h"
#include "YAHMS_Config.h"

extern char outputPins[];
extern char inputPins[];
extern char analogPins[];

extern char xbeeRX;
extern char xbeeTX;

void SetupSampler();

void TakeSamples();

void CheckAndSubmitSamples();

int smoothSamples( int samples[], int numSamples);
