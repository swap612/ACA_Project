#include<iostream>
using namespace std;
class LLC
{
  public:
     unsigned long long L3[L3_SETS][L3_WAYS];
     unsigned long long int Time_Stamp_L3[L3_SETS][L3_WAYS];
     unsigned long long Hit_l3,Miss_l3;
   
   void reset()
   {
    //clearing L3 cache
    for(int i=0;i<L3_SETS;i++)
    {
    for(int j=0;j<L3_WAYS;j++)
    {
      L3[i][j]=0;
    }
   }
   //clearing L3 Timestamp
   for(int i=0;i<L3_SETS;i++)
   {
    for(int j=0;j<L3_WAYS;j++)
    {
      Time_Stamp_L3[i][j]=0;
    }
   }
  }
void print()
{
printf("--------------L3_DATA-----------\n");
   for(int i=0;i<L3_SETS;i++)
   {
      for(int j=0;j<L3_WAYS;j++)
      {
         printf("%lld\t|",L3[i][j]);
      
      }
      printf("\n");
   }
     printf("--------------L3_Timestamp-----------\n");
   for(int i=0;i<L3_SETS;i++)
   {
      for(int j=0;j<L3_WAYS;j++)
      {
         printf("%lld\t|",Time_Stamp_L3[i][j]);
      
      }
      printf("\n");
   }
}
int find_victim(int SET)
{  
  int i,min=Time_Stamp_L3[SET][0],WAY=0;
  for(i=1;i<L3_WAYS;i++)
  {
    if(Time_Stamp_L3[SET][i]<=min) 
    { 
        min=Time_Stamp_L3[SET][i];
        WAY=i; 
    }   
  }
  
  return WAY;
}
void update_timestamp(int SET,int WAY,int opr)
{
  Time_Stamp++;
  //opr 0 mean invalidate
  if(opr==0) 
    Time_Stamp_L3[SET][WAY] = 0; 
  else
    //update timestamp
    Time_Stamp_L3[SET][WAY] = Time_Stamp; 
}
int search(int SET,int TAG)
{
  for(int i=0;i<L3_WAYS;i++) 
  {
    if((L3[SET][i]==TAG) && (Time_Stamp_L3[SET][i]!=0))
      return i;
  }
  return -1; 
}
void insert(int SET,int TAG)
{
  //WAY return the position to be evict, then insert data at WAY and update LRU table
  int WAY =find_victim(SET);
  L3[SET][WAY]=TAG;
  update_timestamp(SET,WAY,1); 
}


}