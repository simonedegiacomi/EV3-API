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
 * The Initial Developer of this code is John Hansen.
 * Portions created by John Hansen are Copyright (C) 2009-2013 John Hansen.
 * All Rights Reserved.
 *
 */

#include "../include/ev3_output.h"

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))

static int __RAMP_UP_PCT = 10;
static int __RAMP_UP_DEGREES = 0;
static int __RAMP_DOWN_PCT = 10;
static int __RAMP_DOWN_DEGREES = 0;

typedef struct {
    int TachoCounts;
    int8_t Speed;
    int TachoSensor;
} MOTORDATA;

typedef struct {
    uint8_t Cmd;
    uint8_t Outputs;
    int8_t PwrOrSpd;
    int StepOrTime1;
    int StepOrTime2;
    int StepOrTime3;
    uint8_t Brake;
} StepOrTimePwrOrSpd;

typedef struct {
    uint8_t Cmd;
    uint8_t Outputs;
    int8_t Speed;
    short Turn;
    int StepOrTime;
    uint8_t Brake;
} StepOrTimeSync;

typedef struct {
    int8_t OutputTypes[NUM_OUTPUTS];
    short Owners[NUM_OUTPUTS];

    int PwmFile;
    int MotorFile;

    MOTORDATA MotorData[NUM_OUTPUTS];
    MOTORDATA *pMotor;
} OutputGlobals;

OutputGlobals OutputInstance;

uint8_t OutputToMotorNum(uint8_t Output) {
    switch (Output) {
        case OUT_A:
            return 0;
        case OUT_B:
            return 1;
        case OUT_C:
            return 2;
        case OUT_D:
            return 3;
    }
    return NUM_OUTPUTS;
}

void DecodeOutputs(uint8_t *outputs, uint8_t *layer) {
    *layer = *outputs & LAYER_MASK;
    *outputs = *outputs & OUT_MASK;
}

int WriteToPWMDevice(int8_t *bytes, int num_bytes) {
    int result = -1;
    if (OutputInstance.PwmFile >= 0) {
        // for some reason write is not returning num_bytes -
        // it usually returns zero
        result = write(OutputInstance.PwmFile, bytes, num_bytes);
        if (result >= 0) {
            return num_bytes;
        }
    }
    return result;
}

bool ResetOutputs(void) {
    // stop all the motors
    int i;
    for (i = 0; i < NUM_OUTPUTS; i++) {
        OutputInstance.Owners[i] = OWNER_NONE;
    }

    return OutputStop(OUT_ALL, false);
}

bool OutputInitialized(void) {
    return (OutputInstance.PwmFile != -1) &&
           (OutputInstance.pMotor != NULL);
}

bool OutputInit(void) {
    if (OutputInitialized())
        return true;

    MOTORDATA *pTmp;

    // To ensure that pMotor is never uninitialised
    OutputInstance.pMotor = OutputInstance.MotorData;

    // Open the handle for writing commands
    OutputInstance.PwmFile = open(LMS_PWM_DEVICE_NAME, O_RDWR);

    if (OutputInstance.PwmFile >= 0) {
        // Open the handle for reading motor values - shared memory
        OutputInstance.MotorFile = open(LMS_MOTOR_DEVICE_NAME, O_RDWR | O_SYNC);
        if (OutputInstance.MotorFile >= 0) {
            pTmp = (MOTORDATA *) mmap(0, sizeof(OutputInstance.MotorData), PROT_READ | PROT_WRITE,
                                      MAP_FILE | MAP_SHARED, OutputInstance.MotorFile, 0);
            if (pTmp == MAP_FAILED) {
//        LogErrorNumber(OUTPUT_SHARED_MEMORY);
                close(OutputInstance.MotorFile);
                close(OutputInstance.PwmFile);
                OutputInstance.MotorFile = -1;
                OutputInstance.PwmFile = -1;
            } else {
                OutputInstance.pMotor = pTmp;
                bool outputOpenTest =  OutputOpen();

                /**
                 * Set the polarity here to fix the issue:
                 * https://github.com/simonedegiacomi/EV3-API/issues/13
                 *
                 * Then simply use a negative speed/power to go in reverse
                 */
                 // TODO: If the fix works, change function name
                Fwd(OUT_ALL);
                Off(OUT_ALL);

                return outputOpenTest;
            }
        }
    }
    return false;
}

bool OutputOpen(void) {
    if (!OutputInitialized())
        return false;

    int8_t cmd;

    bool result = ResetOutputs();
    if (result) {
        cmd = opProgramStart;
        return WriteToPWMDevice(&cmd, 1) == 1;
    }

    return result;
}

bool OutputClose(void) {
    if (!OutputInitialized())
        return false;

    return ResetOutputs();
}


bool OutputExit(void) {
    if (!OutputInitialized())
        return true;

    // otherwise, close down the output module

    bool result = ResetOutputs();
    if (OutputInstance.MotorFile >= 0) {
        munmap(OutputInstance.pMotor, sizeof(OutputInstance.MotorData));
        close(OutputInstance.MotorFile);
        OutputInstance.MotorFile = -1;
    }
    if (OutputInstance.PwmFile >= 0) {
        close(OutputInstance.PwmFile);
        OutputInstance.PwmFile = -1;
    }
    return result;
}


bool OutputStop(uint8_t Outputs, bool useBrake) {
    if (!OutputInitialized())
        return false;
    int cmdLen = 3;
    int8_t cmd[3];
    uint8_t Layer;
    // opOutputStop (outputs, brake)
    // Stops the outputs (brake or coast)
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputStop;
        cmd[1] = Outputs;
        cmd[2] = useBrake;
        return WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    } else {
        return false;
        // support for daisychaining not yet implemented
        // TODO: Remove comment
/*

	  if (cDaisyReady() != BUSY)

	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_STOP;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Nos, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Brake, &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

// TODO: Do we really need this?
bool OutputProgramStop(void) {
    if (!OutputInitialized())
        return false;

    int8_t cmd;
    cmd = opProgramStop;
    return WriteToPWMDevice(&cmd, 1) == 1;
}

// TODO: Do we really need this?
bool OutputSetType(uint8_t Output, int8_t DeviceType) {
    if (!OutputInitialized())
        return false;

    uint8_t Layer;
    // opOutputSetType (output, type)  </b>
    // Set output device type
    DecodeOutputs(&Output, &Layer);
    if (Layer == LAYER_MASTER) {
        Output = OutputToMotorNum(Output);
        if (Output < NUM_OUTPUTS) {
            if (OutputInstance.OutputTypes[Output] != DeviceType) {
                OutputInstance.OutputTypes[Output] = DeviceType;
                // shouldn't this also write the information to the device?
                return OutputSetTypesArray(OutputInstance.OutputTypes);
            }
        }
        return false;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_RESET;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)No, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Type, &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

// TODO: Do we really need this?
bool OutputSetTypesArray(int8_t *pTypes) {
    return OutputSetTypes(pTypes[0], pTypes[1], pTypes[2], pTypes[3]);
}

// TODO: Do we really need this?
bool OutputSetTypes(int8_t OutputA, int8_t OutputB, int8_t OutputC, int8_t OutputD) {
    if (!OutputInitialized())
        return false;
    int cmdLen = 5;
    int8_t cmd[5];
    cmd[0] = opOutputSetType;
    cmd[1] = OutputA;
    cmd[2] = OutputB;
    cmd[3] = OutputC;
    cmd[4] = OutputD;
    bool result = WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    if (result) {
        int i;
        for (i = 0; i < NUM_OUTPUTS; i++) {
            if (OutputInstance.OutputTypes[i] != cmd[i + 1])
                OutputInstance.OutputTypes[i] = cmd[i + 1];
        }
    }
    return result;
}

bool OutputReset(uint8_t Outputs) {
    if (!OutputInitialized())
        return false;

    int cmdLen = 2;
    uint8_t Layer;
    int8_t cmd[2];
    // opOutputReset (outputs)
    // Resets the Tacho counts
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputReset;
        cmd[1] = Outputs;
        return WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_RESET;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Nos, &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputSpeed(uint8_t Outputs, int8_t Speed) {
    if (!OutputInitialized())
        return false;

    int cmdLen = 3;
    uint8_t Layer;
    int8_t cmd[3];
    // opOutputSpeed (outputs, speed)
    // Set speed of the outputs
    // (relative to polarity - enables regulation if the output has a tachometer)
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputSpeed;
        cmd[1] = Outputs;
        cmd[2] = Speed;
        return WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    } else {
        return false;
    }
}

bool OutputPower(uint8_t Outputs, int8_t Power) {
    if (!OutputInitialized())
        return false;
    int cmdLen = 3;
    uint8_t Layer;
    int8_t cmd[3];
    // opOutputPower (outputs, power)
    // Set power of the outputs
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputPower;
        cmd[1] = Outputs;
        cmd[2] = Power;
        return WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    } else {
        return false;
    }
}

// TODO: Do we really need this?
bool OutputStartEx(uint8_t Outputs, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = 2;
    uint8_t Layer;
    int8_t cmd[2];
    // opOutputStart (outputs)
    // Starts the outputs
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputStart;
        cmd[1] = Outputs;
        bool result = WriteToPWMDevice(cmd, cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_START;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Nos, &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		else
		{
		  //printf("cOutPut @ opOUTPUT_START after cDaisyDownStreamCmd - OK and WriteState = %d\n\r", cDaisyGetLastWriteState());
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);

	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputPolarity(uint8_t Outputs, int8_t Polarity) {
    if (!OutputInitialized())
        return false;

    int cmdLen = 3;
    uint8_t Layer;
    int8_t cmd[3];
    // opOutputPolarity (outputs, polarity)
    // Set polarity of the outputs
    //  - -1 makes the motor run backward
    //  -  1 makes the motor run forward
    //  -  0 makes the motor run the opposite direction
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputPolarity;
        cmd[1] = Outputs;
        cmd[2] = Polarity;
        return WriteToPWMDevice(cmd, cmdLen) == cmdLen;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_POLARITY;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Polarity[1], &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)Polarity[2], &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputStepPowerEx(uint8_t Outputs, int8_t Power, int Step1, int Step2, int Step3, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimePwrOrSpd);
    uint8_t Layer;
    StepOrTimePwrOrSpd cmd;
    // opOutputStepPower (outputs, power, step1, step2, step3, brake?)
    // Set Ramp up, constant and rampdown steps and power of the outputs
    DecodeOutputs(&Outputs, &Layer);
    cmd.Cmd = opOutputStepPower;
    cmd.Outputs = Outputs;
    cmd.PwrOrSpd = Power;
    cmd.StepOrTime1 = Step1;
    cmd.StepOrTime2 = Step2;
    cmd.StepOrTime3 = Step3;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_STEP_POWER;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Power, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Step1, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Step2, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Step3, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepPower.Brake, &(DaisyBuf[Len]));

		//if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepPower.Nos))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}

		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepPower.Nos);

	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputTimePowerEx(uint8_t Outputs, int8_t Power, int Time1, int Time2, int Time3, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimePwrOrSpd);
    uint8_t Layer;
    StepOrTimePwrOrSpd cmd;
    // opOutputTimePower (outputs, power, time1, time2, time3, brake?)
    // Set Ramp up, constant and rampdown steps and power of the outputs
    DecodeOutputs(&Outputs, &Layer);
    cmd.Cmd = opOutputTimePower;
    cmd.Outputs = Outputs;
    cmd.PwrOrSpd = Power;
    cmd.StepOrTime1 = Time1;
    cmd.StepOrTime2 = Time2;
    cmd.StepOrTime3 = Time3;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_TIME_POWER;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Power, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Time1, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Time2, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Time3, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimePower.Brake, &(DaisyBuf[Len]));
		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimePower.Nos))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimePower.Nos);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputStepSpeedEx(uint8_t Outputs, int8_t Speed, int Step1, int Step2, int Step3, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimePwrOrSpd);
    uint8_t Layer;
    StepOrTimePwrOrSpd cmd;
    // opOutputStepSpeed (outputs, power, step1, step2, step3, brake?)
    // Set Ramp up, constant and rampdown steps and speed of the outputs
    DecodeOutputs(&Outputs, &Layer);
    cmd.Cmd = opOutputStepSpeed;
    cmd.Outputs = Outputs;
    cmd.PwrOrSpd = Speed;
    cmd.StepOrTime1 = Step1;
    cmd.StepOrTime2 = Step2;
    cmd.StepOrTime3 = Step3;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_STEP_SPEED;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Speed, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Step1, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Step2, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Step3, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSpeed.Brake, &(DaisyBuf[Len]));

		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepSpeed.Nos))
		{
		  printf("NOT ok txed cOutputStepSpeed\n\r");
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		else
		{
		}
		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepSpeed.Nos);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputTimeSpeedEx(uint8_t Outputs, int8_t Speed, int Time1, int Time2, int Time3, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimePwrOrSpd);
    uint8_t Layer;
    StepOrTimePwrOrSpd cmd;
    // opOutputTimeSpeed (outputs, speed, time1, time2, time3, brake?)
    // Set Ramp up, constant and rampdown steps and power of the outputs
    DecodeOutputs(&Outputs, &Layer);
    cmd.Cmd = opOutputTimeSpeed;
    cmd.Outputs = Outputs;
    cmd.PwrOrSpd = Speed;
    cmd.StepOrTime1 = Time1;
    cmd.StepOrTime2 = Time2;
    cmd.StepOrTime3 = Time3;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_TIME_SPEED;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Speed, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Time1, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Time2, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Time3, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSpeed.Brake, &(DaisyBuf[Len]));
		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimeSpeed.Nos))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimeSpeed.Nos);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/

    }

}

bool OutputStepSyncEx(uint8_t Outputs, int8_t Speed, short Turn, int Step, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimeSync);
    uint8_t Layer;
    StepOrTimeSync cmd;
    // opOutputStepSync (outputs, speed, turn, step, brake?)
    DecodeOutputs(&Outputs, &Layer);
    // it is invalid to call this function with anything other than 2 motors:
    // i.e., OUT_AB, OUT_AC, OUT_AD, OUT_BC, OUT_BD, or OUT_CD
    if (!(Outputs == OUT_AB || Outputs == OUT_AC || Outputs == OUT_AD ||
          Outputs == OUT_BC || Outputs == OUT_BD || Outputs == OUT_CD))
        return false;
    cmd.Cmd = opOutputStepSync;
    cmd.Outputs = Outputs;
    cmd.Speed = Speed;
    cmd.Turn = Turn;
    cmd.StepOrTime = Step;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_STEP_SYNC;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSync.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSync.Speed, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSync.Turn,  &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSync.Step,  &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)StepSync.Brake, &(DaisyBuf[Len]));
		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepSync.Nos))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, StepSync.Nos);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputTimeSyncEx(uint8_t Outputs, int8_t Speed, short Turn, int Time, bool useBrake, uint8_t Owner) {
    if (!OutputInitialized())
        return false;

    int cmdLen = sizeof(StepOrTimeSync);
    uint8_t Layer;
    StepOrTimeSync cmd;
    // opOutputTimeSync (outputs, speed, turn, time, brake?)
    DecodeOutputs(&Outputs, &Layer);
    // it is invalid to call this function with anything other than 2 motors:
    // i.e., OUT_AB, OUT_AC, OUT_AD, OUT_BC, OUT_BD, or OUT_CD
    if (!(Outputs == OUT_AB || Outputs == OUT_AC || Outputs == OUT_AD ||
          Outputs == OUT_BC || Outputs == OUT_BD || Outputs == OUT_CD))
        return false;
    cmd.Cmd = opOutputTimeSync;
    cmd.Outputs = Outputs;
    cmd.Speed = Speed;
    cmd.Turn = Turn;
    cmd.StepOrTime = Time;
    cmd.Brake = (uint8_t) useBrake;
    if (Layer == LAYER_MASTER) {
        bool result = WriteToPWMDevice((int8_t * ) & (cmd.Cmd), cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.Owners[i] = Owner;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_TIME_SYNC;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSync.Nos,   &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSync.Speed, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSync.Turn,  &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSync.Time,  &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)TimeSync.Brake, &(DaisyBuf[Len]));
		if(OK != cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimeSync.Nos))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyMotorDownStream(DaisyBuf, Len, Layer, TimeSync.Nos);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputRead(uint8_t Output, int8_t *Speed, int *TachoCount, int *TachoSensor) {
    if (!OutputInitialized())
        return false;

    uint8_t Layer;
    // opOutputRead (output, *speed, *tachocount, *tachosensor)
    // Speed [-100..100]
    // Tacho count [-MAX .. +MAX]
    // Tacho sensor [-MAX .. +MAX]
    *Speed = 0;
    *TachoCount = 0;
    *TachoSensor = 0;
    DecodeOutputs(&Output, &Layer);
    if (Layer == LAYER_MASTER) {
        Output = OutputToMotorNum(Output);
        if (Output < NUM_OUTPUTS) {
            *Speed = OutputInstance.pMotor[Output].Speed;
            *TachoCount = OutputInstance.pMotor[Output].TachoCounts;
            *TachoSensor = OutputInstance.pMotor[Output].TachoSensor;
            return true;
        }
    }
    return false;
    // the firmware code does not contain any sign of letting you read
    // tacho values from other layers via this opcode.
}

// TODO: Rename
bool OutputTest(uint8_t Outputs, bool *isBusy) {
    if (!OutputInitialized())
        return false;

    uint8_t Layer;
    int test;
    int test2;
    char busyReturn[20]; // Busy mask
    bool result = false;

    *isBusy = false;
    test2 = 0;
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        if (OutputInstance.PwmFile >= 0) {
            size_t bytes_read = read(OutputInstance.PwmFile, busyReturn, 10);
            result = bytes_read > 0;
            sscanf(busyReturn, "%u %u", &test, &test2);
            *isBusy = ((Outputs & (uint8_t) test2) != 0);
        }
    } else {
//    if cDaisyCheckBusyBit(Layer, Outputs) )
        *isBusy = false;
    }
    return result;
}


// TODO: Do we need this?
bool OutputState(uint8_t Outputs, uint8_t *State) {
    if (!OutputInitialized())
        return false;

    uint8_t Layer;
    int test;
    int test2;
    char busyReturn[20]; // Busy mask
    bool result = false;

    *State = 0;
    test2 = 0;
    test = 0;
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        if (OutputInstance.PwmFile >= 0) {
            size_t bytes_read = read(OutputInstance.PwmFile, busyReturn, 10);
            result = bytes_read > 0;
            sscanf(busyReturn, "%u %u", &test, &test2);
            *State = test2;
        }
    } else {
//    if cDaisyCheckBusyBit(Layer, Outputs) )
        *State = 0;
    }
    return result;
}

bool OutputClearCount(uint8_t Outputs) {
    if (!OutputInitialized())
        return false;

    int cmdLen = 2;
    uint8_t Layer;
    int8_t cmd[2];
    DecodeOutputs(&Outputs, &Layer);
    if (Layer == LAYER_MASTER) {
        cmd[0] = opOutputClearCount;
        cmd[1] = Outputs;
        bool result = WriteToPWMDevice(cmd, cmdLen) == cmdLen;
        if (result) {
            int i;
            for (i = 0; i < NUM_OUTPUTS; i++) {
                if (Outputs & (0x01 << i))
                    OutputInstance.pMotor[i].TachoSensor = 0;
            }
        }
        return result;
    } else {
        return false;
/*
	  if (cDaisyReady() != BUSY)
	  {
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  0;
		DaisyBuf[Len++]  =  opOUTPUT_CLR_COUNT;
		Len             +=  cOutputPackParam((DATA32)0, &(DaisyBuf[Len]));
		Len             +=  cOutputPackParam((DATA32)ClrCnt[1], &(DaisyBuf[Len]));
		if(OK != cDaisyDownStreamCmd(DaisyBuf, Len, Layer))
		{
		  SetObjectIp(TmpIp - 1);
		  DspStat  =  BUSYBREAK;
		}
		//cDaisyDownStreamCmd(DaisyBuf, Len, Layer);
	  }
	  else
	  {
		SetObjectIp(TmpIp - 1);
		DspStat  =  BUSYBREAK;
	  }
*/
    }
}

bool OutputGetCount(uint8_t Output, int *Tacho) {
    int8_t speed = 0;
    int tcount = 0;
    return OutputRead(Output, &speed, &tcount, Tacho);
}

bool OutputGetTachoCount(uint8_t Output, int *Tacho) {
    int8_t speed = 0;
    int tsensor = 0;
    return OutputRead(Output, &speed, Tacho, &tsensor);
}

bool OutputGetActualSpeed(uint8_t Output, int8_t *Speed) {
    int tcount = 0;
    int tsensor = 0;
    return OutputRead(Output, Speed, &tcount, &tsensor);
}

void SetOutputEx(uint8_t Outputs, uint8_t Mode, uint8_t reset) {
    switch (Mode) {
        case OUT_FLOAT :
            OutputStop(Outputs, false);
            ResetCount(Outputs, reset);
            break;
        case OUT_OFF :
            OutputStop(Outputs, true);
            ResetCount(Outputs, reset);
            break;
        case OUT_ON :
            ResetCount(Outputs, reset);
            OutputStart(Outputs);
            break;
    }
}


void SetDirection(uint8_t Outputs, uint8_t Dir) {
    int8_t Polarity;
    switch (Dir) {
        case OUT_REV :
            Polarity = -1;
            break;
        case OUT_TOGGLE :
            Polarity = 0;
            break;
        default:
            Polarity = 1;
            break;
    }

    OutputPolarity(Outputs, Polarity);
}

void SetPower(uint8_t Outputs, int8_t Power) {
    OutputPower(Outputs, Power);
}

void SetSpeed(uint8_t Outputs, int8_t Speed) {
    OutputSpeed(Outputs, Speed);
}

void OnEx(uint8_t Outputs, uint8_t reset) {
    SetOutputEx(Outputs, OUT_ON, reset);
}

void OffEx(uint8_t Outputs, uint8_t reset) {
    SetOutputEx(Outputs, OUT_OFF, reset);
}

void FloatEx(uint8_t Outputs, uint8_t reset) {
    SetOutputEx(Outputs, OUT_FLOAT, reset);
}

void Fwd(uint8_t Outputs) { // TODO: Used only in initialization? change name
    SetDirection(Outputs, OUT_FWD);
}

void OnFwdEx(uint8_t Outputs, int8_t Power, uint8_t reset) {
    if (Power != OUT_POWER_DEFAULT)
        SetPower(Outputs, Power);
    OnEx(Outputs, reset);
}

void OnFwdRegEx(uint8_t Outputs, int8_t Speed, uint8_t RegMode, uint8_t reset) {
    SetSpeed(Outputs, Speed);
    OnEx(Outputs, reset);
}

void OnFwdSyncEx(uint8_t Outputs, int8_t Speed, short Turn, uint8_t reset) {
    ResetCount(Outputs, reset);
    OutputStepSyncEx(Outputs, Speed, Turn, INT_MAX, false, OWNER_NONE);
}

void RotateMotorNoWaitEx(uint8_t Outputs, int8_t Speed, int Angle, short Turn, bool Sync, bool Stop) {
    if (Sync) {
        uint8_t Layer, tmpOuts;
        tmpOuts = Outputs;
        DecodeOutputs(&tmpOuts, &Layer);
        if (tmpOuts == OUT_AB || tmpOuts == OUT_AC || tmpOuts == OUT_AD ||
            tmpOuts == OUT_BC || tmpOuts == OUT_BD || tmpOuts == OUT_CD) {
            OutputStepSyncEx(Outputs, Speed, Turn, Angle, Stop, OWNER_NONE);
            return;
        }
    }
    // otherwise use a non-synchronized API call
    int s3 = MIN(Angle / __RAMP_DOWN_PCT, __RAMP_DOWN_DEGREES), s1 = MIN(Angle / __RAMP_UP_PCT, __RAMP_UP_DEGREES), s2 =
            Angle - s3 - s1;
    OutputStepSpeedEx(Outputs, Speed, s1, s2, s3, Stop, OWNER_NONE);
}

void RotateMotorEx(uint8_t Outputs, int8_t Speed, int Angle, short Turn, bool Sync, bool Stop) {
    RotateMotorNoWaitEx(Outputs, Speed, Angle, Turn, Sync, Stop);
    bool busy;
    while (true) {
        Wait(MS_2); // 2ms between checks
        busy = false;
        OutputTest(Outputs, &busy);
        if (!busy) break;
    }
}

void OnForSyncEx(uint8_t Outputs, int Time, int8_t Speed, short Turn, bool Stop) {
    OutputTimeSyncEx(Outputs, Speed, Turn, Time, Stop, OWNER_NONE);
}

void OnForEx(uint8_t Outputs, int Time, int8_t Power, uint8_t reset) {
    if (Power != OUT_POWER_DEFAULT)
        SetPower(Outputs, Power);
    OnEx(Outputs, reset);
    usleep(Time);
    OffEx(Outputs, reset);
}

void ResetTachoCount(uint8_t Outputs) {
    // reset tacho counter(s)
    OutputReset(Outputs);
}

void ResetBlockTachoCount(uint8_t Outputs) {
    // synonym for ResetTachoCount
    ResetTachoCount(Outputs);
}

void ResetRotationCount(uint8_t Outputs) {
    // reset tacho counter(s)
    OutputClearCount(Outputs);
}

void ResetAllTachoCounts(uint8_t Outputs) {
    // clear all tacho counts
    OutputReset(Outputs);
    OutputClearCount(Outputs);
}

void ResetBlockAndTachoCount(uint8_t Outputs) {
    // synonym for ResetTachoCount
    ResetTachoCount(Outputs);
}

void ResetCount(uint8_t Outputs, uint8_t reset) {
    // reset tacho counter(s)
    switch (reset) {
        case RESET_COUNT :
            ResetTachoCount(Outputs);
            return;
        case RESET_BLOCK_COUNT :
            ResetBlockTachoCount(Outputs);
            return;
        case RESET_ROTATION_COUNT :
            ResetRotationCount(Outputs);
            return;
        case RESET_BLOCKANDTACHO :
            ResetBlockAndTachoCount(Outputs);
            return;
        case RESET_ALL :
            ResetAllTachoCounts(Outputs);
            return;
    }
}

int MotorTachoCount(uint8_t Output) {
    int Result = 0;
    OutputGetTachoCount(Output, &Result);
    return Result;
}

int MotorBlockTachoCount(uint8_t Output) {
    int Result = 0;
    OutputGetTachoCount(Output, &Result);
    return Result;
}

int8_t MotorPower(uint8_t Output) {
    int8_t Result = 0;
    OutputGetActualSpeed(Output, &Result);
    return Result;
}

int8_t MotorActualSpeed(uint8_t Output) {
    int8_t Result = 0;
    OutputGetActualSpeed(Output, &Result);
    return Result;
}

int MotorRotationCount(uint8_t Output) {
    int Result = 0;
    OutputGetCount(Output, &Result);
    return Result;
}

bool MotorBusy(uint8_t Output) {
    bool Result = false;
    OutputTest(Output, &Result);
    return Result;
}

