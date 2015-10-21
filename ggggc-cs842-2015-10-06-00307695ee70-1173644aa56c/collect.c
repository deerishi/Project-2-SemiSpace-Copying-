/*
 * The collector
 *
 * Copyright (c) 2014, 2015 Gregor Richards
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ggggc/gc.h"
#include "ggggc-internals.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXSIZE 1024
struct CopyList
{
	void **list;
	ggc_size_t numPointers;
	struct CopyList *next,*prev;
};

static struct CopyList LIST;

#define CopyListInit()do{\
\
	if(LIST.list==NULL)\
	{\
		LIST.list=malloc(MAXSIZE * sizeof(void *));\
		if(LIST.list == NULL )\
		{\
			perror("malloc");\
			abort();\
		}\
	}\
	copyList=&LIST;\
	copyList->numPointers=0;\
	copyList->next=NULL;\
	copyList->prev=NULL;\
}while(0);

#define NewList()do{\
\
	struct CopyList *LIST2=malloc(sizeof(struct CopyList));\
	if( LIST2 == NULL )\
	{\
		perror("malloc");\
		abort();\
	}\
	LIST2->list=malloc(MAXSIZE * sizeof(void *));\
	if(LIST.list == NULL )\
	{\
		perror("malloc");\
		abort();\
	}\
	LIST2->numPointers=0;\
	copyList->prev=copyList;\
	copyList->next=LIST2;\
	copyList=copyList->next;\
}while(0);


#define AddToCopyList(ptr) do{\
\
	if( copyList->numPointers >= MAXSIZE )\
	{\
		NewList();\
	}\
	copyList->list[copyList->numPointers++]=ptr;\
}while(0);
	
	
#define CopyListPop(ptr) do{\
	ptr=(void **)copyList->list[--copyList->numPointers];\
	if(copyList->numPointers == 0 && copyList->prev)\
	{\
		copyList=copyList->prev;\
	}\
}while(0);



void ggggc_copy(void **location)
{
	 struct GGGGC_Header *fromLoc= ( struct GGGGC_Header *)*location;
	 printf("\tfromloc is %zx  \n",fromLoc);
	if( fromLoc != NULL )
	{
	    printf("\tfromloc des is %zx\n",fromLoc->descriptor__ptr);
		*location=ggggc_forward(fromLoc);
	}

}


struct CopyList *copyList;

void  *ggggc_forward(struct GGGGC_Header * obj)
{
	CopyListInit();
	
	//Checking if the object was already forwarded
	printf("obj->forward is %zx \n",obj->forward);
	if( obj->forward != NULL )
	{
		return obj->forward;
	}
	
	//Now we copy the object from FromSpace to ToSpace
	struct GGGGC_Pool *copyToPool=toSpaceCurPool;
	struct GGGGC_Header *objInToPool;
	while(copyToPool)
	{
		if( copyToPool->end - copyToPool->free >= obj->descriptor__ptr->size )
		{
			memcpy(copyToPool->free,obj,obj->descriptor__ptr->size);
			objInToPool=(struct GGGGC_Header *)copyToPool->free;
			objInToPool->descriptor__ptr=obj->descriptor__ptr;
			obj->forward=objInToPool; //We make the user ptr NUll here so that we dont mark this as the forwarded object in the next GC cyle.
			objInToPool->forward=NULL;
			printf("earliar object was %zx , now it is %zx \n",obj,objInToPool);
			printf("earliar object-des was %zx , now it is %zx \n",obj->descriptor__ptr,objInToPool->descriptor__ptr->header);
			printf("earliar object-des-size was %zx , now it is %zx \n",obj->descriptor__ptr->size,objInToPool->descriptor__ptr->size);
			objInToPool->forward= copyToPool->free;
			copyToPool->free=copyToPool->free + obj->descriptor__ptr->size;
			memset(obj+1,0,obj->descriptor__ptr->size *sizeof(ggc_size_t) -sizeof(struct GGGGC_Header *));
			//Now we have to add this copied object to  the copy list
			AddToCopyList(objInToPool);
			return objInToPool;
		}
		else
		{	
			copyToPool=copyToPool->next;
			toSpaceCurPool=copyToPool;
		}
	}

	//Implement the Else condition
	
}
		
	
	


/* run a collection */
void ggggc_collect()
{
    /* FILLME */
    //Swap the frome space and to space 
	struct GGGGC_Pool *temp;
	temp=toSpacePoolList;
	toSpacePoolList=fromSpacePoolList;
	fromSpacePoolList=temp;
	fromSpaceCurPool=toSpaceCurPool;
	toSpaceCurPool=toSpacePoolList;
    
    
    
    printf("in collect \n");
    copyList=NULL;
    CopyListInit();
    struct GGGGC_PointerStack *curPointer=ggggc_pointerStack;
    struct GGGGC_Pool *copyToPool,*copyFromPool=toSpacePoolList;
    ggc_size_t i;
    
    
    //First the references for the roots are changed
    for(curPointer=ggggc_pointerStack;curPointer!=NULL;curPointer=curPointer->next)
    {
    	for(i=0;i<curPointer->size;i++)
    	{
    	    void **t=(void **)curPointer->pointers[i];
    		printf("\n\tearliar curPointer->pointers[%zx] was %zx  \n",i,curPointer->pointers[i]);
    		ggggc_copy(curPointer->pointers[i]);
    		printf("\n\tnow curPointer->pointers[%zx] was %zx  \n",i,curPointer->pointers[i]);
    	}
    }
    
    //Now we pop out the stack made for the object references 
    while(copyList->numPointers)
    {
    	
      	void ** ptr;
      	CopyListPop(ptr);
      	struct GGGGC_Header *obj=(struct GGGGC_Header *)*ptr;
      	
      	//Do the copying operation for the object references
      	
      	ggc_size_t i,pointerMap;
		if(obj->descriptor__ptr->pointers[0] & 1 )
		{
			//Means that the descriptor containc pointers 
			void **obj2=(void **)obj;
			pointerMap=obj->descriptor__ptr->pointers[0];
			for(i=0;i<obj->descriptor__ptr->size;i++)
			{
				if(pointerMap & 1)
				{
					ggggc_copy(&obj2[i]);
				}
				pointerMap=pointerMap/2;
			}
		}
	}
	
	
      	
}

/* explicitly yield to the collector */
static ggc_size_t fc=0;
int ggggc_yield()
{
    /* FILLME */
        printf("in yield \n");
     struct GGGGC_Pool *pool=fromSpaceCurPool;
     fc++;
     if(fc>5) ggggc_collect();
    if(pool==NULL)
    {
    	ggggc_collect();
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
