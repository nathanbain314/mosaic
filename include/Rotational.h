#include "mosaicTools.h"

typedef tuple< int, double, double > rotateResult;

void RunRotational( string inputName, string outputName, vector< string > inputDirectory, int numIter, int angleOffset, double imageScale, double renderScale, int numSkip );