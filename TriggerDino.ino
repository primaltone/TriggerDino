#define MAX_ANALOG 7
#define MAX_DIGITAL 13
struct PinType {
	char type;
	int number;
};

struct LevelPin {
	char polarity;
	struct PinType pin;
};
struct PWMPin {
	int dutycycle;
	struct PinType pin;
};
struct PulsePin {
	int firstPolarity;
	int secondPolarity;
	long firstHalf;
	long secondHalf;
	char state;
	int repeat;
	int currRepeat;
	unsigned long start;
	struct PinType pin;
	struct PulsePin *next;
};

struct PulsePin *pulsePinList=NULL;

int read_line(char* buffer, int bufsize)
{
	for (int index = 0; index < bufsize; index++) {
		// Wait until characters are available
		while (Serial.available() == 0) {
		}

		char ch = Serial.read(); // read next character
		Serial.print(ch); // echo it back: useful with the serial monitor (optional)
		// jwd may want to check for other chars since \n didn't work
		if ((ch == '\r')||(ch == '\n') ) {
			buffer[index] = 0; // end of line reached: null terminate string
			Serial.println(ch); // echo it back: useful with the serial monitor (optional)
			return index; // success: return length of string (zero if string is empty)
		}

		buffer[index] = ch; // Append character to buffer
	}

	// Reached end of buffer, but have not seen the end-of-line yet.
	// Discard the rest of the line (safer than returning a partial line).

	char ch;
	do {
		Serial.print("clearning buffer ");
		// Wait until characters are available
		while (Serial.available() == 0) {
		}
		ch = Serial.read(); // read next character (and discard it)
		Serial.print(ch); // echo it back
	} while (ch != '\n');

	buffer[0] = 0; // set buffer to empty string even though it should not be used
	return -1; // error: return negative one to indicate the input was too long
}

const int LINE_BUFFER_SIZE = 80; // max line length is one less than this

void setup() {
	Serial.begin(57600);
}

int getPin(char *str, struct PinType *pin) {
	char pinString[3];
	int rc=0;
	if (str == NULL) {
		rc = -1;	
	}
	else if ((str[0] != 'A') && (str[0] != 'D')) {
		/* must specifiy analog or digital */
		rc = -1;	
	}
	else {
		pin->type=str[0];
		strncpy(pinString,&str[1],sizeof(pinString)-1);
		pin->number=atoi(pinString);
		if ((pin->type == 'D') && ((pin->number < 0) || (pin->number > MAX_DIGITAL))){
			Serial.println("Digital pin out of range");
			rc=-1;
		}
		else if ((pin->type == 'A') && ((pin->number < 0) || (pin->number > MAX_ANALOG))){
			Serial.println("Analog pin out of range");
			rc=-1;
		}
	}

	return rc;	
}
int  validateLevel(char *str){
	int rc = 0;
	if (str == NULL) {
		Serial.println("Error: no level supplied");
		rc = -1;
	}
	else {
		rc = atoi(str);
		//Serial.println(rc);
	}
	if ((rc != 0) && (rc !=1)) {	
		Serial.println("Error: invalid level");
		rc = -1;
	}
	return rc;
}
long getDuration(char *str) {
	long rc = 1;
	int strLen=0;
	long multiplier=1000000; /* if units is seconds, convrt to us */
	if (str == NULL) {
		rc = 0;
	}
	else if ((strLen=strlen(str))>=2) {
		/* look for multiplier */
		char lastChar = str[strLen-1];
		if ((lastChar == 'M') || (lastChar == 'm')){
			multiplier=1000;
			str[strLen-1]='\0';
		}
		else if ((lastChar == 'U') || (lastChar == 'u')){
			multiplier=1;
			str[strLen-1]='\0';
		}
	}

	if (rc) rc = atoi(str)*multiplier;

	return rc;
}
void AddToList(struct PulsePin *ppulsePin){
	// jwd do I need to mutex protect?

	if (pulsePinList == NULL) {
		pulsePinList = ppulsePin; 
	}
	else {
		struct PulsePin *ppulsePinRunner=pulsePinList;
		while (ppulsePinRunner->next != NULL){
			ppulsePinRunner = ppulsePinRunner->next;
		}
		ppulsePinRunner->next = ppulsePin;
	}
}
void RemoveFromList(struct PulsePin *ppulsePin){
	// jwd do I need to mutex protect?

	if ((pulsePinList == NULL) || (ppulsePin == NULL)) {
		/* error need to return code*/
	Serial.println("error nothing to remove");
	}
	else {
		struct PulsePin *ppulsePinRunner=pulsePinList;

		/* check if first value */
		if (ppulsePinRunner == ppulsePin) {
			/* found it*/
			
			pulsePinList = ppulsePinRunner->next;
			free(ppulsePin);
		}
		else {
			do {
				if (ppulsePinRunner->next == ppulsePin) {
					/* found it*/
					ppulsePinRunner = ppulsePin->next;
					free(ppulsePin);
					break;
				}
				else {
					ppulsePinRunner = ppulsePinRunner->next;
				}
			} while(ppulsePinRunner!=NULL);		
		}

	}
}
void parseCommand() {
	char command[LINE_BUFFER_SIZE];
	char * pch;
	struct PinType pin{'U',-1};

	if (read_line(command, sizeof(command)) < 0) {
		Serial.println("Error: line too long");
		return; // skip command processing and try again on next iteration of loop
	}

	for (int i=0; i< strlen(command);i++){
		command[i]=toupper(command[i]);
	}

	pch = strtok (command," ");
	while (pch != NULL)
	{
		/* get pin */
		if ((pin.number == -1) && (getPin(pch,&pin) >= 0)){
		}
		else if (strcmp(pch, "OUT") == 0) {
			int level;
			pch = strtok (NULL, " ");
			if ((level = validateLevel(pch)) >= 0){
				if (pin.type == 'D') {
					pinMode(pin.number,OUTPUT);
					digitalWrite(pin.number,level);
				}
			}
		}
		else if (strcmp(pch, "PULSE") == 0) {
			struct PulsePin *ppulsePin = (struct PulsePin *)malloc(sizeof(struct PulsePin));	
			if (ppulsePin == NULL) {
				Serial.println("failed to allocate memory ");
			}
			else if ((ppulsePin->firstPolarity = validateLevel(pch = strtok (NULL, " "))) < 0){

			}
			else if (( ppulsePin->firstHalf = getDuration(pch = strtok (NULL, " "))) < 0){
				Serial.print("error pulseDuration: ");
			}
			else if (( ppulsePin->secondHalf = getDuration(pch = strtok (NULL, " "))) < 0){
				Serial.print("error postDuration: ");
			}
			else {
				ppulsePin->repeat = atoi(pch = strtok (NULL, " "));
				//ppulsePin->secondPolarity=(ppulsepin->firstPolarity==LOW)?HIGH:LOW;
				ppulsePin->secondPolarity=!(ppulsePin->firstPolarity);
				ppulsePin->currRepeat = 0;
				ppulsePin->state='I';
				ppulsePin->next = NULL;
				ppulsePin->pin.number=pin.number;
				ppulsePin->pin.type=pin.type;
				AddToList(ppulsePin);
			}
			//Serial.print("firstPolarity: ");Serial.println(ppulsePin->firstPolarity);
			//Serial.print("pulseDuration: ");Serial.println(ppulsePin->firstHalf);
			//Serial.print("postDuration: ");Serial.println(ppulsePin->secondHalf);
			//Serial.print("repeat: ");Serial.println(ppulsePin->repeat);
		}
		else if (strcmp(command, "IN") == 0) {
		}
		else {
			Serial.print("Error: unknown command: \"");
			Serial.print(command);
		}

		pch = strtok (NULL, " ");
	}
}

void HandleTimers(struct PulsePin *pinList) {
	struct PulsePin *ppulsePinRunner = pinList;
	while (ppulsePinRunner != NULL) {
		if (ppulsePinRunner->state == 'I') { /* init*/
			ppulsePinRunner->start = micros();
			ppulsePinRunner->state = 'F';
			pinMode(ppulsePinRunner->pin.number,OUTPUT);
			digitalWrite(ppulsePinRunner->pin.number,ppulsePinRunner->firstPolarity);
		}
		else if (ppulsePinRunner->state == 'F'){
			if ((micros() - ppulsePinRunner->start) > ppulsePinRunner->firstHalf) {
				digitalWrite(ppulsePinRunner->pin.number,ppulsePinRunner->secondPolarity);
				if (ppulsePinRunner->secondHalf) {
					/* move to second half if valid */
					ppulsePinRunner->start = micros();
					ppulsePinRunner->state = 'S';
				}
				else {
					ppulsePinRunner->state = 'C'; /* complete */
					RemoveFromList(ppulsePinRunner);
				}
			}
		}
		else if (ppulsePinRunner->state == 'S'){
			if ((micros() - ppulsePinRunner->start) > ppulsePinRunner->secondHalf) {
				/* finish or repeat? */
				if (++ppulsePinRunner->currRepeat < ppulsePinRunner->repeat ){
					ppulsePinRunner->start = micros();
					ppulsePinRunner->state = 'F'; /* keep repeating */
					digitalWrite(ppulsePinRunner->pin.number,ppulsePinRunner->firstPolarity);
				}
				else {
					ppulsePinRunner->state = 'C'; /* complete */
					RemoveFromList(ppulsePinRunner);
				}
			}
		}
		ppulsePinRunner=ppulsePinRunner->next;
	}
}


void loop() {
	/* handle timers */

	HandleTimers(pulsePinList);

	if (Serial.available() != 0) {
		parseCommand();
	}
}
#if 0

> D10 OUT 0
> D10 PULSE HIGH 10u /* set low, keep high for 10us, drop low */
> D10 PULSE HIGH 10m /* set low, keep high for 10ms, drop low */
> D10 PULSE HIGH 10 /* same, but for 10s*/ 
> D10 pulse High 10u 1u /* same as high pulse, but delay 1u after low */ 
> D10 pulse High 10u 1u 5 /* same as high pulse, but delay 1u after low, repeat 5 times */ 
> D10 PWM 40 /* Duty cycle = 40%, stays low 40%, high 60%*/


// 1) PIN - can have format D1, D12, A0
// 2) direction
// 3) if output, value
// 4) later, if input return value
// 5) other options: PWM, pulse (need direction, duration; repeat

/*

   PIN #:  can have format D0 - D19, A0-A7
   FUNCTION: OUT, IN
   OUT: LEVEL, PULSE, PWM
   0/1: Set Level
   PULSE:
   Direction (L/H), Duration
   Repeat, time b/w pulses
   PWM: set duty cycle

   IN: for now, read ADC value or digital value
   Later: read pwm?

 */

#endif
