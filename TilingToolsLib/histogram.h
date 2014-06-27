#pragma once
#ifndef HISTOGRAM_H
#define HISTOGRAM_H
#include "stdafx.h"

namespace gmx
{


  
class Histogram
{
public:
  Histogram () 
  {
    min_val_=0;
    step_=0;
    num_bands_=0;
    num_vals_=0;
    freqs_=0;
  };

  bool Init(int num_bands, GDALDataType gdt);
  bool Init(int num_bands, float min_val, float step, float max_val);
  bool IsInitiated() {return num_bands_!=0;};

  //Histogram(int num_bands, GDALDataType gdt);
  //Histogram(int num_bands, float min_val, float step, float max_val);
  ~Histogram();
  void AddValue(int band, float value);
  __int64 GetFrequency(int band,float value);
  void CalcStatistics(int band, float &min, float &max, float &mean, float &stdev);
  int CalcNumOfExistingValues(int band);
  BOOL GetHistogram(int band, float &min_val, float &step, int &num_vals, __int64 *&freqs);

private:
  float min_val_;
  float step_;
  int num_bands_;
  int num_vals_;
  __int64 **freqs_;
  
};


}




#endif