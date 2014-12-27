/*
 * test.c
 *
 *  Created on: Mar 11, 2014
 *      Author: sw
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Space(s) (s== ' ' || s == '\n' || s == '\t')
#define SkipComment while((c = getc(f)) != '\n')
#define false 0
#define true 1
#define BrDiscr 1
#define ThreshContin 2
#define BrSubset 3
#define  Unknown  -999
#define Inc 2048

#define  CVal(Case,Attribute)   Case[Attribute]._cont_val
#define  DVal(Case,Attribute)   Case[Attribute]._discr_val
#define  Class(Case)		Case[MaxAtt+1]._discr_val
#define	 Bit(b)			(1 << (b))
#define	 In(b,s)		((s[(b) >> 3]) & Bit((b) & 07))

#define IGNORE 1
#define DISCRETE 2

typedef char* String;
typedef unsigned char Boolean, *Set, Byte;
typedef short ClassNo, DiscrValue, Attribute;
typedef float ItemCount;
typedef float ContValue;
typedef  int	ItemNo;		/* data item number */
typedef struct _treerec *Tree;
typedef struct _treerec{
    short NodeType;
    ClassNo Leaf;
    ItemCount Items, * ClassDist, Errors;
    Attribute Tested;
    short Forks;
    ContValue Cut, Lower, Upper;
    Set *Subset;
    Tree *Branch;
}TreeRec;

typedef  union  _attribute_value
	 {
	    DiscrValue	_discr_val;
	    float	_cont_val;
	 }
	 	AttValue, *Description;

FILE* file;
String *ClassName, *AttName = NULL, **AttValName = NULL;
char *SpecialStatus = NULL;
char Delimiter;
int MaxClass = 0, MaxAtt = 0;
short MaxDiscrVal = 2;
DiscrValue	*MaxAttVal=0;
float	*ClassSum = NULL;		/* ClassSum[c] = total weight of class c */
Description	*Item;
ItemNo		MaxItem;


void StreamIn(char* s, int n)
{
/*    if(file == NULL)
    {
        printf("Please open tree file first!");
        return;
    }*/
    while(n--)*s++ = getc(file);
}

int Which(String Val, String List[], short First, short Last)

{
    short n=First;
    while ( n <= Last && strcmp(Val, List[n]) )n++;

    return ( n <= Last ? n : First-1 );
}


String CopyString(String x)
{
    char *s;
    s = (char*)malloc((strlen(x) + 1) * sizeof(char));
    strcpy(s,x);
    return s;
}

Boolean ReadName(FILE* f, String s)
{
    register char *Sp = s;
    register int c;
    while((c = getc(f)) == '|' || Space(c))
    {
        if(c == '|')SkipComment;
    }

    if(c == EOF)
    {
        Delimiter = EOF;
        return false;
    }

    while(c != ':' && c != ',' && c != '\n' && c != '|' && c!= EOF)
    {
        if(c == '.')
        {
            if((c = getc(f)) == '|' || Space(c))break;
            *Sp++ = '.';
        }

        if(c == '\\')
        {
            c = getc(f);
        }

        *Sp++ = c;

        if(c == ' ')
        {
            while((c = getc(f)) == ' ');
        }else{
            c = getc(f);
        }
    }

    if( c == '|') SkipComment;
    Delimiter = c;
    while(Space(*(Sp - 1))) Sp--;
    *Sp++ = '\0';
    return true;
}

void GetNames()
{
    FILE *Nf;
    char Buffer[1000];
    DiscrValue v;
    int AttCeiling = 100, ClassCeiling = 100, ValCeiling;
    if(!(Nf = fopen("fallDetect.names","r")))
    {
        printf("FallDetect.names file missing!");
        return;
    }

    ClassName = (String*)calloc(ClassCeiling, sizeof(String));
    MaxClass = -1;
    do{
        ReadName(Nf, Buffer);

        if(++MaxClass >= ClassCeiling)
        {
            ClassCeiling += 100;
            ClassName = (String*)realloc(ClassName, ClassCeiling * sizeof(String));
        }
        ClassName[MaxClass] = CopyString(Buffer);
    }while(Delimiter == ',');

    AttName = (String*)calloc(AttCeiling, sizeof(String));
    MaxAttVal = (DiscrValue*)calloc(AttCeiling, sizeof(DiscrValue));
    AttValName = (String**)calloc(AttCeiling, sizeof(String*));
    SpecialStatus = (char*)malloc(AttCeiling);

    MaxAtt = -1;
    while(ReadName(Nf, Buffer))
    {
        if(Delimiter != ':')
        {
            printf("Format is wrong!");
            exit(1);
        }

        if(++MaxAtt >= AttCeiling)
        {
            AttCeiling += 100;
            AttName = (String*)realloc(AttName, AttCeiling * sizeof(String));
            MaxAttVal = (DiscrValue *)realloc(MaxAttVal, AttCeiling * sizeof(DiscrValue));
            AttValName = (String**)realloc(AttValName, AttCeiling * sizeof(String *));
            SpecialStatus = (char*)realloc(SpecialStatus, AttCeiling);
        }

        AttName[MaxAtt] = CopyString(Buffer);
        SpecialStatus[MaxAtt] = 0;
        MaxAttVal[MaxAtt] = 0;
        ValCeiling = 100;
        AttValName[MaxAtt] = (String*)calloc(ValCeiling, sizeof(String));

        do{
            if(!(ReadName(Nf, Buffer)))
            {
                printf("ReadName wrong!");
                exit(1);
            }

            if(++MaxAttVal[MaxAtt] >= ValCeiling)
            {
                ValCeiling += 100;
                AttValName[MaxAtt] = (String*)realloc(AttValName[MaxAtt], ValCeiling * sizeof(String));
            }

            AttValName[MaxAtt][MaxAttVal[MaxAtt]] = CopyString(Buffer);
        }while(Delimiter == ',');

        if(MaxAttVal[MaxAtt] == 1)
        {
			if(!strcmp(Buffer, "continuous"))
			{}
			else if(!memcmp(Buffer, "discrete", 8)){
				SpecialStatus[MaxAtt] = DISCRETE;
				v = atoi(&Buffer[8]);
				if(v < 2)
				{
					printf("**%s: illegal number of discrete values\n", AttName[MaxAtt]);
					exit(1);
				}

				AttValName[MaxAtt] = (String*)realloc(AttValName[MaxAtt], (v + 2) * sizeof(String));
				AttValName[MaxAtt][0] = (char*)v;
				if(v > MaxDiscrVal)MaxDiscrVal = v;
			}else if(!strcmp(Buffer, "ignore"))
			{
				SpecialStatus[MaxAtt] = IGNORE;
			}else{

				printf("Cannot have only one discrete value for an attribute!");
				exit(1);
			}
			MaxAttVal[MaxAtt] = 0;
		}else if(MaxAttVal[MaxAtt] > MaxDiscrVal){
			MaxDiscrVal = MaxAttVal[MaxAtt];
		}
    }
    fclose(Nf);
}
int i = 0;
Tree InTree()
{
    int Bytes;
    DiscrValue v;
    Tree T = (Tree)malloc(sizeof(TreeRec));
    StreamIn((char*)&T->NodeType, sizeof(short));
    StreamIn((char*)&T->Leaf, sizeof(ClassNo));
    StreamIn((char*)&T->Items, sizeof(ItemCount));
    StreamIn((char*)&T->Errors, sizeof(ItemCount));

    //if(i == 0)
    //printf("%d   %d   %f   %f\n", T->NodeType, T->Leaf, T->Items, T->Errors);
    //i++;
    T->ClassDist = (ItemCount*)calloc(MaxClass +1, sizeof(ItemCount));
    StreamIn((char*)T->ClassDist, (MaxClass + 1) * sizeof(ItemCount));

    //printf("MaxClass:%d\n", MaxClass);
    if(T->NodeType){
        StreamIn((char*)&T->Tested, sizeof(Attribute));
        StreamIn((char*)&T->Forks, sizeof(short));

        switch(T->NodeType)
        {
            case BrDiscr:
                break;
            case ThreshContin:
                StreamIn((char*)&T->Cut, sizeof(float));
                StreamIn((char*)&T->Lower, sizeof(float));
                StreamIn((char*)&T->Upper, sizeof(float));
                break;
            case BrSubset:
                T->Subset = (Set*)calloc(T->Forks + 1, sizeof(Set));
                Bytes = (MaxAttVal[T->Tested]>>3) + 1;
                for(v = 1; v <= T->Forks; ++v)
                {
                    T->Subset[v] = (Set)malloc(Bytes);
                    StreamIn((char*)T->Subset[v], Bytes);
                }
        }

        T->Branch = (Tree*)calloc(T->Forks + 1, sizeof(Tree));
        for(v = 1; v <= T->Forks; ++v)
        {
            T->Branch[v] = InTree();
        }
    }

    return T;

}

void Classify(Description CaseDesc, Tree T, float Weight)
{
	DiscrValue v,dv;
	float Cv;
	Attribute a;
	ClassNo c;
	float x;

	switch(T->NodeType)
	{
	    case 0:
	    	if(T->Items > 0)
	    	{
	    		for (c = 0; c <= MaxClass; c++)
	    		{
	    			x = T->ClassDist[c];
	    			if(x)//T->ClassDist[c])
	    			{
	    				ClassSum[c] += Weight * x / T->Items;
	    			}
	    		}
	    	}else{
	    		ClassSum[T->Leaf] += Weight;
	    	}
	    	return;

	    case BrDiscr:
	    	a = T->Tested;
	    	v = DVal(CaseDesc, a);

	    	if(v && v <= T->Forks)
	    	{
	    		Classify(CaseDesc, T->Branch[v], Weight);
	    	}else{
	    		for(v = 1; v <= T->Forks; v++)
	    		{
	    			Classify(CaseDesc, T->Branch[v], (Weight * T->Branch[v]->Items) / T->Items);
	    		}
	    	}
	    	return;

	    case ThreshContin:
	    	a = T->Tested;
	    	Cv = CVal(CaseDesc, a);

	    	if(Cv == Unknown)
	    	{
	    		for(v = 1; v <= 2; v++)
	    		{
	    			Classify(CaseDesc, T->Branch[v], (Weight * T->Branch[v]->Items) / T->Items);
	    		}
	    	}else{
	    		v  = (Cv <= T->Cut ? 1 :2);
	    		Classify(CaseDesc, T->Branch[v], Weight);
	    	}
	    	return;

	    case BrSubset:
	    	a = T->Tested;
	    	dv = DVal(CaseDesc, a);
	    	if(dv)
	    	{
	    		for (v = 1; v <= T->Forks; v++)
	    		{
	    			if(In(dv, T->Subset[v]))
	    			{
	    				Classify(CaseDesc, T->Branch[v], Weight);
	    				return;
	    			}
	    		}
	    	}

	    	for(v = 1; v <= T->Forks; v++)
	    	{
	    		Classify(CaseDesc, T->Branch[v], (Weight * T->Branch[v]->Items) / T->Items);
	    	}

	    	return;
	}
}

ClassNo Category(Description CaseDesc, Tree DecisionTree)
{
	ClassNo c, BestClass;
	if(!ClassSum)
	{
		ClassSum = (float*)malloc ((MaxClass + 1) * sizeof(float));
	}

	for (c = 0; c <= MaxClass; c++)
	{
		ClassSum[c] = 0;
	}

	Classify(CaseDesc, DecisionTree, 1.0);

	BestClass = 0;
	for(c = 0; c <= MaxClass; c++)
	{
		if(ClassSum[c] > ClassSum[BestClass]) BestClass = c;
	}

	return BestClass + 1;
}

Description GetDescription(FILE* Df)
{
	Attribute Att;
	char name[500], *endname;
	int Dv;
	float Cv;
	Description Dvec;

	if(ReadName(Df, name))
	{
		Dvec = (Description)calloc(MaxAtt + 2, sizeof(AttValue));

		for(Att = 0; Att <= MaxAtt; Att++)    //drop the class here, Att < MaxAtt;
		{
			if(SpecialStatus[Att] == IGNORE)
			{
				DVal(Dvec, Att) = 0;
			}else if(MaxAttVal[Att] || SpecialStatus[Att] == DISCRETE)
			{
				if(!(strcmp(name, "?")))
				{
					Dv = 0;
				}else{
					Dv = Which(name, AttValName[Att], 1, MaxAttVal[Att]);
					if(!Dv)
					{
						if(SpecialStatus[Att] == DISCRETE)
						{
							Dv = ++ MaxAttVal[Att];
							if(Dv > (int)AttValName[Att][0])
							{
								printf("Too many values for %s (max %d)\n", AttName[Att], (int) AttValName[Att][0]);
								exit(1);
							}
							AttValName[Att][Dv] = CopyString(name);
						}else{
							printf("There is something wrong1!\n");
						}
					}
				}
				DVal(Dvec, Att) = Dv;
			}else{
				if(!(strcmp(name, "?")))
				{
					Cv = Unknown;
				}else{
					Cv = strtod(name, &endname);
					if(endname == name || *endname != '\0')
					{
						printf("i:%d   There is something wrong2!\n", i);
					}
				}
				CVal(Dvec, Att) = Cv;
			}
			ReadName(Df, name);
		}
		i++;
		if((Dv = Which(name, ClassName, 0, MaxClass)) < 0)
		{
			printf("i:%d   There is something wrong3!\n", i);
			Dv = 0;
		}
		Class(Dvec) = Dv;    //drop the class here
		return Dvec;
	}else{
		return NULL;
	}
}

void GetData()
{
	FILE* Df;
	ItemNo i = 0, ItemSpace = 0;
	if(!(Df = fopen("fallDetect.cases", "r")))
	{
		printf("Open fallDetect.cases wrong!");
		exit(1);
	}

	do{
		MaxItem = i;

		if(i >= ItemSpace)
		{
			if(ItemSpace)
			{
				ItemSpace += Inc;
				Item = (Description *)realloc(Item, ItemSpace * sizeof(Description));
			}else{
				Item = (Description *)malloc((ItemSpace = Inc) * sizeof(Description));
			}
		}

		Item[i] = GetDescription(Df);
	}while(Item[i] != NULL && ++i);

	fclose(Df);
	MaxItem = i - 1;
}

int main(int argc, char* argv[])
{
	GetNames();
	printf("%d\n", MaxAtt);
    file = fopen("fallDetect.tree", "rb");
    if(!file || getc(file) == EOF)
    {
    	printf("There  something wrong!");
    	exit(1);
    }
    Tree T = InTree();
    fclose(file);
    GetData();
    for (i = 0; i <= MaxItem; i++)
    {
    	ClassNo class = Category(Item[i], T);
    	printf("%d %d\n", i, class);
    }
    return 0;
}

