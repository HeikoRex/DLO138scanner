
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
void recordPulse() {
// ------------------------
	uint16_t buf[512];
	uint16_t n = 0;
	adc_reg_map *regs = ADC1->regs;
	long t0 = micros();

	while(digitalRead(TRIGGER_IN) == HIGH && n < 512) {
		buf[n++] = (uint16_t)(regs->DR & ADC_DR_DATA);
	}
	long pulseUs = micros() - t0;

	if(n < 4) return;

	uint32_t sum = 0;
	for(uint16_t i = 0; i < n; i++) sum += buf[i];
	uint16_t mean = sum / n;

	uint16_t vmin = buf[0], vmax = buf[0];
	for(uint16_t i = 1; i < n; i++) {
		if(buf[i] < vmin) vmin = buf[i];
		if(buf[i] > vmax) vmax = buf[i];
	}
	uint16_t v10 = vmin + (vmax - vmin) / 10;
	uint16_t v90 = vmin + (vmax - vmin) * 9 / 10;

	int16_t i10 = -1, i90 = -1;
	for(uint16_t i = 0; i < n; i++) {
		if(i10 < 0 && buf[i] >= v10) i10 = i;
		if(i90 < 0 && buf[i] >= v90) i90 = i;
	}

	int16_t rise = 0;
	if(i10 >= 0 && i90 > i10) {
		float usPerSample = (float)pulseUs / n;
		float riseUs = (i90 - i10) * usPerSample;
		rise = (int16_t)(((int32_t)(v90 - v10) * 100) / (int32_t)riseUs);
	}

	recBuf[recHead].mean = mean;
	recBuf[recHead].rise = rise;
	recHead = (recHead + 1) % REC_BUF_SIZE;
	if(recCount < REC_BUF_SIZE) recCount++;
}



// ------------------------
void dumpRecCSV() {
// ------------------------
	USB_PRINTLN("# DLO-138 REC dump");
	USB_PRINT("# Timebase: "); USB_PRINTLN(getTimebaseLabel());
	USB_PRINT("# Range: "); USB_PRINTLN(rngNames[rangePos]);
	USB_PRINT("# Coupling: "); USB_PRINTLN(cplNames[couplingPos]);
	USB_PRINT("# Records: "); USB_PRINTLN(recCount);
	USB_PRINTLN("index,mean_adc,rise_adc_per_us_x100,pointer");

	uint16_t start = (recCount < REC_BUF_SIZE) ? 0 : recHead;
	for(uint16_t i = 0; i < recCount; i++) {
		uint16_t idx = (start + i) % REC_BUF_SIZE;
		USB_PRINT(i);
		USB_PRINT(",");
		USB_PRINT(recBuf[idx].mean);
		USB_PRINT(",");
		USB_PRINT(recBuf[idx].rise);
		USB_PRINT(",");
		USB_PRINTLN((idx == (recHead == 0 ? REC_BUF_SIZE-1 : recHead-1)) ? "<--" : "");
	}
	USB_PRINTLN("# END");
}
