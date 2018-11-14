#include<iostream>
using namespace std;
#define L2_ID 0
#define L2_SETS 1024
#define L2_WAYS 8

//states
#define PR 'R'
#define PW 'W'
#define I 'I'
#define S 'S'
#define M 'M'
#define E 'E'
#define O 'O'

//Core Counts
#define CORE_COUNT 16
//Defining Queue Parameters
#define queue_size 60


class L2
{
   public:
     unsigned long long tags[L2_SETS][L2_WAYS];
     unsigned long long int timeStamp[L2_SETS][L2_WAYS];
     unsigned long long hitCount,missCount;
     char dir[L2_SETS][L2_WAYS];
     int q_empty;
     unsigned long long waiting_time;

   L2(){
    hitCount = 0;
    missCount = 0;
    waiting_time=0;
    q_empty=queue_size;
    reset();
  } 
   void reset()
   {
    //clearing L2 cache
    for(int i=0;i<L2_SETS;i++)
    {
    for(int j=0;j<L2_WAYS;j++)
    {
      tags[i][j]=0;
    }
   }
   //clearing L2 Timestamp
   for(int i=0;i<L2_SETS;i++)
   {
    for(int j=0;j<L2_WAYS;j++)
    {
      timeStamp[i][j]=0;
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
         printf("%lld\t|",tags[i][j]);
      
      }
      printf("\n");
   }
     printf("--------------L2_Timestamp-----------\n");
   for(int i=0;i<L2_SETS;i++)
   {
      for(int j=0;j<L2_WAYS;j++)
      {
         printf("%lld\t|",timeStamp[i][j]);
      
      }
      printf("\n");
   }
}
char get_state(int set,unsigned long long tag,int *way)
{
   for(int i=0;i<L2_WAYS;i++) 
   {
    if((tags[set][i]==tag))
      {
        *way = i;
        return dir[set][i];
      }
   }
  return 'I'; 
}
int find_victim(int set)
{  
  int i,min=timeStamp[set][0],way=0;
  for(i=1;i<L2_WAYS;i++)
  {
    if(timeStamp[set][i]<=min) 
    { 
        min=timeStamp[set][i];
        way=i; 
    }   
  }
  
  return way;
}
void update_timestamp(int set,int way,int opr, unsigned long long * timeStampCntr)
{
  (*timeStampCntr)++;
  //opr 0 mean invalidate
  if(opr==0) 
   { timeStamp[set][way] = 0;
     dir[set][way]='I';}
  else
    //update timestamp
    timeStamp[set][way] = *timeStampCntr; 
}
int search(int set, unsigned long long tag)
{
  for(int i=0;i<L2_WAYS;i++) 
  {
    if((tags[set][i]==tag) && (timeStamp[set][i]!=0))
      return i;
  }
  return -1; 
}
int insert(int set,int tag, unsigned long long * timeStampCntr)
{
  //way return the position to be evict, then insert data at way and update LRU table
  int way =find_victim(set);
  tags[set][way]=tag;
  update_timestamp(set, way, 1, timeStampCntr); 
  return way;
}


};
