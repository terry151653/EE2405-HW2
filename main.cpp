#include "mbed.h"
#include "mbed_events.h"
#include "uLCD_4DGL.h"
#include <chrono>

#define MAXSAMPLES 65536
#define SAMPLINGPERIOD 125us
#define WAVEPERIOD 2500us
#define SAMPLEEACHWAVE (WAVEPERIOD / SAMPLINGPERIOD)
#define FIRSTSTEP (SAMPLEEACHWAVE / 10)
#define SECONDSTEP (SAMPLEEACHWAVE * 5 / 10)

using namespace std::chrono;

uLCD_4DGL uLCD(D1, D0, D2);

Timer DebounceA;
Timer DebounceB;
Timer DurationOfWave;
InterruptIn BtnA(D9);
InterruptIn BtnB(D8);

AnalogOut OutSig(D7);
AnalogIn InSig(D6);

EventQueue BtnQueue(8 * EVENTS_EVENT_SIZE);
EventQueue printfQueue(64 * EVENTS_EVENT_SIZE);
EventQueue WAVEQueue(32 * EVENTS_EVENT_SIZE);

Ticker genWave;
Ticker storeWave;

enum STATUS {WAVEGEN = 0, DATATRANS, DATAEND};

bool GenWave = 0;
float ADCData[MAXSAMPLES] = {0};
int NumOfSamples = 0;
int SampleCnt = 0;
int WaveFormCnt = 0;
int DuraitonThisTime = 0;

void gensample()
{ //Ticker ISR; Call an eventqueue to schedule a DAC sample.
    if (GenWave)
    {
        if (++WaveFormCnt >= SAMPLEEACHWAVE)//0 ~ n-1
            WaveFormCnt = 0;
        
        if (WaveFormCnt < FIRSTSTEP) // < 0.1
            OutSig = (float)WaveFormCnt / FIRSTSTEP;
        else if (SampleCnt > SECONDSTEP) // > 0.5
            OutSig = 2 - (float)WaveFormCnt / SECONDSTEP;
        else
            OutSig = 1.0;
    }
}
void wavegen()
{
	while (1)
	{
   		if (GenWave)
		{
      		genWave.attach(WAVEQueue.event(gensample), SAMPLINGPERIOD);
   		}
   		else
		{
    		genWave.detach();
   		}
		ThisThread::sleep_for(50ms);
	}
}
void storesample()
{ //Ticker ISR; Call an eventqueue to schedule a DAC sample.
    if (GenWave)
    {
        if (SampleCnt < MAXSAMPLES)
            ADCData[SampleCnt++] = InSig;
        else
        {
            DuraitonThisTime = duration_cast<microseconds>(DurationOfWave.elapsed_time()).count();
            DurationOfWave.stop();
        }
        NumOfSamples = SampleCnt;
    }
    else
        SampleCnt = 0;
}
void wavestore()
{
	while (1)
	{
   		if (GenWave)
		{
      		storeWave.attach(WAVEQueue.event(storesample), SAMPLINGPERIOD);
            if (!duration_cast<microseconds>(DurationOfWave.elapsed_time()).count())
                DurationOfWave.start();
   		}
   		else
		{
    		storeWave.detach();
            if (duration_cast<milliseconds>(DurationOfWave.elapsed_time()).count())
                DuraitonThisTime = duration_cast<microseconds>(DurationOfWave.elapsed_time()).count();
            DurationOfWave.stop();
            DurationOfWave.reset();
   		}
		ThisThread::sleep_for(50ms);
	}
}
void printuLCD(int status)
{
	switch (status)
	{
	case WAVEGEN:
		uLCD.printf("\rWave generating.");
		break;
	case DATATRANS:
		uLCD.printf("\rData transmitting.");
		break;
	case DATAEND:
		uLCD.printf("\rData transmitted.");
		break;
	}
}
void printADCData(int pos)
{
	if (pos == -1)
	{
		printf("End transmission.\n");
        printfQueue.call(&printuLCD, DATAEND);
	}
	else
	{
		if (!pos)
		{
			printf("Start transmission.\n");
			printf("%d\n", NumOfSamples);
            printf("%d\n", DuraitonThisTime);
		}
		printf("%f\n", ADCData[pos]);
	}
}

void btnfallirq(bool status)
{
	if (!status && duration_cast<milliseconds>(DebounceA.elapsed_time()).count() > 1000)
    {
        //restart timer when the toggle is performed
        printuLCD(WAVEGEN);
        DebounceA.reset();
        GenWave = true;
    }
	else if (status && duration_cast<milliseconds>(DebounceB.elapsed_time()).count() > 1000)
    {
        GenWave = false;
        DebounceB.reset(); //restart timer when the toggle is performed
		printfQueue.call(&printuLCD, DATATRANS);
    }
}

void uLCDinit()
{
	uLCD.background_color(BLACK);
    uLCD.cls();
	uLCD.textbackground_color(BLACK);
	uLCD.color(WHITE);
	uLCD.printf("\nEE2405 HW2\n");
	uLCD.printf("Status:\n");
}
int main()
{
	int cnt = 0;
	//4 thread, high to low
	Thread BtnThread(osPriorityHigh);
	BtnThread.start(callback(&BtnQueue, &EventQueue::dispatch_forever));

    Thread WAVEThread(osPriorityHigh);
    WAVEThread.start(callback(&WAVEQueue, &EventQueue::dispatch_forever));

	Thread ADCThread(osPriorityNormal);
    ADCThread.start(wavegen);

	Thread DACThread(osPriorityNormal);
    DACThread.start(wavestore);

	Thread printfThread(osPriorityLow);
	printfThread.start(callback(&printfQueue, &EventQueue::dispatch_forever));

	uLCDinit();

	DebounceA.start();
	DebounceB.start();
	//btn interrupt, in high priority thread
	BtnA.fall(BtnQueue.event(&btnfallirq, WAVEGEN));
	BtnB.fall(BtnQueue.event(&btnfallirq, DATATRANS));
    
	printf("Initial over\n");

	while (1)
	{
		//reset counter
		if (GenWave)
			cnt = 0/*, printf("WaveGen%d\n", GenWave), printf("sample%d\n", sampleThisTime)*/, ThisThread::sleep_for(1s);
		//call print in low priority thread
		else
		{
			if (cnt < NumOfSamples)
			{
                //printf("WaveGen%d %d\n", GenWave, cnt);
                printfQueue.call(printADCData, cnt);
                if (++cnt == NumOfSamples)
					printfQueue.call(printADCData, -1);
				    
				ThisThread::sleep_for(15ms);
			}
			else
			 	printf("Waitting\n"), ThisThread::sleep_for(1s);
		}
	}
}