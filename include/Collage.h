
#include "mosaicTools.h"

typedef tuple< int, float, float > rotateResult;

void RunCollage( string inputName, string outputName, vector< string > inputDirectory, int angleOffset, float imageScale, float renderScale, int fillPercentage, string fileName, int minSize, int maxSize, int skip, bool recursiveSearch );