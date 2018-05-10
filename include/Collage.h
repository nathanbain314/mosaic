#include "mosaicTools.h"

typedef tuple< int, double, double > rotateResult;

void RunCollage( string inputName, string outputName, vector< string > inputDirectory, int angleOffset, double imageScale, double renderScale, int fillPercentage, bool trueColor, string fileName, int minSize, int maxSize );