#define MAX_ANALOG 7
#define MAX_DIGITAL 13
int read_line(char* buffer, int bufsize)
{
	for (int index = 0; index < bufsize; index++) {
		// Wait until characters are available
		while (Serial.available() == 0) {
		}

		char ch = Serial.read(); // read next character
// jwd		Serial.print(ch); // echo it back: useful with the serial monitor (optional)
		// jwd may want to check for other chars since \n didn't work
		if ((ch == '\r')||(ch == '\n') ) {
			buffer[index] = 0; // end of line reached: null terminate string
			// jwdSerial.println(ch); // echo it back: useful with the serial monitor (optional)
			return index; // success: return length of string (zero if string is empty)
		}

		buffer[index] = ch; // Append character to buffer
	}

	// Reached end of buffer, but have not seen the end-of-line yet.
	// Discard the rest of the line (safer than returning a partial line).

	char ch;
	do {
	//jwd	Serial.print("clearning buffer ");
		// Wait until characters are available
		while (Serial.available() == 0) {
		}
		ch = Serial.read(); // read next character (and discard it)
		// jwdSerial.print(ch); // echo it back
	} while (ch != '\n');

	buffer[0] = 0; // set buffer to empty string even though it should not be used
	return -1; // error: return negative one to indicate the input was too long
}

const int LINE_BUFFER_SIZE = 80; // max line length is one less than this

void setup() {
	Serial.begin(57600);
}
struct PinType {
	char type;
	int number;
};
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
			//jwdSerial.println("Digital pin out of range");
			rc=-1;
		}
		else if ((pin->type == 'A') && ((pin->number < 0) || (pin->number > MAX_ANALOG))){
			//jwdSerial.println("Analog pin out of range");
			rc=-1;
		}
	}

	return rc;	
}
int  validateLevel(char *str){
	int rc = 0;
	if (str == NULL) {
	//jwd	Serial.println("Error: no level supplied");
		rc = -1;
	}
	else {
		rc = atoi(str);
		// jwdSerial.println(rc);
	}
	if ((rc != 0) && (rc !=1)) {	
		//jwdSerial.println("Error: invalid level");
		rc = -1;
	}
	return rc;
}
void loop() {
	// jwdSerial.print("> ");

	// Read command

	char command[LINE_BUFFER_SIZE];
	if (read_line(command, sizeof(command)) < 0) {
		//jwdSerial.println("Error: line too long");
		return; // skip command processing and try again on next iteration of loop
	}


	char * pch;
	struct PinType pin{'U',-1};

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
					digitalWrite(pin.number,(level==0)?LOW:HIGH);
				}
				// configure if need be, or just set. add param for configured to struct?
			
			}
		} else if (strcmp(command, "IN") == 0) {
		} else {
			//jwdSerial.print("Error: unknown command: \"");
			//jwdSerial.print(command);
		}
			
		pch = strtok (NULL, " ");
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
