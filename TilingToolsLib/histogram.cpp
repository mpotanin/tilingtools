#include "StdAfx.h"
#include "histogram.h"

using namespace gmx;



bool Histogram::Init(int num_bands, GDALDataType gdt)
{
  if ((num_bands<=0) || (num_bands_>0)) return false;

  num_bands_=num_bands;
  min_val_=0;
  step_=1;

  switch (gdt)
  {
    case GDT_Byte:
      num_vals_=256;
      break;
    default:
      num_vals_=0xFFFF + 1;
      break;
  }
  
  freqs_ = new __int64*[num_bands];
  for (int i=0;i<num_bands_;i++)
  {
     freqs_[i]=new __int64[num_vals_];
     for (int j=0;j<num_vals_;j++)
       freqs_[i][j]=0;
  }
  return true;
}

bool Histogram::Init(int num_bands, double min_val, double step, double max_val)
{
  if ((num_bands<=0) || (num_bands_>0)) return false;

  num_bands_=num_bands;
  step_=step;
  min_val_=min_val;
  num_vals_=(int)(((max_val-min_val_)/step_)+1.5);
  
  freqs_ = new __int64*[num_bands];
  for (int i=0;i<num_bands_;i++)
  {
     freqs_[i]=new __int64[num_vals_];
     for (int j=0;j<num_vals_;j++)
       freqs_[i][j]=0;
  }

  return true;
}


Histogram::~Histogram()
{
  for (int i=0;i<num_bands_;i++)
    delete[]freqs_[i];
  delete[]freqs_;
}

void Histogram::AddValue(int band, double value)
{
  if (band>=num_bands_) return;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return;
  freqs_[band][n]++;
}

__int64 Histogram::GetFrequency(int band, double value)
{
  if (band>=num_bands_) return 0;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return 0;
  
  return freqs_[band][n];
} 


void Histogram::CalcStatistics(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata)
{
  min=0;
  max=0;

  if (band>=num_bands_) return;

  int num_nodata = (p_nodata==NULL) ? -1 : (int)(0.5+ ((p_nodata[0] - min_val_)/step_));
  

  for (int i=0;i<num_vals_;i++)
  {
    if (i==num_nodata) continue;
    if (freqs_[band][i] !=0)
    {
      min = min_val_ + i*step_;
      break;
    }
  }

  for (int i = num_vals_-1; i>=0; i--)
  {
    if (i==num_nodata) continue;
    if (freqs_[band][i] !=0)
    {
      max = min_val_ + i*step_;
      break;
    }
  }

  unsigned __int64 total_freqs = 0;

  for (int i = 0; i<num_vals_; i++)
  {
    if (i==num_nodata) continue;
    total_freqs+=freqs_[band][i];
  }


  mean=0;
  stdev=0;
  double v = min_val_;

  for (int i = 0; i<num_vals_;i++)
  {
    if (i==num_nodata) continue;
    if (freqs_[band][i]!=0)
    {
      mean+=((double)freqs_[band][i]/(double)total_freqs)*v;
      stdev+=((double)freqs_[band][i]/(double)total_freqs)*v*v;
    }
    v+=step_;
  }

  stdev=sqrt(stdev-mean*mean);
}


int Histogram::CalcNumOfExistingValues(int band)
{
  if (band>=num_bands_) return 0;
  
  int num=0;
  for (int i = 0; i<num_vals_; i++)
  {
    if (freqs_[band][i]!=0) num++;
  }
  return num;
}

BOOL Histogram::GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs)
{
  if (band>=num_bands_) return FALSE;

  min_val = min_val_;
  step = step_;
  num_vals = num_vals_;
  freqs =  new __int64[num_vals];
  for (int i=0;i<num_vals_;i++)
    freqs[i]=freqs_[band][i];
  
  return TRUE;
}


