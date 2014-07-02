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
  bool Init(int num_bands, double min_val, double step, double max_val);
  bool IsInitiated() {return num_bands_!=0;};

  //Histogram(int num_bands, GDALDataType gdt);
  //Histogram(int num_bands, double min_val, double step, double max_val);
  ~Histogram();
  void AddValue(int band, double value);
  __int64 GetFrequency(int band,double value);
  void CalcStatistics(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata = NULL);
  int CalcNumOfExistingValues(int band);
  BOOL GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs);

private:
  double min_val_;
  double step_;
  int num_bands_;
  int num_vals_;
  __int64 **freqs_;
  
};


}




#endif