#include <stdio.h>
#include <stdlib.h>

#include "ggggc/gc.h"

GGC_TYPE(LinkedList)
    GGC_MPTR(LinkedList, next);
    GGC_MDATA(int, val);
GGC_END_TYPE(LinkedList, 
    GGC_PTR(LinkedList, next)
    )

#define MAX (100000000)

LinkedList makeList(int counter)
{
    int i;
    LinkedList ob2 = NULL, ob3 = NULL, ob4 = NULL , ob5=NULL;

    GGC_PUSH_4(ob2, ob3, ob4, ob5);
 	printf("calling malloc\n");
    
    ob5 = GGC_NEW(LinkedList);
    ob2 = GGC_NEW(LinkedList);
  //  printf("here obj is %u\n",ob2);
    GGC_WD(ob2, val, 0);
    ob3 = ob2;
    ob5=NULL;

    for (i = 1; i < 3; i++) {
        ob4 = GGC_NEW(LinkedList);
        //printf("here obj is %zx\n",ob4);
        GGC_WD(ob4, val, i);
        GGC_WP(ob3, next, ob4);
        ob3 = ob4;
//        ob5=ob3;
        GGC_YIELD();
    }

    return ob2;
}

int main(void)
{
    LinkedList ob1 = NULL;
	printf("calling main\n");
    GGC_PUSH_1(ob1);

    ob1 = makeList(MAX);

    return 0;
}
