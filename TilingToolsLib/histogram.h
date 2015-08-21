#pragma once
#ifndef HISTOGRAM_H
#define HISTOGRAM_H
#include "stdafx.h"

namespace gmx
{

class Metatag {
public:
  virtual  bool GetSerializedBinary(int &size, void *&data) = 0;
  virtual  bool GetSerializedText(int &size, char *&text) = 0;
  virtual  bool Deserialize(int size, void *data) = 0;
  virtual  string GetName() =0;
  virtual  int GetSerializedBinarySize() =0;
};

class Metadata {
public:
  Metadata();
  ~Metadata();
  void AddTagRef(Metatag *p_metatag);
  static Metatag* DeserializeTag(string tag_name, int size, void *data);
  
  void DeleteAll();

  int TagCount();
  Metatag* GetTagRef(int n);
  Metatag* GetTagRef(string tag_name);
  bool GetAllSerialized(int &size, void* &data);
  int GetAllSerializedSize();
  bool SaveToTextFile(string filename);
protected:
  Metatag **p_metatags_;
  int num_tags_;
};




class MetaNodataValue : public Metatag {
public:

  bool GetSerializedBinary(int &size, void *&data) 
  {
    data = new char[8];
    size=8;
    memcpy(data,&nodv_,8);
    return true;
  };

  bool GetSerializedText(int &size, char *&text)
  {
    char buf[256];
    sprintf(buf,"%.2lf",nodv_);

    string str(buf);
    size = str.size();
    text = new char[size];
    memcpy(text,buf,size);
    return true;
  };
  
  bool Deserialize(int size, void *data)
  {
    if (size!=8) return false;
    memcpy(&nodv_,data,8);
    return true;
  }
  string GetName() 
  {
    return "NODV";
  };
  int GetSerializedBinarySize() {return 8;};

public:
  double nodv_;
};


class MetaHistogramStatistics : public Metatag
{
public:
  MetaHistogramStatistics ()
  {
    num_bands_ = 0;
    min_=0;max_=0;mean_=0;stdev_=0;
  };

  void Empty()
  {
    if (min_) delete[]min_;
    if (max_) delete[]max_;
    if (mean_) delete[]mean_;
    if (stdev_) delete[]stdev_;
    num_bands_ = 0;
    min_=0;max_=0;mean_=0;stdev_=0;
  };

  ~MetaHistogramStatistics ()
  {
    Empty();
  };

  bool Init (int num_bands)
  {
    Empty();
    if (num_bands<0) return false;
    num_bands_=num_bands;
    min_ = new double[num_bands_];
    max_ = new double[num_bands_];
    mean_ = new double[num_bands_];
    stdev_ = new double[num_bands_];
    return true;
  };

  bool GetSerializedBinary(int &size, void *&data)
  {
    if (num_bands_==0) return false;
    size = GetSerializedBinarySize ();
    char *ch_data = (char*)(data = new char[size]);
    memcpy(ch_data,&num_bands_,4);
    for (int b=0;b<num_bands_;b++)
    {
      memcpy(&ch_data[(32*b +4)],&min_[b],8);
      memcpy(&ch_data[(32*b +4) +8],&max_[b],8);
      memcpy(&ch_data[(32*b +4) + 16],&mean_[b],8);
      memcpy(&ch_data[(32*b +4) + 24],&stdev_[b],8);
    }

    return true;
  };

  bool GetSerializedText(int &size, char *&text)
  {
    if (num_bands_==0) return false;
    char buf[1000];
    string str_text;
    for (int b=0;b<num_bands_;b++)
    {
      sprintf(buf,"band %d: min = %.2lf, max = %.2lf, mean = %.2lf, stdev = %.2lf\n",
            (b+1),min_[b],max_[b],mean_[b],stdev_[b]);
      str_text+=buf;
    }
    size = str_text.size();
    text = new char[size];
    memcpy(text,str_text.c_str(),size);
    return true;
  };

  bool Deserialize(int size, void *data)
  {
    if (size==0) return false;
    memcpy(&num_bands_,data,4);
    if (num_bands_<=0) return false;
    Init(num_bands_);
    char *ch_data = (char*)data;
    for (int b=0;b<num_bands_;b++)
    {
      memcpy(&min_[b],&ch_data[(32*b +4)],8);
      memcpy(&max_[b],&ch_data[(32*b +4) +8],8);
      memcpy(&mean_[b],&ch_data[(32*b +4) + 16],8);
      memcpy(&stdev_[b],&ch_data[(32*b +4) + 24],8);
    }
    return true;
  };

  string GetName() {return "STAT";};
  
  int GetSerializedBinarySize() {return 4 + 32*num_bands_;};
 
public:
  int num_bands_;
  double *min_, *max_, *mean_, *stdev_;
};


class MetaHistogram : public Metatag{
public:
  MetaHistogram () 
  {
    min_val_=0;
    step_=0;
    num_bands_=0;
    num_vals_=0;
    freqs_=0;
  };
    
  bool Init(int num_bands, GDALDataType gdt);
  bool Init(int num_bands, double min_val, double step, double max_val);
  bool Init(int num_bands, double min_val, double step, int num_vals);
  bool IsInitiated() {return num_bands_!=0;};
    
  bool GetSerializedBinary (int &size, void *&data);
  int GetSerializedBinarySize ();
  bool GetSerializedText(int &size, char *&text);
  string GetName() {return "HIST";};
  bool Deserialize (int size, void *data);

  ~MetaHistogram();
  void AddValue(int band, double value);
  __int64 GetFrequency(int band,double value);
  bool CalcStatistics(MetaHistogramStatistics *p_hist_stat, double *p_nodata);
  int CalcNumOfExistingValues(int band);
  bool GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs);

protected:
  void CalcStatisticsByBand(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata);

private:
  double min_val_;
  double step_;
  int num_bands_;
  int num_vals_;
  __int64 **freqs_;
  
};




/*
class MetaHistogram
{
public:
  MetaHistogram () 
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
  bool GetSerializedMetaHistogram (int &size, void *&data, bool converted_to_float = false);
  int GetSerializedMetaHistogramSize (bool converted_to_float = false);
  bool GetSerializedStatistics (int &size, void *&data, double *p_nodata=NULL);
  int GetSerializedStatisticsSize ();

  bool Deserialize (int size, void *data, bool converted_to_float=false);
  bool SaveToTextFile(string filename);

  //MetaHistogram(int num_bands, GDALDataType gdt);
  //MetaHistogram(int num_bands, double min_val, double step, double max_val);
  ~MetaHistogram();
  void AddValue(int band, double value);
  __int64 GetFrequency(int band,double value);
  void CalcStatistics(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata = NULL);
  int CalcNumOfExistingValues(int band);
  bool GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs);

private:
  double min_val_;
  double step_;
  int num_bands_;
  int num_vals_;
  __int64 **freqs_;
  
};
*/
/*
class Metadata {
public:
  Metadata();
  ~Metadata();
  bool AddTag(char name[4], int size, void *data);
  int GetAllSize();
  bool GetAllSerialized(int &size, void* &data);
  bool PrepareTag(char[4], int size);
  int TagCount(){return num_tags_;};
  bool GetSerializedTagByName(char name[4],int &size, void* &data);
  bool HISTToText(int &text_size, char* &text_data);
  bool STATToText(int &text_size, char* &text_data);
  bool STATToText(int &text_size, char* &text_data);

  bool FromBinaryToText(char name[4], int &text_size, char* &text_data);
  bool SaveMetadataToTextFile(file_name);

private:
  int FindTag (char name[4]);

private:
  char **names_;
  int *sizes_;
  char **data_;
  int num_tags_;
  int max_tags_;
};
*/


}




#endif