#include "stdafx.h"
#include "histogram.h"

using namespace gmx;


Metatag* Metadata::DeserializeTag(string name, int size, void *data)
{
  Metatag *p_metatag = NULL;

  if (name == "HIST")
    p_metatag = new MetaHistogram();
  else if (name == "STAT")
    p_metatag = new MetaHistogramStatistics();
  else if (name == "NODV")
    p_metatag = new MetaNodataValue();
  if (p_metatag)
    p_metatag->Deserialize(size,data);
  return p_metatag;
}

void Metadata::DeleteAll()
{
  for (int i=0;i<num_tags_;i++)
    delete(this->p_metatags_[i]);
  this->num_tags_ = 0;
  delete[]p_metatags_;
  p_metatags_=0;
}


Metadata::Metadata()
{
  num_tags_=0;
  p_metatags_ = new Metatag*[255]; 
  for (int i=0;i<255;i++)
    p_metatags_[i]=0;
}

Metadata::~Metadata()
{
  delete[]p_metatags_;
}

void Metadata::AddTagDirectly(Metatag *p_metatag)
{
  for (int i=0;i<num_tags_;i++)
  {
    if (p_metatags_[i]->GetName() == p_metatag->GetName())
    {
      p_metatags_[i] = p_metatag;
      return;
    }
  }
  p_metatags_[num_tags_] = p_metatag;
  num_tags_++;
  return;
}

int Metadata::TagCount()
{
  return num_tags_;
}

Metatag* Metadata::GetTagRef(int n)
{
  if ( n<0 && n>=num_tags_) return NULL;
  else return p_metatags_[n];
}

Metatag* Metadata::GetTagRef(string name)
{
  for (int i=0;i<num_tags_;i++)
  {
    if (p_metatags_[i]->GetName() == name) return p_metatags_[i];
  }
  return NULL;
}


bool Metadata::GetAllSerialized(int &size, void* &data)
{
  data = NULL;
  size = GetAllSerializedSize();
  if (size==0) return false;
  char *ch_data = (char*)(data = new char[size]);
  int pos=0;
  for (int i=0;i<num_tags_;i++)
  {
    void *tag_data;
    int tag_size;
    p_metatags_[i]->GetSerialized(tag_size,tag_data);
    if (tag_size==0) continue;
    memcpy(&ch_data[pos],p_metatags_[i]->GetName().c_str(),4);
    memcpy(&ch_data[pos+4],&tag_size,4);
    memcpy(&ch_data[pos+8],tag_data,tag_size);
    pos+=tag_size+8;
  }
  return true;
}

int Metadata::GetAllSerializedSize()
{
  int size = 0;
  for (int i=0;i<num_tags_;i++)
    size+=p_metatags_[i]->GetSerializedSize() + 8;
  return size;
}


bool Metadata::SaveToTextFile(string filename)
{
  FILE *fp = fopen(filename.c_str(),"w");
  if (fp == NULL) return false;
  char *tag_text;
  int text_size;
  for (int i=0; i<num_tags_;i++)
  {
    p_metatags_[i]->GetSerializedText(text_size,tag_text);
    fprintf(fp,"%s\n", p_metatags_[i]->GetName().c_str());
    for (int j=0;j<text_size;j++)
      fprintf(fp,"%c",tag_text[j]);
    fprintf(fp,"\n");
  }
  fclose(fp);
  return true;
}


bool MetaHistogram::Init(int num_bands, GDALDataType gdt)
{
  if (gdt == GDT_Byte) return Init(num_bands,0,1,256);
  else if (gdt == GDT_UInt16) return Init(num_bands,0,1, 0xFFFF + 1);
  else return Init(num_bands,-0x8000,1, 0xFFFF + 1);
}
  

bool MetaHistogram::Init(int num_bands, double min_val, double step, double max_val)
{
  if (step<=0.0) return false;
  return Init (num_bands,min_val,step,(int)(((max_val-min_val)/step)+1.5));
}

bool MetaHistogram::Init(int num_bands, double min_val, double step, int num_vals)
{
  if (num_bands<=0 || step<=0.0 || num_vals<=0) return false;

  num_bands_=num_bands;
  step_=step;
  min_val_=min_val;
  num_vals_=num_vals;
  
  freqs_ = new __int64*[num_bands];
  for (int i=0;i<num_bands_;i++)
  {
     freqs_[i]=new __int64[num_vals_];
     for (int j=0;j<num_vals_;j++)
       freqs_[i][j]=0;
  }

  return true;
}


MetaHistogram::~MetaHistogram()
{
  for (int b=0;b<num_bands_;b++)
    delete[]freqs_[b];
  delete[]freqs_;
}

void MetaHistogram::AddValue(int band, double value)
{
  if (band>=num_bands_) return;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return;
  freqs_[band][n]++;
}

__int64 MetaHistogram::GetFrequency(int band, double value)
{
  if (band>=num_bands_) return 0;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return 0;
  
  return freqs_[band][n];
} 

int MetaHistogram::GetSerializedSize ()
{
  return 4 + 8 + 8 + 4 + 4*num_vals_*num_bands_;
}


bool MetaHistogram::Deserialize (int size, void *data)
{
  if (IsInitiated() || size == 0 || data == NULL) return false;
  
  char *ch_data = (char*)data;
  memcpy(&num_bands_,ch_data,4);
  memcpy(&min_val_,&ch_data[4],8);
  memcpy(&step_,&ch_data[12],8);
  memcpy(&num_vals_,&ch_data[20],4);
  if(!Init(num_bands_,min_val_,step_,min_val_+step_*(num_vals_-1))) return false;
  float freq_f;
  for (int b=0;b<num_bands_;b++)
  {
    for (int i=0;i<num_vals_;i++)
    {
      memcpy(&freq_f,&ch_data[24+4*b*num_vals_ + 4*i],4);
      freqs_[b][i] = freq_f;
    }
  }

  return true;
}

bool MetaHistogram::GetSerialized (int &size, void *&data)
{
  if (!IsInitiated()) return false;

  size = GetSerializedSize();
  char *ch_data = (char*)(data = new char[size]);

  memcpy(ch_data,&num_bands_,4);
  memcpy(&ch_data[4],&min_val_,8);
  memcpy(&ch_data[12],&step_,8);
  memcpy(&ch_data[20],&num_vals_,4);
  float val_f;
  for (int b=0;b<num_bands_;b++)
  {
    for (int i=0;i<num_vals_;i++)
    {
      val_f = freqs_[b][i];
      memcpy(&ch_data[24 + 4*i + 4*b*num_vals_],&val_f,4);
    }
  }

  return true;
}


bool MetaHistogram::CalcStatistics(MetaHistogramStatistics *p_hist_stat, double *p_nodata)
{
  if(!IsInitiated())return false;
  if (p_hist_stat == NULL) return false;
  p_hist_stat->Init(num_bands_);
  
  for (int b=0;b<num_bands_;b++)
    CalcStatisticsByBand(b,p_hist_stat->min_[b],p_hist_stat->max_[b],p_hist_stat->mean_[b],p_hist_stat->stdev_[b],p_nodata);
  return p_hist_stat;

}

void MetaHistogram::CalcStatisticsByBand(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata)
{
  min=0;
  max=0;
  mean=0;
  stdev=0;

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
  
  double v = min_val_;
  
  for (int i = 0; i<num_vals_;i++)
  {
    if (i!=num_nodata && freqs_[band][i]!=0)
    {
      mean+=((double)freqs_[band][i]/(double)total_freqs)*v;
      stdev+=((double)freqs_[band][i]/(double)total_freqs)*v*v;
    }
    v+=step_;
  }
  stdev=sqrt(stdev-mean*mean);
}


int MetaHistogram::CalcNumOfExistingValues(int band)
{
  if (band>=num_bands_) return 0;
  
  int num=0;
  for (int i = 0; i<num_vals_; i++)
  {
    if (freqs_[band][i]!=0) num++;
  }
  return num;
}

bool MetaHistogram::GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs)
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

bool  MetaHistogram::GetSerializedText(int &size, char *&text)
{
  size=0;text=0;
  if (!IsInitiated()) return false;

  int max_size = 15*num_bands_*num_vals_;
  string str_text;
  char buf[256];
  for (int i=0;i<num_vals_;i++)
  {
    sprintf(buf,"%.2lf",min_val_+i*step_);
    str_text+=buf;
    for (int b=0;b<num_bands_;b++)
    {
      sprintf(buf,";%d",freqs_[b][i]);
      str_text+=buf;
    }
    str_text+="\n";
  }
  size = str_text.size();
  text = new char[size];
  memcpy(text,str_text.c_str(),size);
  return true;
}


/*
int Metadata::FindTag(char name[4])
{
  for (int i=0; i<num_tags_;i++)
  {  
    if (names_[i][0] == name[0] && names_[i][1] == name[1] && 
        names_[i][2] == name[2] && names_[i][3] == name[3])
        return i;
  }
  return -1;
}

bool Metadata::AddTag(char name[4], int size, void *data)
{
  int n;
  if ( (n=FindTag(name))>=0)
  {
    memcpy(data_[n],data,size);
    return true;
  }
  names_[num_tags_] = new char[4];
  memcpy(names_[num_tags_],name,4);
  sizes_[num_tags_]=size;
  data_[num_tags_] = new char[size];
  memcpy(data_[num_tags_],data,size);
  num_tags_++;

  return true;
}


bool Metadata::PrepareTag(char name[4], int size)
{
  if (FindTag(name)>=0) return false;
  char *data = new char[size];
  for (int i=0;i<size;i++)
    data[i]=0;
  AddTag(name,size,data);
  delete[]data;
  return true;
}


int Metadata::GetAllSize()
{
  int size = 0;
  for (int i=0;i<num_tags_;i++)
  {
    size+=4+4+sizes_[i];
  }
  return size;
}

bool Metadata::GetSerializedTagByName(char name[4],int &size, void* &data)
{
  int n;
  if ((n=FindTag(name))<0) return false;
  size = sizes_[n];
  data = new char[size];
  memcpy(data,data_[n],size);
  return true;
}

bool Metadata::GetAllSerialized(int &size, void* &data)
{
  if (num_tags_==0)
  {
    size=0;
    data=0;
    return false;
  }

  size = GetAllSize();
  char *ch_data = (char*)(data = new char[size]);

  int pos = 0;
  for (int i=0;i<num_tags_;i++)
  {
    memcpy(&ch_data[pos],names_[i],4);
    memcpy(&ch_data[pos+4],&sizes_[i],4);
    memcpy(&ch_data[pos+8],data_[i],sizes_[i]);
    pos+=8+sizes_[i];
  }

  return true;
}
*/


/*

bool MetaHistogram::Init(int num_bands, GDALDataType gdt)
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

bool MetaHistogram::Init(int num_bands, double min_val, double step, double max_val)
{
  if (num_bands<=0) return false;

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


MetaHistogram::~MetaHistogram()
{
  for (int i=0;i<num_bands_;i++)
    delete[]freqs_[i];
  delete[]freqs_;
}

void MetaHistogram::AddValue(int band, double value)
{
  if (band>=num_bands_) return;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return;
  freqs_[band][n]++;
}

__int64 MetaHistogram::GetFrequency(int band, double value)
{
  if (band>=num_bands_) return 0;
  int n = (int)(((value-min_val_)/step_)+0.5);
  if (n>=num_vals_) return 0;
  
  return freqs_[band][n];
} 

int MetaHistogram::GetSerializedMetaHistogramSize (bool converted_to_float)
{
  return 4 + 8 + 8 + 4 + ((converted_to_float) ? 4*num_vals_*num_bands_ : 8*num_vals_*num_bands_);
}

bool MetaHistogram::SaveToTextFile(string filename)
{
  if (!IsInitiated()) return false;
  FILE *fp = fopen(filename.c_str(),"w");
  if (!fp) return false;
  for (int i=0;i<num_vals_;i++)
  {
    fprintf(fp,"%.2f",min_val_+i*step_);
    for (int b=0;b<num_bands_;b++)
      fprintf(fp,";%d",freqs_[b][i]);
    fprintf(fp,"\n");
  }

  fclose(fp);
  return true;
}

bool MetaHistogram::Deserialize (int size, void *data, bool converted_to_float)
{
  if (IsInitiated() || size == 0 || data == NULL) return false;
  
  char *ch_data = (char*)data;
  memcpy(&num_bands_,ch_data,4);
  memcpy(&min_val_,&ch_data[4],8);
  memcpy(&step_,&ch_data[12],8);
  memcpy(&num_vals_,&ch_data[20],4);
  if(!Init(num_bands_,min_val_,step_,min_val_+step_*(num_vals_-1))) return false;
  float freq_f;
  for (int b=0;b<num_bands_;b++)
  {
    for (int i=0;i<num_vals_;i++)
    {
      if (converted_to_float)
      {
        memcpy(&freq_f,&ch_data[24+4*b*num_vals_ + 4*i],4);
        freqs_[b][i] = freq_f;
      }
      else memcpy(&freqs_[b][i],&ch_data[24+8*b*num_vals_ + 8*i],8);
    }
  }

  return true;
}

bool MetaHistogram::GetSerializedMetaHistogram (int &size, void *&data, bool converted_to_float)
{
  if (!IsInitiated()) return false;

  size = GetSerializedMetaHistogramSize (converted_to_float);
  char *ch_data = (char*)(data = new char[size]);

  memcpy(ch_data,&num_bands_,4);
  memcpy(&ch_data[4],&min_val_,8);
  memcpy(&ch_data[12],&step_,8);
  memcpy(&ch_data[20],&num_vals_,4);
  float val_f;
  for (int b=0;b<num_bands_;b++)
  {
    for (int i=0;i<num_vals_;i++)
    {
      if (converted_to_float)
      {
        val_f = freqs_[b][i];
        memcpy(&ch_data[24 + 4*i + 4*b*num_vals_],&val_f,4);
      }
      else memcpy(&ch_data[24 + 8*i + 8*b*num_vals_],&freqs_[b][i],8);
    }
  }

  return true;
}

int MetaHistogram::GetSerializedStatisticsSize ()
{
  return 32*num_bands_;
}

bool MetaHistogram::GetSerializedStatistics (int &size, void *&data, double *p_nodata)
{
  if (!IsInitiated()) return false;

  size = GetSerializedStatisticsSize ();
  char *ch_data = (char*)(data = new char[size]);

  for (int b=0;b<num_bands_;b++)
  {
    double min,max,mean,stdev;
    CalcStatistics(b,min,max,mean,stdev,p_nodata);
    memcpy(&ch_data[32*b],&min,8);
    memcpy(&ch_data[32*b +8],&max,8);
    memcpy(&ch_data[32*b + 16],&mean,8);
    memcpy(&ch_data[32*b + 24],&stdev,8);
  }

  return true;
}


void MetaHistogram::CalcStatistics(int band, double &min, double &max, double &mean, double &stdev, double *p_nodata)
{
  min=0;
  max=0;
  mean=0;
  stdev=0;

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
  
  double v = min_val_;
  
  for (int i = 0; i<num_vals_;i++)
  {
    if (i!=num_nodata && freqs_[band][i]!=0)
    {
      mean+=((double)freqs_[band][i]/(double)total_freqs)*v;
      stdev+=((double)freqs_[band][i]/(double)total_freqs)*v*v;
    }
    v+=step_;
  }
  stdev=sqrt(stdev-mean*mean);
}


int MetaHistogram::CalcNumOfExistingValues(int band)
{
  if (band>=num_bands_) return 0;
  
  int num=0;
  for (int i = 0; i<num_vals_; i++)
  {
    if (freqs_[band][i]!=0) num++;
  }
  return num;
}

bool MetaHistogram::GetHistogram(int band, double &min_val, double &step, int &num_vals, __int64 *&freqs)
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
*/