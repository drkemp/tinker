/**
 ******************************************************************************
 * @file    application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/  
#include "application.h"
//#include "spark_disable_wlan.h"
#include "spark_disable_cloud.h"
#include "http.h"

/* 
 remote control 
 
 3 inputs from accelerometer (a5,a6,a7)
 5 outputs drive the scan rows (a0-a4)
 TX output drives the LED
 rx  not used
 8 inputs from the keys (d0-d7)
 
 drive all key inputs as pullups
 drive all keyscan outputs low for active
 
*/

// used for key debounce
unsigned int lastkey=0;
int keyhold=0;

int inputPins[] = {D0,D1,D2,D3,D4,D5,D6,D7};
String serverurl="secure-potion-546.appspot.com";
int MAXLOGS=20;
String pilelog[20] ;
int ptrlog=0;
int lastlog=0;

int getpin(int pin, int value) {
    if((1<<pin) & value) return LOW;
    return HIGH;
}

void setOutputs(int value){
  digitalWrite(A0,getpin(0,value));
  digitalWrite(A1,getpin(1,value));
  digitalWrite(A2,getpin(2,value));
  digitalWrite(A3,getpin(3,value));
  digitalWrite(A4,getpin(4,value));
}

void setLED(int state) {
    RGB.control(true);
    if(state) {
        RGB.color(120, 0, 0);
    } else {
        RGB.color(0, 0, 0);
    }
}

void setForSleep(){
  setOutputs(0x1F);  // turn on all scan outputs
  setLED(0);  // turn off LED
//  SPARK_WLAN_SLEEP = 1;
}

void powerup(){
//  SPARK_WLAN_SLEEP = 0;
}

void resetkey(){
  keyhold=0;
  lastkey=0;
}

void setup() {
    // set up the input pins
    pinMode(D0,INPUT_PULLUP);
    pinMode(D1,INPUT_PULLUP);
    pinMode(D2,INPUT_PULLUP);
    pinMode(D3,INPUT_PULLUP);
    pinMode(D4,INPUT_PULLUP);
    pinMode(D5,INPUT_PULLUP);
    pinMode(D6,INPUT_PULLUP);
    pinMode(D7,INPUT_PULLUP);
    
    // set up the output pins
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    pinMode(A4, OUTPUT);
    
    // set up the accelerometer pins
    pinMode(A5, INPUT);
    pinMode(A6, INPUT);
    pinMode(A7, INPUT);

    Serial.begin(9600);

    // tcp
    sethosturl(serverurl);
    resetkey();
    setForSleep();
}

void Log(String msg, bool send, bool cache){
    static int lnum=0;
    if(cache) {
       msg.concat(lnum++);
       pilelog[ptrlog++]=msg;
       ptrlog = ptrlog%20;
    }
    if(send) Serial.println(msg);
}

void Log(String msg){
   Log(msg,true,true);
}

void wifistate() {
   String sleep="NO";
   String dhcp="NO";
   if (SPARK_WLAN_SLEEP) { sleep="YES";}
   if(WLAN_DHCP) {dhcp="YES";}
   String b="Sleep: ";
   b.concat(sleep);
   Log(b,true,false);
   b="DHCP: ";
   b.concat(dhcp);
   Log(b,true,false);
}

void spewlog(){
    for(int i=0;i<20;i++){
        Serial.println(pilelog[i]);
    }
    Serial.println("-----");
    wifistate();
}

void sendkey(unsigned int k){
  Serial.print("K");
  Serial.print(k & 0xFF,HEX);
  Serial.print(",");
  Serial.print((k >> 8)& 0x07,HEX);
  Serial.println("");
  String s = "GET /key?b=";
  s.concat(k);
  httpsend( s);
}

int mapkey(unsigned int col, unsigned int row){
  // col can be 8 bits
  // row is 0-4 - call it 3 bits
  // I want to keep 0 as an error return, so add 1 to row

  unsigned int mapped =  col | (row+1)<<8;
  return mapped;
}

unsigned int readScanLine() {
  unsigned int c =0;
  for(int i=0;i<8;i++) {
    if( digitalRead(inputPins[i]) == LOW) {
      c |= (1<<i);
    };
  }
  return c;
}

unsigned int scanForKey(){
  // turn off all scan outputs, on LED
  setOutputs(0); // turn off key lines
  setLED(1);

  unsigned int k=0;
  for(unsigned int j=0;j<5;j++){
     setOutputs(1<<j);
     delay(10);
     unsigned int c= readScanLine();
     if(c){
       k=mapkey(c,j);
       break;
     }
  }
  setOutputs(0x1F);  // turn on key lines
  return k;
}

void processConsole(){
    int ib = Serial.read();
    Serial.println(ib, DEC);
    if(ib == 'l') {
      spewlog();
    } else if(ib=='k') {
        httpsend("GET /key?b=33");
    }
}
void processhost(){
    String s=readhost();
    Log(s, false,true);
}

int readanalog(){
    static int accmax=50;
    static int lastx=0;
    static int lasty=0;
    static int lastz=0;
    int gotmotion=0;
    int x=analogRead(A5);
    int y=analogRead(A6);
    int z=analogRead(A7);
    int diffx=abs(x-lastx);
    int diffy=abs(y-lasty);
    int diffz=abs(z-lastz);
    if(diffx>accmax || diffy>accmax || diffz>accmax){
        String s = "GET /accel?x=";
        s.concat(x);
        s.concat("&y=");
        s.concat(y);
        s.concat("&z=");
        s.concat(z);
        httpsend(s);

        lastx=(lastx+x)/2;
        lasty=(lasty+y)/2;
        lastz=(lastz+z)/2;
        gotmotion=1;
    }
    return gotmotion;
}
void loop() {
  if(readanalog()){
    powerup();
  }

  if(readScanLine()){
    powerup();
    // some button is down
    unsigned int k=scanForKey();
    if(k){
      if(k != lastkey){
        sendkey(k);
        lastkey=k;
        keyhold=1;
      } else {
        keyhold+=1;
      }
    } else {
      resetkey();
    }
  } else {
    resetkey();
  }
  if(!keyhold) {
    setForSleep();
  }
  if( Serial.available() > 0){
      processConsole();
  }
  if(host_hasdata()){
      processhost();
  }
  delay(500);
}


