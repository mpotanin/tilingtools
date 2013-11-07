#pragma once
#ifndef HISTOGRAM_H
#define HISTOGRAM_H
#include "stdafx.h"

namespace gmx
{


  
class Histogram
{
public:
  Histogram(int num_bands);
  ~Histogram();
  void AddValue(int band, float value);
  unsigned __int64 GetFrequency(int band,float value);
  void CalcStatistics(int band, float &min, float &max, float &mean, float &stdev);
  unsigned int CalcNumOfExistingValues(int band);

private:
  int num_bands_;
  float **freqs_;
};


}




#endif