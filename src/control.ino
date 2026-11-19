
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

	String line = Serial.readStringUntil('\n');
	line.trim();
	line.toUpperCase();
	if(line.length() == 0) return;

	if(line == "HELP") {
		USB_PRINTLN("HELP");
		USB_PRINTLN("STATUS");
		USB_PRINTLN("SAMPLES");
		USB_PRINTLN("DUMP");
		USB_PRINTLN("CAPTURE");
		USB_PRINTLN("AUTO");
		USB_PRINTLN("NORMAL");
		USB_PRINTLN("SINGLE");
		USB_PRINTLN("HOLD ON|OFF");
		USB_PRINTLN("TIMEBASE 0..11");
		USB_PRINTLN("TRIGGER RISING|FALLING");
		USB_PRINTLN("R|C|H legacy commands");
		USB_PRINTLN("OK");
		return;
	}
	if(line == "STATUS") {
		USB_PRINT("STATUS,timebase="); USB_PRINT(currentTimeBase);
		USB_PRINT(",timebase_name="); USB_PRINT(getTimebaseLabel());
		USB_PRINT(",samples="); USB_PRINT(NUM_SAMPLES);
		USB_PRINT(",hold="); USB_PRINT(hold ? "ON" : "OFF");
		USB_PRINT(",rec="); USB_PRINT(recMode ? "ON" : "OFF");
		USB_PRINT(",trigger="); USB_PRINT(triggerRising ? "RISING" : "FALLING");
		USB_PRINT(",triggered="); USB_PRINTLN(triggered ? "YES" : "NO");
		USB_PRINTLN("OK");
		return;
	}
	if(line == "SAMPLES") {
		USB_PRINTLN(NUM_SAMPLES);
		USB_PRINTLN("OK");
		return;
	}
	if(line == "DUMP") {
		dumpRecCSV();
		USB_PRINTLN("OK");
		return;
	}
	if(line == "CAPTURE") {
		recMode = true;
		hold = false;
		keepSampling = false;
		recHead = 0;
		recCount = 0;
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}
	if(line == "AUTO") {
		setTriggerType(TRIGGER_AUTO);
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}
	if(line == "NORMAL") {
		setTriggerType(TRIGGER_NORM);
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}
	if(line == "SINGLE") {
		setTriggerType(TRIGGER_SINGLE);
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}
	if(line.startsWith("HOLD ")) {
		line.remove(0, 5);
		if(line == "ON") hold = true;
		else if(line == "OFF") hold = false;
		else {
			USB_PRINTLN("ERROR HOLD ON|OFF");
			return;
		}
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}
	if(line.startsWith("TIMEBASE ")) {
		String value = line.substring(9);
		int timeBase = value.toInt();
		if(value.length() == 0 || timeBase < T20US || timeBase > T50MS) {
			USB_PRINTLN("ERROR TIMEBASE 0..11");
			return;
		}
		setTimeBase((uint8_t)timeBase);
		USB_PRINTLN("OK");
		return;
	}
	if(line.startsWith("TRIGGER ")) {
		String edge = line.substring(8);
		if(edge == "RISING") setTriggerRising(true);
		else if(edge == "FALLING") setTriggerRising(false);
		else {
			USB_PRINTLN("ERROR TRIGGER RISING|FALLING");
			return;
		}
		repaintLabels();
		USB_PRINTLN("OK");
		return;
	}

	// Short compatibility commands
	char cmd = line.charAt(0);
	switch(cmd) {
		case 'R':
			recMode = !recMode;
			hold = false;
			keepSampling = false;
			if(recMode) {
				recHead = 0;
				recCount = 0;
			}
			USB_PRINTLN(recMode ? "REC MODE ON" : "REC MODE OFF");
			break;
		case 'D':
			dumpRecCSV();
			USB_PRINTLN("OK");
			break;
		case 'C':
			recHead = 0;
			recCount = 0;
			USB_PRINTLN("BUFFER CLEARED");
			break;
		case 'H':
			hold = !hold;
			repaintLabels();
			USB_PRINTLN(hold ? "HOLD ON" : "HOLD OFF");
			break;
		default:
			USB_PRINTLN("ERROR UNKNOWN COMMAND");
	}
}


// ------------------------
void controlLoop()	{
// ------------------------
	static boolean recModeWasActive = false;

	processSerialCommands();

	if(recMode) {
		if(!recModeWasActive) {
			drawLabels();
			recModeWasActive = true;
		}
		if(digitalRead(REC_TRIGGER_IN) == HIGH)
			recordPulse();
		else
			drawRecStats(false);
		return;
	}

	recModeWasActive = false;
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
