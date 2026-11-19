
// sampling delay table in quarter-microseconds
const int16_t samplingDelay[] =   {-1,  0, 14, 38, 86, 229, 468, 948, 2385, 4776, 9570, 23940};
const uint16_t timeoutDelayMs[] = {50, 50, 50, 100, 100, 100, 150, 250, 500, 1000, 2000, 4500};

int16_t sDly, tDly;
boolean minSamplesAcquired;
boolean triggerRising;
long prevTime = 0;

uint16_t *sIndexPtr = &sIndex;
volatile boolean *keepSamplingPtr = &keepSampling;
volatile boolean *triggeredPtr = &triggered;



// ------------------------
void setSamplingRate(uint8_t timeBase)	{
// ------------------------
	sDly = samplingDelay[timeBase];
	tDly = timeoutDelayMs[timeBase];
	keepSampling = false;
	Timer2.pause();
}



// ------------------------
void setTriggerRising(boolean rising)	{
// ------------------------
	keepSampling = false;
	triggerRising = rising;
	detachInterrupt(TRIGGER_IN);
	if(rising)
		attachInterrupt(TRIGGER_IN, triggerISR, RISING);
	else
		attachInterrupt(TRIGGER_IN, triggerISR, FALLING);
}



// ------------------------
void sampleWaves(boolean wTimeout)	{
// ------------------------
	if(wTimeout)
		startScanTimeout(tDly);
	startSampling(sDly);
	Timer2.pause();
}



// ------------------------
void startScanTimeout(int16_t mSec)	{
// ------------------------
	Timer2.setCount(2);
	Timer2.setPeriod(mSec * 1000);
	Timer2.resume();
}



// ------------------------
void triggerISR(void) {
// ------------------------
	if(!triggered)	{
		if(!minSamplesAcquired && (sIndex < NUM_SAMPLES/2))
			return;
		tIndex = sIndex;
		triggered = true;
	}
}



// ------------------------
void scanTimeoutISR(void) {
// ------------------------
	keepSampling = false;
	Timer2.pause();
}



// ------------------------
void startSampling(int16_t lDelay)	{
// ------------------------
	keepSampling = true;
	minSamplesAcquired = false;
	uint16_t lCtr = 0;

	samplingTime = 0;
	triggered = false;
	sIndex = 0;

	prevTime = micros();

	if(lDelay < 0)	{

		asm volatile(
			"\tldrh r9, [%[sIndex]]\t\t\t\n\t"

			"top_1:\t\t\t\t\t\t\t\n\t"
			"\tldrb r0, [%[keepSampling]]\t\t\n\t"
			"\tcbz r0, finished_1\t\t\t\t\n\t"

			"\tldr r1, =0x40012400\t\t\t\t\n\t"
			"\tldr r0, [r1, #0x4C]\t\t\t\t\n\t"
			"\tstrh r0, [%[ch1], r9, lsl #1]\t\n\t"

			"\tadds r9, #1\t\t\t\t\t\t\n\t"
			"\tcmp r9, %[nSamp]\t\t\t\t\n\t"
			"\tbne notOverflowed_1\t\t\t\t\n\t"
			"\tmov r9, #0\t\t\t\t\t\t\n\t"

			"\tstmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr]}\t\n\t"
			"\tbl %[snapMicros]\t\t\t\t\n\t"
			"\tldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr]}\t\n\t"

			"notOverflowed_1:\t\t\t\t\t\n\t"
			"\tstrh r9, [%[sIndex]]\t\t\t\n\t"

			"\tldrb r0, [%[triggered]]\t\t\t\n\t"
			"\tcbz r0, notTriggered_1\t\t\t\n\t"

			"\tadds %[lCtr], #1\t\t\t\t\n\t"
			"\tcmp %[lCtr], %[halfSamples]\t\t\n\t"
			"\tbeq finished_1\t\t\t\t\t\n\t"

			"notTriggered_1:\t\t\t\t\t\n\t"
			"\tb top_1\t\t\t\t\t\t\t\n\t"
			"finished_1:\t\t\t\t\t\t\n\t"

			:
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr),
				[ch1] "r" (ch1Capture), [lCtr] "r" (lCtr),
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);

	}
	else if(lDelay == 0)	{

		asm volatile(
			"\tldrh r9, [%[sIndex]]\t\t\t\n\t"

			"top_2:\t\t\t\t\t\t\t\n\t"
			"\tldrb r0, [%[keepSampling]]\t\t\n\t"
			"\tcbz r0, finished_2\t\t\t\t\n\t"

			"\tldr r1, =0x40012400\t\t\t\t\n\t"

			"waitADC1_2:\t\t\t\t\t\t\n\t"
			"\tldr r0, [r1, #0]\t\t\t\t\n\t"
			"\tlsls r0, r0, #30\t\t\t\t\n\t"
			"\tbpl\twaitADC1_2\t\t\t\t\t\n\t"

			"\tldr r0, [r1, #0x4C]\t\t\t\t\n\t"
			"\tstrh r0, [%[ch1], r9, lsl #1]\t\n\t"

			"\tadds r9, #1\t\t\t\t\t\t\n\t"
			"\tcmp r9, %[nSamp]\t\t\t\t\n\t"
			"\tbne notOverflowed_2\t\t\t\t\n\t"
			"\tmov r9, #0\t\t\t\t\t\t\n\t"

			"\tstmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr]}\t\n\t"
			"\tbl %[snapMicros]\t\t\t\t\n\t"
			"\tldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr]}\t\n\t"

			"notOverflowed_2:\t\t\t\t\t\n\t"
			"\tstrh r9, [%[sIndex]]\t\t\t\n\t"

			"\tldrb r0, [%[triggered]]\t\t\t\n\t"
			"\tcbz r0, notTriggered_2\t\t\t\n\t"

			"\tadds %[lCtr], #1\t\t\t\t\n\t"
			"\tcmp %[lCtr], %[halfSamples]\t\t\n\t"
			"\tbeq finished_2\t\t\t\t\t\n\t"

			"notTriggered_2:\t\t\t\t\t\n\t"
			"\tb top_2\t\t\t\t\t\t\t\n\t"
			"finished_2:\t\t\t\t\t\t\n\t"

			:
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr),
				[ch1] "r" (ch1Capture), [lCtr] "r" (lCtr),
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);

	}
	else	{

		asm volatile(
			"\tldrh r9, [%[sIndex]]\t\t\t\n\t"

			"top_3:\t\t\t\t\t\t\t\n\t"
			"\tldrb r0, [%[keepSampling]]\t\t\n\t"
			"\tcbz r0, finished_3\t\t\t\t\n\t"

			"\tldr r1, =0x40012400\t\t\t\t\n\t"

			"waitADC1_3:\t\t\t\t\t\t\n\t"
			"\tldr r0, [r1, #0]\t\t\t\t\n\t"
			"\tlsls r0, r0, #30\t\t\t\t\n\t"
			"\tbpl\twaitADC1_3\t\t\t\t\t\n\t"

			"\tldr r0, [r1, #0x4C]\t\t\t\t\n\t"
			"\tstrh r0, [%[ch1], r9, lsl #1]\t\n\t"

			"\tadds r9, #1\t\t\t\t\t\t\n\t"
			"\tcmp r9, %[nSamp]\t\t\t\t\n\t"
			"\tbne notOverflowed_3\t\t\t\t\n\t"
			"\tmov r9, #0\t\t\t\t\t\t\n\t"

			"\tstmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr], %[tDelay]}\t\n\t"
			"\tbl %[snapMicros]\t\t\t\t\n\t"
			"\tldmfd sp!,{r9, %[keepSampling], %[sIndex], %[triggered], %[ch1], %[lCtr], %[tDelay]}\t\n\t"

			"notOverflowed_3:\t\t\t\t\t\n\t"
			"\tstrh r9, [%[sIndex]]\t\t\t\n\t"

			"\tldrb r0, [%[triggered]]\t\t\t\n\t"
			"\tcbz r0, notTriggered_3\t\t\t\n\t"

			"\tadds %[lCtr], #1\t\t\t\t\n\t"
			"\tcmp %[lCtr], %[halfSamples]\t\t\n\t"
			"\tbeq finished_3\t\t\t\t\t\n\t"

			"notTriggered_3:\t\t\t\t\t\n\t"
			"\tmov r0, %[tDelay]      \t\t\t\n\t"
			"1:           \t\t\t\t\t\t\n\t"
			"\tsubs r0, #1            \t\t\t\n\t"
			"\tbhi 1b                 \t\t\t\n\t"

			"\tb top_3\t\t\t\t\t\t\t\n\t"
			"finished_3:\t\t\t\t\t\t\n\t"

			:
			: [keepSampling] "r" (keepSamplingPtr), [sIndex] "r" (sIndexPtr), [triggered] "r" (triggeredPtr),
				[ch1] "r" (ch1Capture), [lCtr] "r" (lCtr),
				[nSamp] "I" (NUM_SAMPLES), [halfSamples] "I" (NUM_SAMPLES/2), [tDelay] "r" (lDelay),
				[snapMicros] "i" (snapMicros)
			: "r0", "r1", "r9", "memory", "cc"
		);

	}

}



// ------------------------
inline void snapMicros()	{
// ------------------------
	samplingTime = micros() - prevTime;
	prevTime = micros();
	minSamplesAcquired = true;
}



// ------------------------
void dumpSamples()	{
// ------------------------
	float timePerSample = ((float)samplingTime) / NUM_SAMPLES;
	DBG_PRINT("Net sampling time (us): "); DBG_PRINTLN(samplingTime);
	DBG_PRINT("Per Sample (us): "); DBG_PRINTLN(timePerSample);
	DBG_PRINT("Timebase: "); DBG_PRINT(getTimebaseLabel()); DBG_PRINTLN("/div");
	DBG_PRINT("CH1 Coupling: "); DBG_PRINT(cplNames[couplingPos]); DBG_PRINT(", Range: "); DBG_PRINT(rngNames[rangePos]); DBG_PRINTLN("/div");
	DBG_PRINT("Triggered: "); DBG_PRINTLN(triggered ? "YES" : "NO");
	DBG_PRINTLN("");
}


// ------------------------
void dumpSamplesCSV()	{
// ------------------------
	USB_PRINTLN("index,time_us,ch1_adc");
	double timePerSample = ((double)samplingTime) / NUM_SAMPLES;
	for(uint16_t i = 0; i < NUM_SAMPLES; i++) {
		USB_PRINT(i);
		USB_PRINT(",");
		USB_PRINT((double)i * timePerSample, 3);
		USB_PRINT(",");
		USB_PRINTLN(ch1Capture[i]);
	}
}



// ------------------------
float getMilliVoltsPerAdcCount() {
// ------------------------
	float scale = adcMultiplier[rangePos];
	if(rangePos < RNG_50mV)
		scale *= 1000.0f;
	return scale;
}



// ------------------------
void recordPulse() {
// ------------------------
	uint16_t first = 0;
	uint16_t last = 0;
	uint32_t sum = 0;
	uint32_t firstFourSum = 0;
	uint32_t lastFourSum = 0;
	uint16_t lastFour[4] = {0, 0, 0, 0};
	uint16_t n = 0;
	uint8_t firstFourCount = 0;
	uint8_t lastFourCount = 0;
	uint8_t lastFourIndex = 0;

	while(digitalRead(REC_TRIGGER_IN) == HIGH) {
		uint16_t sample = (uint16_t)(ADC1->regs->DR & ADC_DR_DATA);
		if(n == 0) first = sample;
		last = sample;
		sum += sample;
		n++;

		if(firstFourCount < 4) {
			firstFourSum += sample;
			firstFourCount++;
		}
		if(lastFourCount == 4)
			lastFourSum -= lastFour[lastFourIndex];
		else
			lastFourCount++;
		lastFour[lastFourIndex] = sample;
		lastFourSum += sample;
		lastFourIndex = (lastFourIndex + 1) % 4;
	}

	if(n == 0) return;

	uint16_t meanAdc = sum / n;
	float milliVoltsPerAdcCount = getMilliVoltsPerAdcCount();
	int32_t meanMv = (int32_t)(((int32_t)meanAdc - zeroVoltageA1) * milliVoltsPerAdcCount);
	if(meanMv < 0) meanMv = 0;
	if(meanMv > 4000) meanMv = 4000;

	float deltaAdc = (n >= 9)
		? ((int32_t)lastFourSum - (int32_t)firstFourSum) / 4.0f
		: (int32_t)last - first;
	int32_t deltaMv = (int32_t)(deltaAdc * milliVoltsPerAdcCount);
	if(deltaMv > 4000) deltaMv = 4000;
	if(deltaMv < -4000) deltaMv = -4000;

	recBuf[recHead].meanMv = (uint16_t)meanMv;
	recBuf[recHead].deltaMv = (int16_t)deltaMv;
	recHead = (recHead + 1) % REC_BUF_SIZE;
	if(recCount < REC_BUF_SIZE) recCount++;

	uint16_t meanWire = (uint16_t)meanMv;
	uint16_t deltaWire = (uint16_t)(int16_t)deltaMv;
	// Sync bytes followed by unsigned mean mV and signed delta mV.
	uint8_t packet[6] = {
		0xA5, 0x5A,
		(uint8_t)(meanWire & 0xFF), (uint8_t)(meanWire >> 8),
		(uint8_t)(deltaWire & 0xFF), (uint8_t)(deltaWire >> 8)
	};
	Serial.write(packet, sizeof(packet));
}



// ------------------------
void dumpRecCSV() {
// ------------------------
	USB_PRINTLN("# DLO-138 REC dump");
	USB_PRINT("# Timebase: "); USB_PRINTLN(getTimebaseLabel());
	USB_PRINT("# Range: "); USB_PRINTLN(rngNames[rangePos]);
	USB_PRINT("# Coupling: "); USB_PRINTLN(cplNames[couplingPos]);
	USB_PRINT("# Records: "); USB_PRINTLN(recCount);
	USB_PRINTLN("mean_mV,delta_mV");

	uint16_t start = (recCount < REC_BUF_SIZE) ? 0 : recHead;
	for(uint16_t i = 0; i < recCount; i++) {
		uint16_t idx = (start + i) % REC_BUF_SIZE;
		USB_PRINT(recBuf[idx].meanMv);
		USB_PRINT(",");
		USB_PRINTLN(recBuf[idx].deltaMv);
	}
	USB_PRINTLN("# END");
}
