#include <SoftwareSerial.h>

struct BLE {
  String ID;
  String Name;
};

const String greeting = "boas";               // detected by the computer interface to check the connection to the arduino
const String IDintro = "00A05";               // sequence of characters which identifies the BLE modules with our configuration
const struct BLE BLEinnit = { IDintro, "" };  // BLE struct object with ID=IDintro and empty Name.
static struct BLE BLEs[10](BLEinnit);         // array of BLE structs. Each member of the array is initialized with the object BLEinnit
int n_epochs, stim_type = 49;
long time_off;
int control = 0, stim_control =1;
//int 1st_stim = 0;

static int parameters[6] = { '0', '0', '0', '0', '0', '0'};
static char info[6] = { '0', '0', '0', '0', '0', '0'};

unsigned long time1;
unsigned long time2;

const byte DLC_pin = 3;

SoftwareSerial HM(10, 11);  // connect HM-10 TXD and RXD to pin 10 and 11 on arduino, respectively

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);
  HM.begin(9600);
  pinMode(A2, OUTPUT);      // for debug
  pinMode(DLC_pin, INPUT);  // Pin that triggers stimulation writing request if active
  time1 = micros();
}

void loop() {

  static bool collectingID = false, getName = false, collectingName = false;
  static String SerialIncoming = "", BTincoming = "", devicename = "";
  static int n_BLEs = 0, blePairingState = 0;
  char c;
  
  if (digitalRead(DLC_pin) == HIGH){ stim_control = 1; } else{ stim_control = 0; } //Verify trigger pin status
  //if (stim_control == 1) { digitalWrite(A2, HIGH); delay(1000); digitalWrite(A2, LOW); delay(50); }

  //===== Read from Serial ================================================
  if (Serial.available() > 0 || stim_control == 1) { 
    // t1 = millis();
    // Serial.print(t1);
    SerialIncoming = Serial.readString();

    //===== Greeting used by pc to check connection =================
    if (SerialIncoming == greeting) {
      Serial.print(greeting);
      SerialIncoming = "";
      delay(100);
      Serial.write(blePairingState);
    }

    //===== BT search command =======================================
    else if (SerialIncoming == "BT") {
      HM.print("AT");
      HM.readString();
      blePairingState = 0;
      for (int i = 0; i < n_BLEs; i++) {
        BLEs[i].Name = "";
        BLEs[i].ID = IDintro;
      }
      n_BLEs = 0;
      HM.print("AT+DISC?");
      SerialIncoming = "";
    }

    //===== BT connect command ======================================
    else if (SerialIncoming.startsWith("connect.to.")) {
      devicename = SerialIncoming.substring(11, SerialIncoming.length());
      for (int i = 0; i < n_BLEs; i++) {
        if (BLEs[i].Name == devicename) {
          String conCommand = "AT+CON" + BLEs[i].ID + "76C42902"; //Construct AT command to pair based on the BLE device select on the interface
          HM.print(conCommand);
          SerialIncoming = "";
        }
      }
      
    }

    //===== BT disconnect command ====================================
    else if (SerialIncoming == "disconnect") {
      HM.print("AT");
      SerialIncoming = "";
    }

    //===== Check battery status command ==============================
    else if (SerialIncoming == "battlevel") {
      for (int i = 0; i<5; i++)
      /*{digitalWrite(A2, HIGH);
      delay(50);
      digitalWrite(A2, LOW);
      delay(50);}*/

      info[1] = 255;        //when frequency is set to 255, the BLE chip reads battery and pushs that info into the GATT service level
      HM.write(info);
      //info[1] = parameters[1];
      delay(50);
      int batt = HM.read(); // reads the value on GATT service
      while (batt < 0){     //sometimes it returns a null value, repeat operation until valid value
        //Serial.println(batt);
        //digitalWrite(A2, HIGH);
        HM.write(info);
        delay(100);
        digitalWrite(A2, LOW);
        batt = HM.read();
      }
      batt = batt + 200;    // due to parameter resolution
      Serial.println(batt);
      info[1] = parameters[1];
    }

    //===== Trigger command ===================================
    //----- wether it is received by the interface manual trigger button or by the input arduino pin (for closed-loop experiments)
    else if (SerialIncoming == "trigger" || stim_control == 1) {
      //digitalWrite(A2, HIGH); delay(1000); digitalWrite(A2, LOW); delay(50);
      //info[0] = 119; info[1] = 1; info[2] = 32 ; info[3] = 64; info[4] = 255;
      //int dur = 60 * 1000;
      //stim_type = 49 --> if a stimulation is still happening and a trigger is received, nothing happens until stim stops
      //stim_type != 49 --> if a stimulation is still happening and a trigger is received, counts how much time is still required to stimulate (to give response to the trigger during stim)
      if (stim_type == 49){
        
        for (int i = 0; i <= n_epochs; i++){
          HM.write(info);
          digitalWrite(A2, HIGH);
          delay(time_off);
          //delay(120050);  //delay(dur); delay(dur);
          digitalWrite(A2, LOW);
        }
      }
      else{
        time2 = micros();
        Serial.print("---");
        Serial.println((time2-time1) / 1000000);
        if ((time2 - time1)/1000000 < info[0]){
          Serial.println("2");
          //digitalWrite(A2, HIGH);
          //delay(5000);
          //delayMicroseconds(5*1000000-(time2-time1));
          Serial.print("->"); Serial.println(info[0]*1000000-(time2-time1));
          info[0] = (time2-time1)/1000000;
          HM.write(info);
          info[0] = 9;

        }
        else{
          Serial.println("1");
          HM.write(info);
          time1 = micros();
        }
        
      }
      
    }

    //===== Protocol parameters setting command ==============================
    else if (SerialIncoming.startsWith("protocol")) {
      
      stim_type = SerialIncoming[8];
      int del= SerialIncoming.indexOf(";");
      n_epochs = (SerialIncoming.substring(9, del)).toInt();
      time_off = (((SerialIncoming.substring(del+1)).toInt()) * 1000) + 50;
      //n_epochs = (SerialIncoming.substring(9, del)).toInt();
      //time_off = (SerialIncoming.substring(del+1)).toInt() * 1000;
      //Serial.println("n_epochs: "); Serial.print(n_epochs); Serial.print(" |  time_off: "); Serial.print(time_off);
      /*if (stim_type==49){
      digitalWrite(A2, LOW);
      delay(50);
      }*/

      //n_epochs = 5;      
      //time_off = 180000; 


      if (n_epochs != 0 and time_off != 0){
          control = 1;
      }
      //Serial.print(String(n_epochs)+" ; "+String(time_off));
      Serial.print("Parameters set\n");
    }

    //===== Stimulation parameters setting command ==============================
    else {
      parameterSeparation_4(SerialIncoming, parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5]);
      // parameters[0] => Total Duration (s)
      // parameters[1] => Frequency (Hz)
      // parameters[2] => Pulse Width (%)
      // parameters[3] => Intensity (mW)

      info[0] = parameters[0]; 
      info[1] = parameters[1];
      info[2] = parameters[2];
      info[3] = parameters[3];
      info[4] = parameters[4];
      info[5] = parameters[5];

      //info[0] = 9; info[1] = 1; info[2] = 50; info[3] = 100; info[4] = 255;
      Serial.print("Parameters set\n");

      /***************************************/
    }


  }

  //===== Read from BT module =============================================
  else if (HM.available() > 0) {
    c = HM.read();
    BTincoming = BTincoming + c;
    // Serial.print(c); //======================= FOR DEBUGGING ONLY==================

    //===== collecting ID =====================================
    if (collectingID == true) {
      BLEs[n_BLEs].ID = BLEs[n_BLEs].ID + c;
      if (BLEs[n_BLEs].ID.length() >= 12) {
        collectingID = false;
        getName = true;
        // Serial.print("\nID: " + BLEs[n_BLEs].ID + "\n"); //======================= FOR DEBUGGING ONLY==================
      }
    }

    //===== collecting Name ===================================
    if (collectingName == true) {
      BLEs[n_BLEs].Name = BLEs[n_BLEs].Name + c;
      if (BLEs[n_BLEs].Name.length() >= 11) {
        collectingName = false;
        getName = false;
        // Serial.print("\nName: " + BLEs[n_BLEs].Name + "\n"); //======================= FOR DEBUGGING ONLY==================
        n_BLEs++;
      }
    }

    //===== IDintro detection to start collecting ID ===========
    if (BTincoming.endsWith(IDintro))
      collectingID = true;

    //===== "Name:" detection to start collecting name =========
    else if (BTincoming.endsWith("NAME:") && getName == true)
      collectingName = true;

    //===== clean BTincoming when communication ends ===========
    else if (BTincoming.endsWith("DISCE")) {
      BTincoming = "";
      Serial.print("End of search\n");
      String devices = "";
      for (int n = 0; n < n_BLEs; n++)
        devices = devices + BLEs[n].Name + ';'; // string com o nome dos BLE devices detetados, separados por ";"
      Serial.println(devices);  // returns BLE devices names interspersed with ';'
    }
    //===== detect when BLE pairing occurred ===========
    else if (BTincoming.endsWith("CONNAOK+CONN")) {
      BTincoming = "";
      blePairingState = 1;
      Serial.println("Connected to " + devicename);
    }
    //===== detect when BLE unpairing occurred ===========
    else if (BTincoming.endsWith("OK+LOST")) {
      BTincoming = "";
      blePairingState = 0;
      Serial.print("Disconnected\n");
    }
  }
}

//==== Function to handle the stimulation parameters information ====//
void parameterSeparation_4(String incoming, int &parameter1, int &parameter2, int &parameter3, int &parameter4, int &parameter5,int &parameter6) { //only works for 6 parameters

  char delimiter = ';';  // the delimiter character

  int delimiter1 = incoming.indexOf(";");
  String parameter1str = incoming.substring(0, delimiter1);
  parameter1 = parameter1str.toInt();

  int delimiter2 = incoming.indexOf(";", delimiter1 + 1);
  String parameter2str = incoming.substring(delimiter1 + 1, delimiter2);
  parameter2 = parameter2str.toInt();

  int delimiter3 = incoming.indexOf(";", delimiter2 + 1);
  String parameter3str = incoming.substring(delimiter2 + 1, delimiter3);
  parameter3 = parameter3str.toInt();

  int delimiter4 = incoming.indexOf(";", delimiter3 + 1);
  String parameter4str = incoming.substring(delimiter3 + 1, delimiter4);
  parameter4 = parameter4str.toInt();

  int delimiter5 = incoming.indexOf(";", delimiter4 + 1);
  String parameter5str = incoming.substring(delimiter4 + 1, delimiter5);
  parameter5 = parameter5str.toInt();

  String parameter6str = incoming.substring(delimiter5 + 1);
  parameter6 = parameter6str.toInt();
}