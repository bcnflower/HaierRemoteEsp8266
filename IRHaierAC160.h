#include <algorithm>
#include <cstring>
#ifndef UNIT_TEST
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRtext.h>
#include <IRutils.h>
#include <IRsend.h>
#endif


using namespace std;

//Orignal Values----------------------
const uint16_t kHaierAcHdr = 3000;
const uint16_t kHaierAcHdrGap = 4300;
const uint16_t kHaierAcBitMark = 520;
const uint16_t kHaierAcOneSpace = 1650;
const uint16_t kHaierAcZeroSpace = 650;
const uint32_t kHaierAcMinGap = 150000; 
//-------------------------------------

//Modified Values----------------------
// const uint16_t kHaierAcHdr = 3000;
// const uint16_t kHaierAcHdrGap = 4350;
// const uint16_t kHaierAcBitMark = 586;
// const uint16_t kHaierAcOneSpace = 1675;
// const uint16_t kHaierAcZeroSpace = 550;
// const uint32_t kHaierAcMinGap = 150000; 
//-------------------------------------
const uint8_t kHaierAc160StateLength = 20;

const uint8_t kHaierAc160MinTempC = 16;
const uint8_t kHaierAc160MaxTempC = 30;
const uint8_t kHaierAc160DefTempC = 25;

const uint8_t kHaierAc160ModelA = 0xA6;
const uint8_t kHaierAc160ModelB = 0x59;

const uint8_t kHaierAc160Prefix2 = 0xB5;

const uint8_t kHaierAc160SwingVOff = 		0x0;
const uint8_t kHaierAc160SwingVTop = 		0x1;
const uint8_t kHaierAc160SwingVFront = 	0x2;  // Not available in heat mode.
const uint8_t kHaierAc160SwingVBottom = 	0x3;
const uint8_t kHaierAc160SwingVDown = 		0xA;   // Not available in cool mode.
const uint8_t kHaierAc160SwingVAuto = 		0xC;

const uint8_t kHaierAc160SwingHMiddle = 0x0;
const uint8_t kHaierAc160SwingHLeftMax = 0x3;
const uint8_t kHaierAc160SwingHLeft = 0x4;
const uint8_t kHaierAc160SwingHRight = 0x5;
const uint8_t kHaierAc160SwingHRightMax = 0x6;
const uint8_t kHaierAc160SwingHAuto = 0x7;

const uint8_t kHaierAc160FanHigh = 0b001; //1
const uint8_t kHaierAc160FanMed =  0b010; //2
const uint8_t kHaierAc160FanLow =  0b011; //3
const uint8_t kHaierAc160FanAuto = 0b101; //5

const uint8_t kHaierAc160Auto = 0b000;  // 0
const uint8_t kHaierAc160Cool = 0b001;  // 1
const uint8_t kHaierAc160Dry =  0b010;  // 2
const uint8_t kHaierAc160Heat = 0b100;  // 4
const uint8_t kHaierAc160Fan =  0b110;  // 6

const uint8_t kHaierAc160NoTimers       = 0b000;
const uint8_t kHaierAc160OffTimer       = 0b001;
const uint8_t kHaierAc160OnTimer        = 0b010;
const uint8_t kHaierAc160OnThenOffTimer = 0b100;
const uint8_t kHaierAc160OffThenOnTimer = 0b101;

const uint8_t kHaierAc160ButtonTempUp =   		0b00000;
const uint8_t kHaierAc160ButtonTempDown =		0b00001;
const uint8_t kHaierAc160ButtonSwingV =   		0b00010;
const uint8_t kHaierAc160ButtonSwingH =   		0b00011;
const uint8_t kHaierAc160ButtonFan =      		0b00100;
const uint8_t kHaierAc160ButtonPower =    		0b00101;
const uint8_t kHaierAc160ButtonMode =     		0b00110;
const uint8_t kHaierAc160ButtonHealth =   		0b00111;
const uint8_t kHaierAc160ButtonQuiteOrTurbo =   0b01000;
const uint8_t kHaierAc160ButtonSleep =    		0b01011;
const uint8_t kHaierAc160ButtonTimer =    		0b10000;
const uint8_t kHaierAc160ButtonLock =     		0b10100;
const uint8_t kHaierAc160ButtonLEDLight = 		0b10101;
const uint8_t kHaierAc160ButtonSelfClean =		0b11001;

const uint16_t kHaierAc160OneSpace = 1700;
const uint16_t kHaierAc160ZeroSpace = 550;
const uint16_t kHaierAc160BitMark = 550;
const uint16_t kHaierProntoLength = kHaierAc160StateLength*8*2 + 4 + 2;

struct IRHaierAC160RemoteState
{
	// Byte 0
	uint8_t Prefix1		:8;
	// Byte 1
	uint8_t SwingV		:4;
	uint8_t Temp		:4;
	// Byte 2
	uint8_t				:5;
	uint8_t SwingH		:3;
	// Byte 3
	uint8_t 			:1;
	uint8_t Health		:1;
	uint8_t				:3;
	uint8_t TimerMode	:3;
	// Byte 4
	uint8_t 			:6;
	uint8_t Power		:1;
	uint8_t				:1;
	// Byte 5
	uint8_t OffHours	:5;
	uint8_t Fan 		:3;
	// Byte 6
	uint8_t OffMinutes 	:6;
	uint8_t Turbo		:1;
	uint8_t Quiet		:1;
	// Byte 7
	uint8_t OnHours		:5;
	uint8_t Mode		:3;
	// Byte 8
	uint8_t OnMinutes	:6;
	uint8_t				:1;
	uint8_t	Sleep		:1;
	// Byte 9
	uint8_t				:8;
	// Byte 10
	uint8_t				:4;
	uint8_t SelfClean1	:1;
	uint8_t				:3;
	// Byte 11
	uint8_t				:8;
	// Byte 12
	uint8_t Button		:5;
	uint8_t	Lock		:1;
	uint8_t				:2;
	// Byte 13
	uint8_t Sum1		:8;
	// Byte 14
	uint8_t Prefix2		:8;
	// Byte 15
	uint8_t				:6;
	uint8_t	SelfClean2	:1;
	uint8_t				:1;
	// Byte 16
	uint8_t				:8;
	// Byte 17
	uint8_t				:8;
	// Byte 18
	uint8_t				:8;
	// Byte 19
	uint8_t Sum2		:8;
};


class IRHaierAC160
{
public:
	IRHaierAC160RemoteState _;
	#ifndef UNIT_TEST
    IRsend _irsend;
	IRHaierAC160(const uint16_t pin, const bool inverted = false, const bool use_modulation = true)
    : _irsend(pin, inverted, use_modulation){
		stateReset();
	}
	void begin(void) { _irsend.begin(); }
	#else
	IRHaierAC160(){
		stateReset();
	}
	#endif

	void stateReset(void){
		clearState();
		_.Prefix1 = kHaierAc160ModelA;
		_.Prefix2 = kHaierAc160Prefix2;	
	}

	void clearState(void){
		memset((void*)&_, 0x00, sizeof _);
	}

	void setModel(uint8_t model){
		_.Prefix1 = model;
	}

	void getProntoSigned(int *pronto){
		unsigned int bit = 0;
		pronto[bit++] = +3000;
		pronto[bit++] = -3100;
		pronto[bit++] = +3000;
		pronto[bit++] = -4450;
		pronto[bit++] = kHaierAc160BitMark;
		for(int i=0;i<kHaierAc160StateLength;i++){
			uint8_t byte = ((uint8_t*) &_)[i];
			for(int j=7;j>=0;j--){
				pronto[bit++] = (((byte>>j)&1)?~kHaierAc160OneSpace:~kHaierAc160ZeroSpace);
				pronto[bit++] = kHaierAc160BitMark;
			}
		}
		pronto[bit++] = -50100;
	}

	void getPronto(uint16_t *pronto){
		unsigned int bit = 0;
		pronto[bit++] = 3000;
		pronto[bit++] = 3100;
		pronto[bit++] = 3000;
		pronto[bit++] = 4450;
		pronto[bit++] = kHaierAc160BitMark;
		for(int i=0;i<kHaierAc160StateLength;i++){
			uint8_t byte = ((uint8_t*) &_)[i];
			for(int j=7;j>=0;j--){
				pronto[bit++] = (((byte>>j)&1)?kHaierAc160OneSpace:kHaierAc160ZeroSpace);
				pronto[bit++] = kHaierAc160BitMark;
			}
		}
		pronto[bit++] = 50100;
	}

	uint8_t getTemp(void) const {
		return _.Temp + kHaierAc160MinTempC;
	}

	void setTemp(const uint8_t degrees) {
		uint8_t temp = degrees;
		if (temp < kHaierAc160MinTempC)
			temp = kHaierAc160MinTempC;
		else if (temp > kHaierAc160MaxTempC)
			temp = kHaierAc160MaxTempC;

		uint8_t old_temp = getTemp();
		if (old_temp == temp) return;
		if (old_temp > temp)
			_.Button = kHaierAc160ButtonTempDown;
		else
			_.Button = kHaierAc160ButtonTempDown;
		_.Temp = temp - kHaierAc160MinTempC;
	}

	uint8_t getSwingV(void) const {
  return _.SwingV;
}

void setSwingV(const uint8_t pos) {
  if (pos == _.SwingV) return;  // Nothing to do.
  switch (pos) {
  case kHaierAc160SwingVOff :
	case kHaierAc160SwingVTop :
	case kHaierAc160SwingVFront :
	case kHaierAc160SwingVBottom :
	case kHaierAc160SwingVDown :
	case kHaierAc160SwingVAuto :
      _.Button = kHaierAc160ButtonSwingV;
      _.SwingV = pos;
      break;
  }
}

uint8_t getSwingH(void) const {
  return _.SwingH;
}

void setSwingH(uint8_t pos) {
	if (pos == _.SwingH) return;  // Nothing to do.
  switch (pos) {
    case kHaierAc160SwingHMiddle:
    case kHaierAc160SwingHLeftMax:
    case kHaierAc160SwingHLeft:
    case kHaierAc160SwingHRight:
    case kHaierAc160SwingHRightMax:
    case kHaierAc160SwingHAuto: 
	_.Button = kHaierAc160ButtonSwingH; break;
    default: return;  // Unexpected value so don't do anything.
  }
  _.SwingH = pos;
}

uint16_t getOnTimer(void) const {
  return _.OnHours * 60 + _.OnMinutes;
}

void setOnTimer(const uint16_t mins) {
  const uint16_t nr_mins = std::min((uint16_t)(23 * 60 + 59), mins);
  _.OnHours = nr_mins / 60;
  _.OnMinutes = nr_mins % 60;

  const bool enabled = (nr_mins > 0);
  uint8_t mode = getTimerMode();
  switch (mode) {
    case kHaierAc160OffTimer:
      mode = enabled ? kHaierAc160OffThenOnTimer : mode;
      break;
    case kHaierAc160OnThenOffTimer:
    case kHaierAc160OffThenOnTimer:
      mode = enabled ? kHaierAc160OffThenOnTimer : kHaierAc160OffTimer;
      break;
    default:
      mode = enabled << 1;
  }
  _.TimerMode = mode;
}

uint16_t getOffTimer(void) const {
  return _.OffHours * 60 + _.OffMinutes;
}

void setOffTimer(const uint16_t mins) {
  const uint16_t nr_mins = std::min((uint16_t)(23 * 60 + 59), mins);
  _.OffHours = nr_mins / 60;
  _.OffMinutes = nr_mins % 60;

  const bool enabled = (nr_mins > 0);
  uint8_t mode = getTimerMode();
  switch (mode) {
    case kHaierAc160OnTimer:
      mode = enabled ? kHaierAc160OnThenOffTimer : mode;
      break;
    case kHaierAc160OnThenOffTimer:
    case kHaierAc160OffThenOnTimer:
      mode = enabled ? kHaierAc160OnThenOffTimer : kHaierAc160OnTimer;
      break;
    default:
      // Enable/Disable the Off timer for the simple case.
      mode = enabled;
  }
  _.TimerMode = mode;
}

uint8_t getTimerMode(void) const { 
	return _.TimerMode;
 }

void setTimerMode(const uint8_t mode) {
  _.TimerMode = (mode > kHaierAc160OffThenOnTimer) ? kHaierAc160NoTimers : mode;
  switch (_.TimerMode) {
    case kHaierAc160NoTimers:
      setOnTimer(0);  // Disable the On timer.
      setOffTimer(0);  // Disable the Off timer.
      break;
    case kHaierAc160OffTimer:
      setOnTimer(0);  // Disable the On timer.
      break;
    case kHaierAc160OnTimer:
      setOffTimer(0);  // Disable the Off timer.
      break;
  }
}

bool getHealth(void) const {
  return _.Health;
}

void setHealth(const bool on) {
	if(on == _.Health) return;
  _.Button = kHaierAc160ButtonHealth;
  _.Health = on;
}

bool getSleep(void) const {
  return _.Sleep;
}

void setSleep(const bool on) {
	if(on == _.Sleep) return;
  _.Button = kHaierAc160ButtonSleep;
  _.Sleep = on;
}

bool getPower(void) const { return _.Power; }

void setPower(const bool on) {
	setSelfClean(false);
	if(on == _.Power) return;
  _.Button = kHaierAc160ButtonPower;
  _.Power = on;
}

void on(void) { setPower(true); }

void off(void) { setPower(false); }

uint8_t getFan(void) const { return _.Fan; }

void setFan(uint8_t speed) {
	if(speed == _.Fan) return;
  switch (speed) {
    case kHaierAc160FanLow:
    case kHaierAc160FanMed:
    case kHaierAc160FanHigh:
    case kHaierAc160FanAuto:
      _.Fan = speed;
      _.Button = kHaierAc160ButtonFan;
  }
}

uint8_t getMode(void) const { return _.Mode; }

void setMode(uint8_t mode) {
	setSelfClean(false);
	if(mode == _.Mode) return;
  switch (mode) {
    case kHaierAc160Fan:
		setSleep(false);
	case kHaierAc160Auto:
    case kHaierAc160Dry:
      // Turbo & Quiet is only available in Cool/Heat mode.
      _.Turbo = false;
      _.Quiet = false;
      // FALL-THRU
    case kHaierAc160Cool:
    case kHaierAc160Heat:
      _.Mode = mode;
      break;
	  // Unexpected, default to auto mode.
    default:
	  if (mode > kHaierAc160Fan) _.Mode =  kHaierAc160Auto;
  }
  _.Button = kHaierAc160ButtonMode;
}

bool getTurbo(void) const { return _.Turbo; }

void setTurbo(const bool on) {
	if(on == _.Turbo) return;
  	switch (getMode()) {
    case kHaierAc160Cool:
    case kHaierAc160Heat:
      _.Turbo = on;
      _.Button = kHaierAc160ButtonQuiteOrTurbo;
      if (on) _.Quiet = false;
  }
}

bool getQuiet(void) const { return _.Quiet; }

void setQuiet(const bool on) {
	if(on == _.Quiet) return;
  switch (getMode()) {
    case kHaierAc160Cool:
    case kHaierAc160Heat:
      _.Quiet = on;
      _.Button = kHaierAc160ButtonQuiteOrTurbo;
      if (on) _.Turbo = false;
  }
}

bool getLock(void) const { return _.Lock; }

void setLock(const bool on) {
	if(on == _.Lock) return;
  _.Button = kHaierAc160ButtonLock;
  _.Lock = on;
}

bool getSelfClean(void) const {return _.SelfClean1 && _.SelfClean2;}

bool setSelfClean(bool on){
	if(on == getSelfClean()) return true;
	if(on){
		switch (getMode())
		{
			case kHaierAc160Fan:
			case kHaierAc160Cool:
			case kHaierAc160Dry:
				_.SelfClean1 = 0b1;
				_.SelfClean2 = 0b1;
				_.Button = kHaierAc160ButtonSelfClean;
				return true;
				break;
			case kHaierAc160Auto:
			case kHaierAc160Heat:
				if(!getPower()){
					_.SelfClean1 = 0b1;
					_.SelfClean2 = 0b1;
					_.Button = kHaierAc160ButtonSelfClean;
					return true;
				}
			break;
			default:
			break;
		};
		return false;
	}else{
		_.SelfClean1 = 0b0;
		_.SelfClean2 = 0b0;
		return true;
	}
}

void toggleDisplayLED(){
	_.Button = kHaierAc160ButtonLEDLight;
	// send();
}

uint8_t sumBytes(const uint8_t * const start, const uint8_t length,const uint8_t init) {
  uint8_t checksum = init;
  const uint8_t *ptr;
  for (ptr = start; ptr - start < length; ptr++) checksum += *ptr;
  return checksum;
}

void checksum(void) {
  _.Sum1 = sumBytes((uint8_t*)&_,13 ,0);
  _.Sum2 = sumBytes((uint8_t*)&_+14,5,0);
}

bool validChecksum(uint8_t state[], const uint16_t length) {
  if (length < 2) return false;  // 1 byte of data can't have a checksum.
//   cout<<hex<<(unsigned int)state[length]<<" = "<<(unsigned int)sumBytes(state, length,0)<<endl;
  return (state[length] == sumBytes(state, length,0));
}

bool validateChecksum(void) {
	//   cout<<validChecksum((uint8_t*)&_,13)<<endl;
	//   cout<<validChecksum((uint8_t*)&_+14,5)<<endl;
	return validChecksum((uint8_t*)&_,13) && validChecksum((uint8_t*)&_+14,5);
}

uint8_t* getRaw(void) {
  checksum();
  return (uint8_t*)&_;
}

#ifndef UNIT_TEST
void send(const uint16_t repeat = 0){
// if (kHaierAc160StateLength < kHaierACStateLength) return;
  for (uint16_t r = 0; r <= repeat; r++) {
    _irsend.enableIROut(38000);
    _irsend.mark(kHaierAcHdr);
    _irsend.space(kHaierAcHdr);
    _irsend.sendGeneric(kHaierAcHdr, kHaierAcHdrGap, kHaierAcBitMark, kHaierAcOneSpace,
                kHaierAcBitMark, kHaierAcZeroSpace, kHaierAcBitMark,
                kHaierAcMinGap, getRaw(), kHaierAc160StateLength, 38, true,
                0,  // Repeats handled elsewhere
                50);
  }
}
  #endif

};
