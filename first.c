#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
struct line{
  int validbit;
  long tag;
  long order;
};
bool powerOfTwo(int n)
{
   if(n==0)
   return false;

   return (ceil(log2(n)) == floor(log2(n)));
}
long indexCalc(unsigned long memoryAddress,long offsetSize, long indexBits){
  long indexBitValue=0;
  int powerValue=offsetSize;
  memoryAddress=memoryAddress>>offsetSize;
  int i;
  for(i=0;i<indexBits;i++){
    int bitValue =(memoryAddress) & 1;
    if(bitValue==1){
      indexBitValue+=bitValue*pow(2,i);
      powerValue++;
    }else{
      powerValue++;
    }
    memoryAddress=(memoryAddress>>1);
  }
    return indexBitValue;
}
long tagBitCalc(unsigned long memoryAddress,long offsetSize, long indexBits,long tagBitSize){
  long tagBitValue=0;
  int powerValue=offsetSize+indexBits;
  memoryAddress=memoryAddress>>(offsetSize+indexBits);
  int i;
  for(i=0;i<tagBitSize;i++){
    int bitValue =(memoryAddress) & 1;
    if(bitValue==1){
      tagBitValue+=bitValue*pow(2,i);
      powerValue++;
    }else{
      powerValue++;
    }
    memoryAddress=(memoryAddress>>1);
  }
    return tagBitValue;
}
int associatvity;
int numberOfSets;
int setIndex;
int indexBits;
int offsetSize;
int tagSize;

int main(int argc, char* argv[]){
  if(argc < 6){
    printf("error\n");
    return 0;
  }
  int cacheSize=atoi(argv[1]);

  if(powerOfTwo(cacheSize)==false){
     printf("error.\n");
     return 0;
   }


  char* associativityStyle = argv[2];


  char* cachePolicy=argv[3];
  if(strcmp(cachePolicy,"lru")!=0){
     printf("error\n");
     return 0;
  }


  int blockSize=atoi(argv[4]);

  if(powerOfTwo(blockSize)==false){
     printf("error.\n");
     return 0;
  }


  char* traceFile=argv[5];
//  printf("cache size: %d\nassociativity: %s\ncache policy: %s\nblock size: %d\ntrace file: %s\n\n\n",cacheSize,associativityStyle,cachePolicy,blockSize,traceFile);
  if(strcmp(associativityStyle,"direct")==0){
    associatvity=1;
    numberOfSets=cacheSize/blockSize;
    indexBits=log2(numberOfSets);
    offsetSize=log2(blockSize);
    //printf("index size: %d\n",indexBits);
  //  printf("offset size: %d\n",offsetSize);
    tagSize=48-(indexBits+offsetSize);
  }else if(strcmp(associativityStyle,"assoc")==0){
    //fully associative
    numberOfSets=0;
    associatvity=cacheSize/blockSize;
    indexBits=0;
    offsetSize=log2(blockSize);
    //printf("index size: %d\n",indexBits);
    //printf("offset size: %d\n",offsetSize);
    tagSize=48-(indexBits+offsetSize);
  }else if(strstr(associativityStyle,":")!=NULL){
      char c = argv[2][6];
      int value = c-'0';
      associatvity = value;
      numberOfSets=cacheSize/(associatvity*blockSize);
    //  printf("number of sets: %d\n",numberOfSets);
      indexBits=log2(numberOfSets);
      offsetSize=log2(blockSize);
      //printf("index size: %d\n",indexBits);
      //printf("offset size: %d\n",offsetSize);
      tagSize=48-(indexBits+offsetSize);
  }
//  printf("\n\n\n\n");

  long nonPrefetchReads=0;
  long nonPrefetchWrites=0;
  long nonPrefetchCacheHits=0;
  long nonPrefetchCacheMiss=0;

  unsigned long uselessInfo;
  char readOrWrite;
  unsigned long memoryAddress;
  FILE *fileReader;
  fileReader=fopen(traceFile,"r");
  if(fileReader==NULL){
    printf("file not found\n");
    return 0;
  }
  int j;
  int foundPosition=0;
  //printf("number of sets (vertical of 2d array): %d\n", numberOfSets+1);
  //printf("associativity (horizontal of 2d array): %d\n", associatvity);

  struct line** cache=(struct line**)calloc((numberOfSets+1), sizeof(struct line*));
  for(j=0;j<numberOfSets+1;j++){
    cache[j]=(struct line*)calloc(associatvity,sizeof(struct line));
  }


  while(fscanf(fileReader,"%lx: %c %lx\n",&uselessInfo,&readOrWrite,&memoryAddress)==3){

    long indexBitValue = indexCalc(memoryAddress,offsetSize,indexBits);
    long tagBitValue = tagBitCalc(memoryAddress,offsetSize, indexBits,tagSize);


    int fullCheck=1;//if full, this should be 1
    for(j=0;j<associatvity;j++){
      if(cache[indexBitValue][j].validbit==0){
        fullCheck=0;
        break;
      }
    }

    int i;
    if(readOrWrite=='R'){
      int flag=0;
      for(i=0;i<associatvity;i++){
        if(cache[indexBitValue][i].validbit==1){
          if(cache[indexBitValue][i].tag==tagBitValue){
            //you have a cache hit
            //save the index i
            foundPosition=i;
            flag=1;
          }
        }
      }
      if(flag==1){
        //you would have a cache hit
        //set this indexes order to 1
        cache[indexBitValue][foundPosition].order=1;
        //increment everything but foundPosition
        for(j=0;j<associatvity;j++){
          if(j!=foundPosition){
            cache[indexBitValue][j].order++;
          }
        }
        nonPrefetchCacheHits++;
      }else if(flag==0 && fullCheck==1){
            //flag means the tag isn't in the set, it's a cache miss, so now I need to do a cache write
            //read cache miss
            nonPrefetchCacheMiss++;
            nonPrefetchReads++;
            int max=cache[indexBitValue][0].order;
            for(j=1;j<associatvity;j++){
              if(cache[indexBitValue][j].order > max){
                max=cache[indexBitValue][j].order;
              }
            }
            for(j=0;j<associatvity;j++){
              if(cache[indexBitValue][j].order==max){
                break;
              }
            }
            struct line newLine={1,tagBitValue,1};
            cache[indexBitValue][j]=newLine;
            int addedValue=j;
            for(j=0;j<associatvity;j++){
              if(j!=addedValue){
                cache[indexBitValue][j].order++;
              }
            }
      }else{
        //you look into the set and it is not there
        nonPrefetchCacheMiss++;
        //find the next position in the set to use that has a zero
        int j;
        for(j=0;j<associatvity;j++){
          if(cache[indexBitValue][j].validbit==0){
            break;
          }
        }
        //j has the value as to where to write
        nonPrefetchReads++;
        struct line newLine={1,tagBitValue,1};

        cache[indexBitValue][j]=newLine;
        int addedValue=j;
        for(j=0;j<associatvity;j++){
          if(j!=addedValue){
            cache[indexBitValue][j].order++;
          }
        }


      }

    }else if(readOrWrite=='W'){
      int flag=0;
      for(i=0;i<associatvity;i++){
        if(cache[indexBitValue][i].validbit==1){
          if(cache[indexBitValue][i].tag==tagBitValue){
            //you have a cache hit
            //save the index i
            foundPosition=i;
            flag=1;
          }
        }
      }

      if(flag==1){
        //write cache hit
        cache[indexBitValue][foundPosition].order=1;
        for(j=0;j<associatvity;j++){
          if(j!=foundPosition){
            cache[indexBitValue][j].order++;
          }
        }
        nonPrefetchCacheHits++;
        nonPrefetchWrites++;
      }else if(flag==0 && fullCheck==1){
        //we need to do a write miss but now we need to evict and rewrite
        nonPrefetchCacheMiss++;
        nonPrefetchReads++;
        nonPrefetchWrites++;

        //now I need to find the largest order position
        int max=cache[indexBitValue][0].order;
        for(j=1;j<associatvity;j++){
          if(cache[indexBitValue][j].order > max){
            max=cache[indexBitValue][j].order;
          }
        }
        for(j=0;j<associatvity;j++){
          if(cache[indexBitValue][j].order==max){
            break;
          }
        }
        struct line newLine={1,tagBitValue,1};
        cache[indexBitValue][j]=newLine;

        int addedValue=j;
        for(j=0;j<associatvity;j++){
          if(j!=addedValue){
            cache[indexBitValue][j].order++;
          }
        }

      }else{
        nonPrefetchCacheMiss++;
        nonPrefetchReads++;
        nonPrefetchWrites++;
        int j;
        for(j=0;j<associatvity;j++){
          if(cache[indexBitValue][j].validbit==0){
            break;
          }
        }
        struct line newLine={1,tagBitValue,1};

        cache[indexBitValue][j]=newLine;
        int addedValue=j;
        for(j=0;j<associatvity;j++){
          if(j!=addedValue){
            cache[indexBitValue][j].order++;
          }
        }
      }

    }
  }
/*
  printf("tag\torder\n");
  for(j=0;j<associatvity;j++){
    printf("%d\t%d\n",cache[0][j].tag,cache[0][j].order);
  }
*/


/*

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          THE FINAL FRONTIER
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/






  printf("no-prefetch\n");
  printf("Memory reads: %d\n",nonPrefetchReads);
  printf("Memory writes: %d\n", nonPrefetchWrites);
  printf("Cache hits: %d\n", nonPrefetchCacheHits);
  printf("Cache misses: %d\n", nonPrefetchCacheMiss);
/*
  struct line** prefetchCache=(struct line**)calloc((numberOfSets+1), sizeof(struct line*));
  for(j=0;j<numberOfSets+1;j++){
    prefetchCache[j]=(struct line*)calloc(associatvity,sizeof(struct line));
  }
  long prefetchReads=0;
  long prefetchWrites=0;
  long prefetchCacheHits=0;
  long prefetchCacheMiss=0;

  unsigned long garbageInfo;
  char prefetchReadOrWrite;
  unsigned long originalMemoryAddress;
  FILE *fileReader2;
  fileReader2=fopen(traceFile,"r");
  if(fileReader2==NULL){
    printf("file not found\n");
    return 0;
  }

  while(fscanf(fileReader2,"%lx: %c %lx\n",&garbageInfo,&prefetchReadOrWrite,&originalMemoryAddress)==3){
    long indexBitValue = indexCalc(originalMemoryAddress,offsetSize,indexBits);
    long tagBitValue = tagBitCalc(originalMemoryAddress,offsetSize, indexBits,tagSize);
    printf("original memory address located at: %d\n",indexBitValue);
    printf("original memory address tag value: %d\n",tagBitValue);

    unsigned long prefetchAddress = (originalMemoryAddress+blockSize);

    long prefetchIndexValue = indexCalc(prefetchAddress, offsetSize, indexBits);
    long prefetchTagBitValue = tagBitCalc(prefetchAddress, offsetSize, indexBits, tagSize);
    printf("prefetch memory address located at: %d\n",prefetchIndexValue);
    printf("prefetch memory address tag value: %d\n",prefetchTagBitValue);

    if(prefetchReadOrWrite=='R'){
      int flag=0;
      int i;
      for(i=0;i<associatvity;i++){
        if(prefetchCache[indexBitValue][i].validbit==1){
          if(prefetchCache[indexBitValue][i].tag==tagBitValue){
            //you have a cache hit
            //save the index i
            foundPosition=i;
            flag=1;
          }
        }
      }

      if(flag==1){
        prefetchCache[indexBitValue][foundPosition].order=1;
        //increment everything but foundPosition
        for(j=0;j<associatvity;j++){
          if(j!=foundPosition){
            prefetchCache[indexBitValue][j].order++;
          }
        }
        prefetchCacheHits++;
    }else if(flag==0){
      //check if the set is full
      int fullCheck=1;
      for(j=0;j<associatvity;j++){
        if(prefetchCache[indexBitValue][j].validbit==0){
          fullCheck=0;
          break;
        }
      }
      printf("\n\n\nPREFETCH\n\n\n");

      printf("the set full value is: %d\n", fullCheck);
              if(fullCheck==0){
                  //there is a place where you can write to
                  prefetchCacheMiss++;
                  //find the next position in the set to use that has a zero
                  int j;
                  for(j=0;j<associatvity;j++){
                    if(prefetchCache[indexBitValue][j].validbit==0){
                      break;
                    }
                  }
                  //j has the value as to where to write
                  prefetchReads++;
                  struct line newLine={1,tagBitValue,1};

                  prefetchCache[indexBitValue][j]=newLine;
                  int addedValue=j;
                  for(j=0;j<associatvity;j++){
                    if(j!=addedValue){
                      prefetchCache[indexBitValue][j].order++;
                    }
                  }
                  printf("valid bit\ttag\torder\tset: %d\n",indexBitValue);
                  for(j=0;j<associatvity;j++){
                    printf("%d\t%d\t%d\n",cache[0][j].validbit,cache[0][j].tag,cache[0][j].order);
                  }
              }else if(fullCheck==1){
                    //now you need to evict
                    prefetchCacheMiss++;
                    prefetchReads++;
                    int max=prefetchCache[indexBitValue][0].order;
                    for(j=1;j<associatvity;j++){
                      if(prefetchCache[indexBitValue][j].order > max){
                        max=prefetchCache[indexBitValue][j].order;
                      }
                    }
                    for(j=0;j<associatvity;j++){
                      if(prefetchCache[indexBitValue][j].order==max){
                        break;
                      }
                    }
                    struct line newLine={1,tagBitValue,1};
                    prefetchCache[indexBitValue][j]=newLine;
                    int addedValue=j;
                    for(j=0;j<associatvity;j++){
                      if(j!=addedValue){
                        prefetchCache[indexBitValue][j].order++;
                      }
                    }
              }



              //step 1: check if prefetch tag is in the set
              int doesPrefetchExist=0;
              printf("valid bit\ttag\torder\tset: %d\n",prefetchIndexValue);
              for(j=0;j<associatvity;j++){
                printf("%d\t%d\t%d\n",prefetchCache[prefetchIndexValue][j].validbit,prefetchCache[prefetchIndexValue][j].tag,prefetchCache[prefetchIndexValue][j].order);
              }
              for(j=0;j<associatvity;j++){
                if(prefetchCache[prefetchIndexValue][j].tag==prefetchTagBitValue){
                  doesPrefetchExist=1;
                  break;
                }
              }
              printf("prefetch check: %d\n",doesPrefetchExist);


              int prefetchFullCheck=1;
              for(j=0;j<associatvity;j++){
                if(prefetchCache[prefetchIndexValue][j].validbit==0){
                  prefetchFullCheck=0;
                  break;
                }
              }
                printf("prefetch full check: %d\n",prefetchFullCheck);


              if(doesPrefetchExist==0){
                prefetchReads++;
                if(prefetchFullCheck==0){

                    //there is a place where you can write to
                    //find the next position in the set to use that has a zero
                    int j;
                    for(j=0;j<associatvity;j++){
                      if(prefetchCache[prefetchIndexValue][j].validbit==0){
                        break;
                      }
                    }

                    //j has the value as to where to write
                    struct line newLine={1,prefetchTagBitValue,1};
                    prefetchCache[prefetchIndexValue][j]=newLine;
                    printf("valid bit\ttag\torder\tset: %d\n",prefetchIndexValue);
                    for(j=0;j<associatvity;j++){
                      printf("%d\t%d\t%d\n",prefetchCache[prefetchIndexValue][j].validbit,prefetchCache[prefetchIndexValue][j].tag,prefetchCache[prefetchIndexValue][j].order);
                    }
                    int addedValue=j;

                    for(j=0;j<associatvity;j++){
                      if(j!=addedValue){
                        prefetchCache[prefetchIndexValue][j].order++;
                      }
                    }
                    printf("valid bit\ttag\torder\tset: %d\n",prefetchIndexValue);
                    for(j=0;j<associatvity;j++){
                      printf("%d\t%d\t%d\n",prefetchCache[prefetchIndexValue][j].validbit,prefetchCache[prefetchIndexValue][j].tag,prefetchCache[prefetchIndexValue][j].order);
                    }
                }else if(prefetchFullCheck==1){
                      //now you need to evict
                      int max=prefetchCache[prefetchIndexValue][0].order;
                      for(j=1;j<associatvity;j++){
                        if(prefetchCache[prefetchIndexValue][j].order > max){
                          max=prefetchCache[prefetchIndexValue][j].order;
                        }
                      }
                      for(j=0;j<associatvity;j++){
                        if(prefetchCache[prefetchIndexValue][j].order==max){
                          break;
                        }
                      }                //now I need to add in the prefetch block

                      struct line newLine={1,prefetchTagBitValue,1};
                      prefetchCache[prefetchIndexValue][j]=newLine;
                      int addedValue=j;
                      for(j=0;j<associatvity;j++){
                        if(j!=addedValue){
                          prefetchCache[prefetchIndexValue][j].order++;
                        }
                      }
                }

              }
    }

  }else if(prefetchReadOrWrite=='W'){
    int flag=0;            //flag means the tag isn't in the set, it's a cache miss, so now I need to do a cache write
    int i;
    for(i=0;i<associatvity;i++){
      if(prefetchCache[indexBitValue][i].validbit==1){
        if(prefetchCache[indexBitValue][i].tag==tagBitValue){
          //you have a cache hit
          //save the index i
          foundPosition=i;
          flag=1;
        }
      }
    }

    if(flag==1){
      //write cache hit
      prefetchCache[indexBitValue][foundPosition].order=1;
      for(j=0;j<associatvity;j++){
        if(j!=foundPosition){
          prefetchCache[indexBitValue][j].order++;
        }
      }
      prefetchCacheHits++;
      prefetchWrites++;
    }else if(flag==0){
      //flag means you can't find the original memory address

        int fullCheck=1;
        for(j=0;j<associatvity;j++){
          if(prefetchCache[indexBitValue][j].validbit==0){
            fullCheck=0;
            break;
          }
        }

        if(fullCheck==0){
            //there is a place where you can write to
            prefetchCacheMiss++;
            //find the next position in the set to use that has a zero
            int j;
            for(j=0;j<associatvity;j++){
              if(prefetchCache[indexBitValue][j].validbit==0){
                break;
              }
            }
            //j has the value as to where to write
            prefetchReads++;
            struct line newLine={1,prefetchTagBitValue,1};
            prefetchWrites++;
            prefetchCache[indexBitValue][j]=newLine;
            int addedValue=j;
            for(j=0;j<associatvity;j++){
              if(j!=addedValue){
                prefetchCache[indexBitValue][j].order++;
              }
            }
        }else if(fullCheck==1){
              //now you need to evict
              prefetchCacheMiss++;
              prefetchReads++;
              prefetchWrites++;
              int max=prefetchCache[indexBitValue][0].order;
              for(j=1;j<associatvity;j++){
                if(prefetchCache[indexBitValue][j].order > max){
                  max=prefetchCache[indexBitValue][j].order;
                }
              }
              for(j=0;j<associatvity;j++){
                if(prefetchCache[indexBitValue][j].order==max){
                  break;
                }
              }
              struct line newLine={1,prefetchTagBitValue,1};
              prefetchCache[indexBitValue][j]=newLine;
              int addedValue=j;
              for(j=0;j<associatvity;j++){
                if(j!=addedValue){
                  prefetchCache[indexBitValue][j].order++;
                }
              }
            }


            //NOW WORRY ABOUT PREFETCH SHIT
            int doesPrefetchExist=0;
            for(j=0;j<associatvity;j++){
              if(prefetchCache[prefetchIndexValue][j].tag==prefetchTagBitValue){
                doesPrefetchExist=1;
                break;
              }
            }
            /*
            doesPrefetchExist = 0 means prefetch does not exist in the set
            doesPrefetchExist = 1 means prefetch does exist in the set
            */
/*
            if(doesPrefetchExist==0){
              prefetchReads++;
              if(fullCheck==0){
                  //there is a place where you can write to
                  //find the next position in the set to use that has a zero
                  int j;
                  for(j=0;j<associatvity;j++){
                    if(prefetchCache[prefetchIndexValue][j].validbit==0){
                      break;
                    }
                  }
                  //j has the value as to where to write
                  struct line newLine={1,prefetchTagBitValue,1};
                  prefetchCache[prefetchIndexValue][j]=newLine;
                  int addedValue=j;
                  for(j=0;j<associatvity;j++){
                    if(j!=addedValue){
                      prefetchCache[prefetchIndexValue][j].order++;
                    }
                  }
              }else if(fullCheck==1){
                    //now you need to evict
                    int max=prefetchCache[prefetchIndexValue][0].order;
                    for(j=1;j<associatvity;j++){
                      if(prefetchCache[prefetchIndexValue][j].order > max){
                        max=prefetchCache[prefetchIndexValue][j].order;
                      }
                    }
                    for(j=0;j<associatvity;j++){
                      if(prefetchCache[prefetchIndexValue][j].order==max){
                        break;
                      }
                    }                //now I need to add in the prefetch block

                    struct line newLine={1,prefetchTagBitValue,1};
                    prefetchCache[prefetchIndexValue][j]=newLine;
                    int addedValue=j;
                    for(j=0;j<associatvity;j++){
                      if(j!=addedValue){
                        prefetchCache[prefetchIndexValue][j].order++;
                      }
                    }
              }



    }


  }
}
}



printf("with-prefetch\n");
printf("Memory reads: %d\n",prefetchReads);
printf("Memory writes: %d\n", prefetchWrites);
printf("Cache hits: %d\n", prefetchCacheHits);
printf("Cache misses: %d\n", prefetchCacheMiss);
*/
}
