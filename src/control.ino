
enum { TRIGGER_AUTO, TRIGGER_NORM, TRIGGER_SINGLE };
uint8_t triggerType;


// ------------------------
void setTriggerType(uint8_t tType)	{
// ------------------------
	triggerType = tType;
	keepSampling = false;
}


// ------------------------
void processSerialCommands() {
// ------------------------
	if(!Serial.available()) return;
	char cmd = Serial.read();
	switch(cmd) {
		case 'R': case 'r':
			recMode = !recMode;
			hold = false;
			keepSampling = false;
			if(recMode) {
				recHead = 0;
				recCount = 0;
			}
			USB_PRINTLN(recMode ? "REC MODE ON" : "REC MODE OFF");
			break;
		case 'D': case 'd':
			USB_PRINTLN("DUMPING...");
			dumpRecCSV();
			USB_PRINTLN("DONE");
			break;
		case 'C': case 'c':
			recHead = 0;
			recCount = 0;
			USB_PRINTLN("BUFFER CLEARED");
			break;
		case 'H': case 'h':
			hold = !hold;
			repaintLabels();
			USB_PRINTLN(hold ? "HOLD ON" : "HOLD OFF");
			break;
	}
}


// ------------------------
void controlLoop()	{
// ------------------------
	processSerialCommands();

	if(recMode) {
		if(digitalRead(REC_TRIGGER_IN) == HIGH)
			recordPulse();
		return;
	}

	readInpSwitches();

	if(triggerType == TRIGGER_AUTO)	{
		captureDisplayCycle(true);
	}
	else if(triggerType == TRIGGER_NORM)	{
		captureDisplayCycle(false);
	}
	else	{
		clearWaves();
		indicateCapturing();
		sampleWaves(false);
		indicateCapturingDone();
		hold = true;
		repaintLabels();
		drawWaves();
		blinkLED();
		dumpSamples();
		while(hold);
		drawLabels();
	}
}


// ------------------------
void captureDisplayCycle(boolean wTimeOut)	{
// ------------------------
	indicateCapturing();
	sampleWaves(wTimeOut);
	indicateCapturingDone();
	drawWaves();
	if(triggered)
		blinkLED();
	if(hold)	{
		drawLabels();
		dumpSamples();
	}
	while(hold);
}
