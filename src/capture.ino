
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
void recordPulse() {
// ------------------------
	uint16_t first = 0;
	uint16_t last = 0;
	uint32_t sum = 0;
	uint16_t n = 0;
	uint32_t t0 = micros();

	while(digitalRead(REC_TRIGGER_IN) == HIGH) {
		if(n < 512) {
			uint16_t sample = (uint16_t)(ADC1->regs->DR & ADC_DR_DATA);
			if(n == 0) first = sample;
			last = sample;
			sum += sample;
			n++;
		}
	}
	uint32_t pulseUs = micros() - t0;

	if(n == 0) return;

	uint16_t mean = sum / n;
	int32_t slopeX10 = (pulseUs > 0) ? ((int32_t)last - first) * 10 / pulseUs : 0;
	if(slopeX10 > 32767) slopeX10 = 32767;
	if(slopeX10 < -32768) slopeX10 = -32768;
	int16_t slope = (int16_t)slopeX10;

	recBuf[recHead].mean = mean;
	recBuf[recHead].rise = slope;
	recHead = (recHead + 1) % REC_BUF_SIZE;
	if(recCount < REC_BUF_SIZE) recCount++;

	float meanMv = ((int32_t)mean - zeroVoltageA1) * adcMultiplier[rangePos] * 1000.0f;
	float slopeMvPerUsX10 = slope * adcMultiplier[rangePos] * 1000.0f;
	int32_t meanMvInt = (int32_t)meanMv;
	int32_t slopeMvPerUsX10Int = (int32_t)slopeMvPerUsX10;
	if(meanMvInt > 32767) meanMvInt = 32767;
	if(meanMvInt < -32768) meanMvInt = -32768;
	if(slopeMvPerUsX10Int > 32767) slopeMvPerUsX10Int = 32767;
	if(slopeMvPerUsX10Int < -32768) slopeMvPerUsX10Int = -32768;

	uint16_t meanWire = (uint16_t)(int16_t)meanMvInt;
	uint16_t slopeWire = (uint16_t)(int16_t)slopeMvPerUsX10Int;
	// Sync bytes followed by signed little-endian mean mV and slope x10.
	uint8_t packet[6] = {
		0xA5, 0x5A,
		(uint8_t)(meanWire & 0xFF), (uint8_t)(meanWire >> 8),
		(uint8_t)(slopeWire & 0xFF), (uint8_t)(slopeWire >> 8)
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
	USB_PRINTLN("mean_mV,slope_mV_per_us");

	uint16_t start = (recCount < REC_BUF_SIZE) ? 0 : recHead;
	for(uint16_t i = 0; i < recCount; i++) {
		uint16_t idx = (start + i) % REC_BUF_SIZE;
		float meanMv = ((int32_t)recBuf[idx].mean - zeroVoltageA1) * adcMultiplier[rangePos] * 1000.0f;
		float slopeMvPerUs = recBuf[idx].rise * adcMultiplier[rangePos] * 100.0f;
		USB_PRINT(meanMv, 2);
		USB_PRINT(",");
		USB_PRINTLN(slopeMvPerUs, 2);
	}
	USB_PRINTLN("# END");
}
