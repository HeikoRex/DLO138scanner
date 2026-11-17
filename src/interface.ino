

// ------------------------
const char* getTimebaseLabel()	{
// ------------------------
	return tbNames[currentTimeBase];
}



// interface operations defined below

long lastBtnPress = 0;



// ------------------------
void btn4ISR()	{
// ------------------------
	static boolean pressed = false;
	static long pressedTime = 0;

	if(!pressed && (digitalRead(BTN4) == LOW))	{
		if(millis() - pressedTime < BTN_DEBOUNCE_TIME)
			return;
		pressedTime = millis();
		pressed = true;
	}

	if(pressed && (digitalRead(BTN4) == HIGH))	{
		if(millis() - pressedTime < 5)
			return;
		pressed = false;

		if(millis() - pressedTime < 1000)	{
			// short press: cycle Run -> Hold -> REC
			if(recMode) {
				// REC -> Run
				recMode = false;
				hold = false;
				keepSampling = false;
				USB_PRINTLN("REC MODE OFF");
			} else if(hold) {
				// Hold -> REC
				hold = false;
				recMode = true;
				recHead = 0;
				recCount = 0;
				keepSampling = false;
				USB_PRINTLN("REC MODE ON");
			} else {
				// Run -> Hold
				hold = true;
			}
			repaintLabels();
		}
		else	{
			// long press: reset parameter
			resetParam();
		}
	}
}



// ------------------------
void readESwitchISR()	{
// ------------------------
	if(millis() - lastBtnPress < BTN_DEBOUNCE_TIME)
		return;
	lastBtnPress = millis();

	focusNextLabel();
	repaintLabels();
	if(hold)
		drawWaves();
	if(triggerType != TRIGGER_AUTO)
		keepSampling = false;
}




// ------------------------
void resetParam()	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_triggerLevel:
			// set trigger level to 0
			setTriggerLevel(0);
			saveParameter(PARAM_TLEVEL, 0);
			repaintLabels();
			break;
		case L_window:
			// set x in the middle
			changeXCursor((NUM_SAMPLES - GRID_WIDTH)/2);
			break;
		case L_vPos1:
			// zero the trace base
			calculateTraceZero(0);
			changeYCursor(0, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos2:
			// zero the trace base
			calculateTraceZero(1);
			changeYCursor(1, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos3:
			changeYCursor(2, -GRID_HEIGHT/2 - 1);
			break;
		case L_vPos4:
			changeYCursor(3, -GRID_HEIGHT/2 - 1);
			break;
		default:
			// toggle stats printing
			printStats = !printStats;
			saveParameter(PARAM_STATS, printStats);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}




// ------------------------
void calculateTraceZero(int waveID)		{
// ------------------------
	if(couplingPos != CPL_GND || waveID != 0)
		return;

	int32_t sumSamples = 0;
	for(uint16_t k = 0; k < NUM_SAMPLES; k++)
		sumSamples += ch1Capture[k];

	zeroVoltageA1 = sumSamples / NUM_SAMPLES;
	saveParameter(PARAM_ZERO1, zeroVoltageA1);
}




// ------------------------
void encoderChanged(int steps)	{
// ------------------------
	// which label has current focus
	switch(currentFocus)	{
		case L_timebase:
			if(steps > 0) decrementTimeBase(); else	incrementTimeBase();
			break;
		case L_triggerType:
			if(steps > 0) incrementTT(); else decrementTT();
			break;
		case L_triggerEdge:
			if(steps > 0) setTriggerRising(); else setTriggerFalling();
			break;
		case L_triggerLevel:
			if(steps > 0) incrementTLevel(); else decrementTLevel();
			break;
		case L_waves:
			if(steps > 0) incrementWaves(); else decrementWaves();
			break;
		case L_window:
			if(steps > 0) changeXCursor(xCursor + XCURSOR_STEP); else changeXCursor(xCursor - XCURSOR_STEP);
			break;
		case L_vPos1:
			if(steps > 0) changeYCursor(0, yCursors[0] - YCURSOR_STEP); else changeYCursor(0, yCursors[0] + YCURSOR_STEP);
			break;
		case L_vPos2:
			if(steps > 0) changeYCursor(1, yCursors[1] - YCURSOR_STEP); else changeYCursor(1, yCursors[1] + YCURSOR_STEP);
			break;
		case L_vPos3:
			if(steps > 0) changeYCursor(2, yCursors[2] - YCURSOR_STEP); else changeYCursor(2, yCursors[2] + YCURSOR_STEP);
			break;
		case L_vPos4:
			if(steps > 0) changeYCursor(3, yCursors[3] - YCURSOR_STEP); else changeYCursor(3, yCursors[3] + YCURSOR_STEP);
			break;
	}
	
	// manually update display if frozen
	if(hold)
		drawWaves();
	
	if(triggerType != TRIGGER_AUTO)
		// break the sampling loop
		keepSampling = false;
}



// ------------------------
void incrementTLevel()	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL + 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}



// ------------------------
void decrementTLevel()	{
// ------------------------
	int16_t tL = getTriggerLevel();
	setTriggerLevel(tL - 5);
	saveParameter(PARAM_TLEVEL, tL);
	repaintLabels();
}


// A1, D1, D2, A2 
// 0, 2, 3, 1
// ------------------------
void incrementWaves()	{
// ------------------------
	// add more waves
	if(waves[1])	{
		return;
	}
	else if(waves[3])	{
		waves[1] = true;
		saveParameter(PARAM_WAVES + 1, waves[1]);
	}
	else if(waves[2])	{
		waves[3] = true;
		saveParameter(PARAM_WAVES + 3, waves[3]);
	}
	else {
		waves[2] = true;
		saveParameter(PARAM_WAVES + 2, waves[2]);
	}

	repaintLabels();
}




// ------------------------
void decrementWaves()	{
// ------------------------
	// remove waves
	if(waves[1])	{
		waves[1] = false;
		saveParameter(PARAM_WAVES + 1, waves[1]);
	}
	else if(waves[3])	{
		waves[3] = false;
		saveParameter(PARAM_WAVES + 3, waves[3]);
	}
	else if(waves[2])	{
		waves[2] = false;
		saveParameter(PARAM_WAVES + 2, waves[2]);
	}
	else {
		return;
	}
	
	repaintLabels();
}



// ------------------------
void setTriggerRising()	{
// ------------------------
	if(triggerRising)
		return;
	
	setTriggerRising(true);
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
}



// ------------------------
void setTriggerFalling()	{
// ------------------------
	if(!triggerRising)
		return;
	
	setTriggerRising(false);
	saveParameter(PARAM_TRIGDIR, triggerRising);
	repaintLabels();
}




// ------------------------
void incrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_SINGLE)
		return;
	
	setTriggerType(triggerType + 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}



// ------------------------
void decrementTT()	{
// ------------------------
	if(triggerType == TRIGGER_AUTO)
		return;
	setTriggerType(triggerType - 1);
	// trigger type is not saved
	// saveParameter(PARAM_TRIGTYPE, triggerType);
	repaintLabels();
}




// ------------------------
void incrementTimeBase()	{
// ------------------------
	if(currentTimeBase == T50MS)
		return;
	
	setTimeBase(currentTimeBase + 1);
}



// ------------------------
void decrementTimeBase()	{
// ------------------------
	if(currentTimeBase == T20US)
		return;
	
	setTimeBase(currentTimeBase - 1);
}



// ------------------------
void setTimeBase(uint8_t timeBase)	{
// ------------------------
	currentTimeBase = timeBase;
	setSamplingRate(timeBase);
	saveParameter(PARAM_TIMEBASE, currentTimeBase);
	// request repainting of screen labels
	repaintLabels();
}




// ------------------------
void toggleWave(uint8_t num)	{
// ------------------------
	waves[num] = !waves[num];
	saveParameter(PARAM_WAVES + num, waves[num]);
	repaintLabels();
}



// ------------------------
void changeYCursor(uint8_t num, int16_t yPos)	{
// ------------------------
	if(yPos > 0)
		yPos = 0;
	
	if(yPos < -GRID_HEIGHT)
		yPos = -GRID_HEIGHT;

	yCursors[num] = yPos;
	saveParameter(PARAM_YCURSOR + num, yCursors[num]);
	repaintLabels();
}



// ------------------------
void changeXCursor(int16_t xPos)	{
// ------------------------
	if(xPos < 0)
		xPos = 0;
	
	if(xPos > (NUM_SAMPLES - GRID_WIDTH))
		xPos = NUM_SAMPLES - GRID_WIDTH;
	
	xCursor = xPos;
	saveParameter(PARAM_XCURSOR, xCursor);
	repaintLabels();
}



