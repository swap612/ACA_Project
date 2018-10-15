#include<iostream>
using namespace std;


#define L2_SETS 2048
#define L2_WAYS 8

class L2
{
   public:
     unsigned long long L2[L2_SETS][L2_WAYS];
     unsigned long long int Time_Stamp_L2[L2_SETS][L2_WAYS];
     unsigned long long Hit_L2,Miss_L2;
   
   void reset()
   {
    //clearing L2 cache
    for(int i=0;i<L2_SETS;i++)
    {
    for(int j=0;j<L2_WAYS;j++)
    {
      L2[i][j]=0;
    }
   }
   //clearing L2 Timestamp
   for(int i=0;i<L2_SETS;i++)
   {
    for(int j=0;j<L2_WAYS;j++)
    {
      Time_Stamp_L2[i][j]=0;
    }
   }
  }
void print()
{
printf("--------------L2_DATA-----------\n");
   for(int i=0;i<L2_SETS;i++)
   {
      for(int j=0;j<L2_WAYS;j++)
      {
         printf("%lld\t|",L2[i][j]);
      
      }
      printf("\n");
   }
     printf("--------------L2_Timestamp-----------\n");
   for(int i=0;i<L2_SETS;i++)
   {
      for(int j=0;j<L2_WAYS;j++)
      {
         printf("%lld\t|",Time_Stamp_L2[i][j]);
      
      }
      printf("\n");
   }
}
int find_victim(int SET)
{  
  int i,min=Time_Stamp_L2[SET][0],WAY=0;
  for(i=1;i<L2_WAYS;i++)
  {
    if(Time_Stamp_L2[SET][i]<=min) 
    { 
        min=Time_Stamp_L2[SET][i];
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
    Time_Stamp_L2[SET][WAY] = 0; 
  else
    //update timestamp
    Time_Stamp_L2[SET][WAY] = Time_Stamp; 
}
int search(int SET,int TAG)
{
  for(int i=0;i<L2_WAYS;i++) 
  {
    if((L2[SET][i]==TAG) && (Time_Stamp_L2[SET][i]!=0))
      return i;
  }
  return -1; 
}
void insert(int SET,int TAG)
{
  //WAY return the position to be evict, then insert data at WAY and update LRU table
  int WAY =find_victim(SET);
  L2[SET][WAY]=TAG;
  update_timestamp(SET,WAY,1); 
}


}
