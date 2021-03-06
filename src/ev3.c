/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Initial Developer of this code is Simón Rodriguez Perez.
 * Portions created by Simón Rodriguez Perez are Copyright (C) 2014-2015 Simón Rodriguez Perez.
 * All Rights Reserved.
 *
 */

#include "ev3_inputs/ev3_input.h"
#include "ev3.h"

static bool initialized;
int __attribute__((constructor)) InitEV3 (void)
{
	if (EV3IsInitialized())
	    return 1;

	OutputInit();
    InputInit();
    SensorInit();
	ButtonLedInit();
	LcdInit();
	SoundInit();
	BluetoothInit();
    
	LcdClean();

	initialized = true;
	return 1;
}

int __attribute__((destructor)) FreeEV3()

{
	OutputExit();
    InputExit();
	ButtonLedExit();
	LcdExit();
	SensorExit();
	SoundExit();
	BluetoothExit();
	initialized = false;

	return 1;
}

bool EV3IsInitialized(void)
{
	return initialized;
}

char* EV3GetC4ev3Version() {
    return "0.1.0";
}
