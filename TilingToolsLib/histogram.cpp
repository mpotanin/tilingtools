#include "StdAfx.h"
#include "histogram.h"

using namespace gmx;

Histogram::Histogram(int num_bands)
{
  if (num_bands<=0) return;
  num_bands_=num_bands;
  freqs_ = new float*[num_bands];

  for (int i=0;i<num_bands_;i++)
    freqs_[i]=new float[(1<<16)-1];
}

Histogram::~Histogram()
{
  for (int i=0;i<num_bands_;i++)
    delete[]freqs_[i];
  delete[]freqs_;
}

void Histogram::AddValue(int band, float value)
{
  if (band<num_bands_) freqs_[band][(unsigned int)value]++;
}

unsigned __int64 Histogram::GetFrequency(int band, float value)
{
   if (band<num_bands_) return freqs_[band][(unsigned int)value];
   else return 0;
}


void Histogram::CalcStatistics(int band, float &min, float &max, float &mean, float &stdev)
{
  min=0;
  max=0;
  mean=0;
  stdev=0;
  if (band>=num_bands_) return;

  for (int i=0;i<(1<<16);i++)
  {
    if (freqs_[band][i] !=0)
    {
      min = i;
      break;
    }
  }

  for (int i = (1<<16)-1; i>=0; i--)
  {
    if (freqs_[band][i] !=0)
    {
      max = i;
      break;
    }
  }

  unsigned __int64 total_values = 0;

  for (int i = (1<<16)-1; i>=0; i--)
  {
    total_values+=freqs_[band][i];
  }

  for (int i = (1<<16)-1; i>=0; i--)
  {
    if (freqs_[band][i]!=0)
      mean+=((float)freqs_[band][i]/(float)total_values)*i;
  }

  for (int i = (1<<16)-1; i>=0; i--)
  {
    if (freqs_[band][i]!=0)
      stdev+=(mean-i)*(mean-i)*((float)freqs_[band][i]/(float)total_values);
  }
  stdev=sqrt(stdev);

}

unsigned int Histogram::CalcNumOfExistingValues(int band)
{
  if (band>=num_bands_) return 0;
  
  unsigned int num=0;
  for (int i = (1<<16)-1; i>=0; i--)
  {
    if (freqs_[band][i]!=0) num++;
  }
  return num;
}

