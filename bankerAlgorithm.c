#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUMBER_OF_RESOURSES 3
#define NUMBER_OF_CUSTOMERS 5

int i = 0;
int j = 0;
pthread_mutex_t mutex;

int initResourceVector [NUMBER_OF_RESOURSES];
//available, max, allocation, need
int availResourceVector [NUMBER_OF_RESOURSES];
int allocMatrix [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURSES] = {{1,1,0},{1,3,0},{0,0,2},{0,1,1},{0,2,0}};
int maxMatrix [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURSES] = {{5,5,5},{3,3,6},{3,5,3},{7,1,4},{7,2,2}};
int needMatrix [NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURSES];

int requestResource(int processID,int requestVector[]);
int releaseResource(int processID,int releaseVector[]);
int ifGreaterThanNeed(int processID,int requestVector[]);
int ifEnoughToRelease(int processID,int releaseVector[]);
int ifInSafeMode();
int ifEnoughToAlloc();
void printNeedMatrix();
void printAllocMatrix();
void printAvailable();
void printReqOrRelVector(int vec[]);
void customer(void customerID);

int main(int argc, char const *argv[])
{
	if(argc != NUMBER_OF_RESOURSES + 1)
	{
		printf("Parameters number is not correct.\n");
		return -1;
	}
	for(i = 0; i < NUMBER_OF_RESOURSES; i++)
	{
		initResourceVector[i] = atoi(argv[i+1]);//argv[0] is name of program
		availResourceVector[i] = initResourceVector[i];
	}

	//initialize needMatrix
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		for (j = 0; j < NUMBER_OF_RESOURSES; ++j)
		{
			needMatrix[i][j] = maxMatrix[i][j] - allocMatrix[i][j];
		}
	}

	printf("Available resources vector is:\n");
	printAvailable();

	printf("Initial allocation matrix is:\n");
	printAllocMatrix();

	printf("Initial need matrix is:\n");
	printNeedMatrix();

	pthread_mutex_init(&mutex,NULL);//declared at head of code
	pthread_attr_t attrDefault;
	pthread_attr_init(&attrDefault);
	pthread_t *tid = malloc(sizeof(pthread_t) * NUMBER_OF_CUSTOMERS);
	int *pid = malloc(sizeof(int) * NUMBER_OF_CUSTOMERS);//customer's ID, for banker's algorithm, not pthread
	//initialize pid and create threads
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		*(pid + i) = i;
		pthread_create((tid+i), &attrDefault, customer, (pid+i));
	}
	//join threads
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		pthread_join(*(tid+i),NULL);
	}
	return 0;
}

void customer(void customerID)
{
	int processID = (int)customerID;

	while(1)
	{
		//request random number of resources
		sleep(1);
		int requestVector[NUMBER_OF_RESOURSES];

		//Because i is global variable, we should lock from here
		//lock mutex for accessing global variable and printf
		pthread_mutex_lock(&mutex);
		//initialize requestVector
		for(i = 0; i < NUMBER_OF_RESOURSES; i++)
		{
			if(needMatrix[processID][i] != 0)
			{
				requestVector[i] = rand() % needMatrix[processID][i];
			}
			else
			{
				requestVector[i] = 0;
			}
		}


		printf("Customer %d po kerkon resources:\n",processID);
		printReqOrRelVector(requestVector);
		//requestResource() will still return -1 when it fail and return 0 when succeed in allocate, like textbook says
		//altough I put the error message output part in the requestResource function
		requestResource(processID,requestVector);
		//unlock
		pthread_mutex_unlock(&mutex);

		//release random number of resources
		sleep(1);
		int releaseVector[NUMBER_OF_RESOURSES];
		//Because i is global variable, we should lock from here
		//lock mutex for accessing global variable and printf
		pthread_mutex_lock(&mutex);
		//initialize releaseVector
		for(i = 0; i < NUMBER_OF_RESOURSES; i++)
		{
			if(allocMatrix[processID][i] != 0)
			{
				releaseVector[i] = rand() % allocMatrix[processID][i];
			}
			else
			{
				releaseVector[i] = 0;
			}
		}
		printf("Customer %d po leshon resources:\n",processID);
		printReqOrRelVector(releaseVector);
		//releaseResource() will still return -1 when it fail and return 0 when succeed in allocate, like textbook says
		//altough I put the error message output part in the releaseResource function
		releaseResource(processID,releaseVector);
		//unlock
		pthread_mutex_unlock(&mutex);
	}
}

int requestResource(int processID,int requestVector[])
{
	//whether request number of resources is greater than needed
	if (ifGreaterThanNeed(processID,requestVector) == -1)
	{
		printf("Resourset e kerkuara jane me shume se sa nevojiten.\n");
		return -1;
	}
	printf("Resources e kerkuara nuk jane me shume se nevojiten.\nJapim resurset...\n");

	//whether request number of resources is greater than needed
	if(ifEnoughToAlloc(requestVector) == -1)
	{
		printf("Nuk ka resources mjaftueshem per kete process.\n");
		return -1;
	}

	//pretend allocated
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		needMatrix[processID][i] -= requestVector[i];
		allocMatrix[processID][i] += requestVector[i];
		availResourceVector[i] -= requestVector[i];
	}
	printf("Kontrollojme nese jemi ne safe status...\n");

	//check if still in safe status
	if (ifInSafeMode() == 0)
	{
		printf("Safe. Allocated successfully.\nNow available resources vector is:\n");
		printAvailable();
		printf("Now allocated matrix is:\n");
		printAllocMatrix();
		printf("Now need matrix is:\n");
		printNeedMatrix();
		return 0;
	}
	else
	{
		printf("It is not safe. Rolling back.\n");
		for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
		{
			needMatrix[processID][i] += requestVector[i];
			allocMatrix[processID][i] -= requestVector[i];
			availResourceVector[i] += requestVector[i];
		}
		printf("Rolled back successfully.\n");
		return -1;
	}
}

int releaseResource(int processID,int releaseVector[])
{
	if(ifEnoughToRelease(processID,releaseVector) == -1)
	{
		printf("Processi nuk ka resurse mjaftueshem per ti leshuar.\n");
		return -1;
	}

	//enough to release
	for(i = 0; i < NUMBER_OF_RESOURSES; i++)
	{
		allocMatrix[processID][i] -= releaseVector[i];
		needMatrix[processID][i] += releaseVector[i];
		availResourceVector[i] += releaseVector[i];
	}
	printf("Release successfully.\nNow available resources vector is:\n");
	printAvailable();
	printf("Now allocated matrix is:\n");
	printAllocMatrix();
	printf("Now need matrix is:\n");
	printNeedMatrix();
	return 0;
}

int ifEnoughToRelease(int processID,int releaseVector[])
{
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		if (releaseVector[i] <= allocMatrix[processID][i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}
int ifGreaterThanNeed(int processID,int requestVector[])
{
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		if (requestVector[i] <= needMatrix[processID][i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

int ifEnoughToAlloc(int requestVector[])
{
	//first element of requestVector is processID
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		if (requestVector[i] <= availResourceVector[i])
		{
			continue;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

void printNeedMatrix()
{
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		printf("{ ");
		for (j = 0; j < NUMBER_OF_RESOURSES; ++j)
		{
			printf("%d, ", needMatrix[i][j]);
		}
		printf("}\n");
	}
	return;
}

void printAllocMatrix()
{
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		printf("{ ");
		for (j = 0; j < NUMBER_OF_RESOURSES; ++j)
		{
			printf("%d, ", allocMatrix[i][j]);
		}
		printf("}\n");
	}
	return;
}

void printAvailable()
{
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		printf("%d, ",availResourceVector[i]);
	}
	printf("\n");
	return;
}

void printReqOrRelVector(int vec[])
{
	for (i = 0; i < NUMBER_OF_RESOURSES; ++i)
	{
		printf("%d, ",vec[i]);
	}
	printf("\n");
	return;
}
int ifInSafeMode()
{
	int ifFinish[NUMBER_OF_CUSTOMERS] = {0};//there is no bool type in old C
	int work[NUMBER_OF_RESOURSES];//temporary available resources vector
	for(i = 0; i < NUMBER_OF_RESOURSES; i++)
	{
		work[i] = availResourceVector[i];
	}
	int k;
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			for(j = 0; j < NUMBER_OF_RESOURSES; j++)
			{
				if(needMatrix[i][j] <= work[j])
				{
					if(j == NUMBER_OF_RESOURSES - 1)//means we checked whole vector, so this process can execute
					{
						ifFinish[i] = 1;
						for (k = 0; k < NUMBER_OF_RESOURSES; ++k)
						{
							work[k] += allocMatrix[i][k];
							//execute and release resources
						}
						//if we break here, it will not check all process, so we should reset i to let it check from beginning
						//If we cannot find any runnable process from beginning to the end in i loop, we can determine that
						//there is no any runnable process, but we cannot know if we do not reset i.
						i = -1;//at the end of this loop, i++, so -1++ = 0
						break;//in loop j, break to loop i and check next runnable process
					}
					else//not finished checking all resource, but this kind resources is enough
					{
						continue;
					}
				}
				else//resources not enough, break to loop i for next process
				{
					//because there is no change happened, so we do not need to reset i in this condition.
					break;
				}
			}
		}
		else
		{
			continue;
		}
	}
	//there are two condition if we finish loop i
	//1. there is no process can run in this condition.
	//2. all processes are runned, which means it is in safe status.
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (ifFinish[i] == 0)
		{
			//not all processes are runned, so it is condition 1.
			return -1;
		}
		else
		{
			continue;
		}
	}
	//finished loop, so it is condition 2
	return 0;
}