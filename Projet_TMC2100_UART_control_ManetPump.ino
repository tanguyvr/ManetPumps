///////// SERIAL /////////

bool newData = false;
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

// variables to hold the parsed data
char messageFromPC[numChars] = {0};
unsigned long paramFromPC = 0;



///////// STEPPER /////////

#define DIR_PIN   4  //direction
#define STEP_PIN  5  //step
#define EN_PIN    7  //enable

const int fullEndStopPin = 3; 
const int emptyEndStopPin = 2; 

#include <AccelStepper.h>

AccelStepper stepper = AccelStepper(1, STEP_PIN, DIR_PIN);

int STEPPER_SPEED;
unsigned long STEPPER_TARGET ;

unsigned long currentpos = 0;
unsigned long targetpos = 0;
unsigned long emptyStopPos = 0;
unsigned long fullStopPos = 20000000.0;



void setup() {

  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, false, true);
  stepper.disableOutputs();

  ///// SERIAL /////
  Serial.begin(115200);
  while(!Serial);
  //Serial.println("Serial Connected! <R:1254> :");

  ///// STEPPER /////



  pinMode(fullEndStopPin, INPUT_PULLUP);
  pinMode(emptyEndStopPin, INPUT_PULLUP);


  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(6000);

}

void loop() {
  Update();
}


void Update() {
  recvWithStartEndMarkers();
  if (newData == true) {
    strcpy(tempChars, receivedChars);
    parseData();
    // this temporary copy is necessary to protect the original data
    //   because strtok() used in parseData() replaces the commas with \0
    char command = receivedChars[0];

    switch(command){
      /*case 'I':
        initPos();
        break;*/

      //faire fois 800 pour le nombre de pas
      case 'S':
        Serial.print("Set Speed To: ");
        Serial.println(paramFromPC);
        STEPPER_SPEED = paramFromPC;
        break;

      case 'R':
        Serial.print("Run to position: ");
        Serial.println(paramFromPC);
        STEPPER_TARGET = paramFromPC;
        newData = false;
        runStepper();
        break;

      case 'Q':
        Serial.print("Quit: ");
        Serial.println(paramFromPC);
        stepper.moveTo(stepper.currentPosition());

      Serial.print(stepper.currentPosition());
      break;

      default:
      //Serial.println("Case not valid ...");
      break;
    }
    newData = false;
  }
}

///// SERIAL /////

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    if (Serial.available() > 0) {
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
  }
}

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,":");      // get the first part - the string
    strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC

    strtokIndx = strtok(NULL, ":"); // this continues where the previous call left off
    paramFromPC = atol(strtokIndx);     // convert this part to an long
}

///// STEPPER /////

void initPos(){

  stepper.enableOutputs();
  stepper.setSpeed(-4000);
  while (true){
    if(digitalRead(emptyEndStopPin) == LOW){break;}
    else{stepper.runSpeed();}
  }
  stepper.setCurrentPosition(-2000);

  stepper.setSpeed(4000);
  while (true){
    if(digitalRead(fullEndStopPin) == LOW){break;}
    else{stepper.runSpeed();}
  }
  
  fullStopPos = stepper.currentPosition();
  Serial.print("here is the fullstop pos");
  Serial.println(fullStopPos);
  Serial.println(stepper.targetPosition());
  Serial.println(stepper.currentPosition());

  stepper.moveTo(fullStopPos);
  stepper.setSpeed(-4000);
  
   while (stepper.currentPosition() != stepper.targetPosition()) {
      stepper.runSpeed();
    }
 
  stepper.disableOutputs();
}

void runStepper(){
  STEPPER_TARGET = constrain(STEPPER_TARGET, 0, fullStopPos);
  stepper.setCurrentPosition(0);
  stepper.moveTo(STEPPER_TARGET);
  
  if ((stepper.targetPosition() - stepper.currentPosition()) <= 0 ){
    stepper.setSpeed(-STEPPER_SPEED);
  }
  else{stepper.setSpeed(STEPPER_SPEED);}
  
  stepper.enableOutputs();
  while (stepper.currentPosition() != stepper.targetPosition()) {
      stepper.runSpeed();
      Update();
      //stepperStop();
    }
  Serial.print(stepper.currentPosition());
  stepper.disableOutputs();
}
