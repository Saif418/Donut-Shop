#define WIN32_LEAN_AND_MEAN 

#pragma warning(disable : 4996)
#include <time.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <WinSock2.h>

#if !defined(_Wp64)
#define DWORD_PTR DWORD
#define LONG_PTR LONG
#define INT_PTR INT
#endif

typedef struct _Bakers
{
	HANDLE bakerHandle;
	int donutMade;
	int numOfDonuts[4];  // how many of each donut did the baker bake.
	int numOfDonutsToBake;
	int time;
	
}bakers, * pbakers;

typedef struct _DonutBin
{
	HANDLE DonutHandle;
	HANDLE donutInBinSem; 
	HANDLE emptyDonutBinSem;
	HANDLE lockBinMutex; // mutex
	
	int donutsInBin; 
	int donutsSold;
} donutBin, *pdonutBin;

typedef struct _DonutLine
{
	int worker[6];
	int front, back, count;
	HANDLE lockLineMutex;


}donutLine, *pDonutLine;

typedef struct _LineManager
{
	HANDLE donutAvailableEvent;
	HANDLE workDoneEvent;
	BOOL alive;
	HANDLE lineManager;

}lineManager, * pLineManager;

typedef struct _Workers
{
	HANDLE workerGoEvent;
	HANDLE workerHandle;
	int donutsSold[4];
	int totalDonutSold;
}worker, *pworker;

donutLine Dline;
worker workerArray[6];
bakers bakerArray[2];
donutBin donutBinArray[4];
donutLine donutLineArray[4];
lineManager lineManagerArray[4];
donutBin donuts;
bakers baker;
HANDLE bakerAlive; 
HANDLE protectLineManagerCount;
int bakerAliveCount = 2;
int lineManagerAliveCount = 4;

void WINAPI BakerFunction(int who);
void WINAPI LineManagerFunction(int who);
void WINAPI WorkerFunction(int who);

int _tmain(int argc, LPTSTR* argv[])
{
	int numOfDonutsToBake1 = 0;
	int numOfDonutsToBake2 = 0;
	int timeToBakeDonut = 0;
	int donutsInEachBin = 0;
	

	// inputs
	printf("Enter how many donuts baker 1 will bake today: ");
	scanf("%d", &numOfDonutsToBake1);
	
	printf("Enter how many donuts baker 2 will bake today: ");
	scanf("%d", &numOfDonutsToBake2);

	printf("Enter how much time it takes to bake 1 donut: ");
	scanf("%d", &timeToBakeDonut);
	
	printf("Enter how many donuts fit in a bin: ");
	scanf("%d", &donutsInEachBin);
	
	protectLineManagerCount = CreateMutex(NULL, FALSE, NULL);
	// initialize how many donuts each baker will make
	bakerArray[0].numOfDonutsToBake = numOfDonutsToBake1;
	bakerArray[1].numOfDonutsToBake = numOfDonutsToBake2;
	bakerAlive = CreateMutex(NULL, FALSE, NULL);
	// start threads
	for (int i = 0; i < 2; i++)
	{
		bakerArray[i].time = timeToBakeDonut;
		bakerArray[i].bakerHandle = (HANDLE)_beginthreadex(NULL, 0, BakerFunction, i, NULL, NULL);
		
	}
	for (int i = 0; i < 4; i++)
	{
	
		donutBinArray[i].donutInBinSem = CreateSemaphore(NULL, 0, donutsInEachBin, NULL);
		donutBinArray[i].emptyDonutBinSem = CreateSemaphore(NULL, donutsInEachBin, donutsInEachBin, NULL);
		donutBinArray[i].lockBinMutex = CreateMutex(NULL, FALSE, NULL);
		lineManagerArray[i].donutAvailableEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		lineManagerArray[i].alive = TRUE;
		donutLineArray[i].lockLineMutex = CreateMutex(NULL, FALSE, NULL);
		lineManagerArray[i].lineManager = (HANDLE)_beginthreadex(NULL, 0, LineManagerFunction, i, NULL, NULL);

	}
	for (int i = 0; i < 6; i++)
	{
		workerArray[i].workerGoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		workerArray[i].workerHandle = (HANDLE)_beginthreadex(NULL, 0, WorkerFunction, i, NULL, NULL);
	}
	for (int i = 0; i < 2; i++)
	{
		WaitForSingleObject(bakerArray[i].bakerHandle, INFINITE);
	}
	
	for (int i = 0; i < 4; i++)
	{
		WaitForSingleObject(lineManagerArray[i].lineManager, INFINITE);
	}


	for (int i = 0; i < 6; i++)
	{

		WaitForSingleObject(workerArray[i].workerHandle, INFINITE);
	}
	
	printf("Baker #1\n");
	printf("Made %d donuts\n", bakerArray[0].donutMade);
	for (int i = 0; i < 4; i++)
	{
		printf("%d type #%d\n", bakerArray[0].numOfDonuts[i],i);
		
	}
	
	printf("\nBaker #2\n");
	printf("Made %d donuts\n", bakerArray[1].donutMade);
	for (int i = 0; i < 4; i++)
	{
		printf("%d type #%d\n", bakerArray[1].numOfDonuts[i],i);
		
	}
	
	printf("\n\n");
	for (int i = 0; i < 4; i++)
	{
		printf("Bin #%d  ",i);
		printf("%d donuts\n", donutBinArray[i].donutsSold);
	}
		
	for (int i = 0; i < 6; i++)
	{
		printf("\n\nWorker #%d  ", i);
		printf("Sold %d", workerArray[i].totalDonutSold);
		
		
		for (int x = 0; x < 4; x++)
		{
			printf("\nDonut #%d: %d", x, workerArray[i].donutsSold[x]);
		
		}
			
	}

	return 0;
}

// Baker goes home
void WINAPI BakerFunction(int whichBaker) // baking
{
	srand(&whichBaker);
	int randomNum = 0;
	int prevCount;

	while(bakerArray[whichBaker].numOfDonutsToBake > 0)
	{
		
		
			randomNum = rand() % 4;

			if (!WaitForSingleObject(donutBinArray[randomNum].emptyDonutBinSem, 10))  
			{
				bakerArray[whichBaker].numOfDonutsToBake--;
				Sleep(bakerArray[whichBaker].time);
				bakerArray[whichBaker].donutMade++;
				
				WaitForSingleObject(donutBinArray[randomNum].lockBinMutex, INFINITE);
					donutBinArray[randomNum].donutsInBin++;
					
				ReleaseMutex(donutBinArray[randomNum].lockBinMutex);
				ReleaseSemaphore(donutBinArray[randomNum].donutInBinSem, 1, &prevCount);
				bakerArray[whichBaker].numOfDonuts[randomNum]++;  // for report
				
				SetEvent(lineManagerArray[randomNum].donutAvailableEvent); // signal line managers, donut available (event)
			}
		
		
	}
	WaitForSingleObject(bakerAlive, INFINITE);
	bakerAliveCount--;
		if (bakerAliveCount == 0)
		{
			//signal all line managers donut avaible (event)
			for (int i = 0; i < 4; i++)
			{
				SetEvent(lineManagerArray[i].donutAvailableEvent);
			}
		}
	ReleaseMutex(bakerAlive);

}


void WINAPI WorkerFunction(int whichWorker)
{
	srand(&whichWorker);
	int randomNum = 0;
	int prevCount;
	while (lineManagerAliveCount > 0)
	{
		randomNum = rand() % 4;
		if(lineManagerArray[randomNum].alive)
		{
			//lock down line before changing it
			WaitForSingleObject(donutLineArray[randomNum].lockLineMutex, INFINITE);
				donutLineArray[randomNum].worker[donutLineArray[randomNum].back] = whichWorker;
				donutLineArray[randomNum].back = (donutLineArray[randomNum].back +1) % 6;  // circular array
				donutLineArray[randomNum].count++;
			ReleaseMutex(donutLineArray[randomNum].lockLineMutex);
			WaitForSingleObject(workerArray[whichWorker].workerGoEvent, INFINITE);

			if (!WaitForSingleObject(donutBinArray[randomNum].donutInBinSem, 10))
			{
				WaitForSingleObject(donutBinArray[randomNum].lockBinMutex, INFINITE);
				donutBinArray[randomNum].donutsInBin = donutBinArray[randomNum].donutsInBin--;
				donutBinArray[randomNum].donutsSold = donutBinArray[randomNum].donutsSold++;

				ReleaseMutex(donutBinArray[randomNum].lockBinMutex);
				workerArray[whichWorker].totalDonutSold++;
				workerArray[whichWorker].donutsSold[randomNum]++;
				ReleaseSemaphore(donutBinArray[randomNum].emptyDonutBinSem, 1, &prevCount);
				SetEvent(lineManagerArray[randomNum].workDoneEvent); // not sure what he means by signal line manager that work is done
			}
			else {
				SetEvent(lineManagerArray[randomNum].workDoneEvent);
			}

		}

	}


}


void WINAPI LineManagerFunction(int who)
{
	int person; //front of the line who is being dequeued 
	while (bakerAliveCount > 0 || donutBinArray[who].donutsInBin > 0)
	{
		WaitForSingleObject(lineManagerArray[who].donutAvailableEvent, INFINITE);
		if (donutLineArray[who].count > 0)
		{
			WaitForSingleObject(donutLineArray[who].lockLineMutex, INFINITE);
				person = donutLineArray[who].worker[donutLineArray[who].front];
				donutLineArray[who].front = (donutLineArray[who].front+1)%6;
				donutLineArray[who].count--;
			ReleaseMutex(donutLineArray[who].lockLineMutex);
			SetEvent(workerArray[person].workerGoEvent); 
			WaitForSingleObject(lineManagerArray[who].workDoneEvent, INFINITE);
			if (donutBinArray[who].donutsInBin > 0)
			{
				SetEvent(lineManagerArray[who].donutAvailableEvent);
			}

		}
		else {
			SetEvent(lineManagerArray[who].donutAvailableEvent);
		}
	}

	WaitForSingleObject(protectLineManagerCount, INFINITE);
		lineManagerAliveCount--;
		lineManagerArray[who].alive = FALSE;
	ReleaseMutex(protectLineManagerCount);

	WaitForSingleObject(donutLineArray[who].lockLineMutex, INFINITE);
	while (donutLineArray[who].count > 0)
	{
		person = donutLineArray[who].worker[donutLineArray[who].front];
		donutLineArray[who].front = (donutLineArray[who].front + 1) % 6;
		donutLineArray[who].count--;
		

		SetEvent(workerArray[person].workerGoEvent);
	}
	ReleaseMutex(donutLineArray[who].lockLineMutex);
}