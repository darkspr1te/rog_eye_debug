/*
 * 
 *See README.md 
 *darkspr1te
 * 
 */
 
#include <Wire.h>
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <ILI9488.h>
#include <EEPROM.h>
#include "logo.h"

#define TFT_CS         10
#define TFT_DC         9
#define TFT_RST        7

//math different between cpu makers it seems, online has AMD /INTEL swapped, 1.28/0.005 is correct for AMD/C7H boards
#define CPU_TYPE AMD
#if CPU_TYPE == AMD
  #define VDIV1 1.28
  #define VDIV2 0.005
  
#else
  #define VDIV1 0.6475
  #define VDIV2 0.0025
#endif

//clock deviders correct for intel/amd
#define CDIV1 25.6
#define CDIV2 0.1


#define CPU_TEMP  0x50
#define QCODE 0x01
#define XCODE 0x8


#define TVOLT    0x00
#define TCLOCK   0x01
#define TRATIO   0x02
#define TFAN     0x03
#define TTEMP    0x04
#define TQCODE   0x05
#define TOPEID   0x06

//ROG_EXT devices responde to 0x4a 
#define ROG_EXT 0x4A

#define ADDRESSES 11
#define BUTTON_BLUE PC13
#define ROT_BUTTON PB5
#define ROT_A PA10
#define ROT_B PB3

//to enable ROG_EXT bring ITWOC_ENABLE low 
#define ITWOC_ENABLE D8 //arduino D2

/*
 * define for mem locations direct
 */

#define TX_LED A0
#define RX_LED A1
#define RX_RAMV A3
#define RX_CPUV A2


#define ROG_LOGO_X_SIZE 146
#define ROG_LOGO_Y_SIZE 80
#define ROG_LOGO_X ILI9488_TFTHEIGHT-ROG_LOGO_X_SIZE
//#define ROG_LOGO_Y ILI9488_TFTWIDTH-ROG_LOGO_Y_SIZE
#define ROG_LOGO_Y 0



typedef struct {
    int addr;
    char bytes;
    char type;
    char * name;
} addrStruct;
//From https://github.com/kevinlekiller/rogext.git
//for reference only as these structure are for talking to Elmor labs ROG_EXT_OUT module (REOM1B) [URL]Elmor Labs[https://elmorlabs.com/index.php/hardware/{/URL]
addrStruct addresses[ADDRESSES] = {
    {0x00, 1, TOPEID, "OPEID"},
//    {0x01, 1 
//    {0x07, 1
    {0x10, 1, TQCODE, "QCODE"},
//    {0x12, 1
    {0x20, 1, TRATIO, "CPU Ratio"},
//    {0x22, 1
    {0x24, 1, TRATIO, "Cache Ratio"},
    {0x28, 2, TCLOCK, "BCLK"},
//    {0x2a, 2 ; PCIEBCLK?
//    {0x2c, 2
//    {0x2e, 2
    {0x30, 2, TVOLT,  "V1"},
//    {0x32, 2
//    {0x34, 2
//    {0x36, 2
    {0x38, 2, TVOLT,  "V2"},
//    {0x3a, 2
//    {0x3c, 2 ; 1.8v
//    {0x3e, 2
    {0x40, 2, TVOLT,  "VCORE"},
//    {0x42, 2
//    {0x44, 2
//    {0x46, 2
    {0x48, 2, TVOLT,  "VDRAM"},
//    {0x4c, 2
    {0x50, 1, TTEMP,  "CPU Temp"},
    {0x60, 2, TFAN,   "CPU Fanspeed"},
};

ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_RST);

byte buf1, buf2;
#define MEM_SIZE 0xff

byte memory[MEM_SIZE] {};
byte alt_memory[MEM_SIZE] {};
byte temp_array[ILI9488_TFTHEIGHT+5] {};
byte First_Run =0;


#define CPU_LOGO_X_SIZE 162
#define CPU_LOGO_Y_SIZE 89
#define CPU_LOGO_X 0
#define CPU_LOGO_Y 100


void setup() {

  Wire.begin(ROG_EXT);                // join i2c bus with address 0x4A
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(read_mem);//register a request event 
  pinMode(SCL, INPUT);  // remove internal pullup
  pinMode(BUTTON_BLUE,INPUT);
  pinMode(TX_LED,OUTPUT);
  pinMode(RX_LED,OUTPUT);
  pinMode(ITWOC_ENABLE,OUTPUT);
  pinMode(ROT_BUTTON,INPUT_PULLUP);
  pinMode(RX_RAMV,OUTPUT);
  pinMode(ROT_A,INPUT_PULLUP);
  pinMode(ROT_B,INPUT_PULLUP);
  pinMode(RX_CPUV,OUTPUT);
  
  digitalWrite(ITWOC_ENABLE, HIGH);//This enables the i2c/smbus output of the ROG_EXT connector 
  digitalWrite(ROT_BUTTON, HIGH);
  digitalWrite(ROT_A, HIGH);
  digitalWrite(ROT_B, HIGH);
  digitalWrite(TX_LED,HIGH);
  digitalWrite(RX_LED,HIGH);
  digitalWrite(RX_RAMV,HIGH);
  digitalWrite(RX_CPUV,HIGH);
//  Serial.println("Tft setup");
 if (digitalRead(BUTTON_BLUE)==LOW) {
    for (int count=0;count < MEM_SIZE;count++){memory[count]=0;} // skip mem clean for now
   }
   else
   Rom_Out();//else load stored data
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9488_BLACK);
 // delay(50);

  if (digitalRead(ROT_BUTTON)==0) 
  {
    //dump mem
    dump_lcd();
  }
//give chance to reset stored data
  
  if (memory[QCODE]>0) {Text_qcode();}

  tft.drawBitmap(ROG_LOGO_X, ROG_LOGO_Y, myRog, ROG_LOGO_X_SIZE,ROG_LOGO_Y_SIZE,ILI9488_RED);
  tft.drawBitmap(CPU_LOGO_X, CPU_LOGO_Y, myRyzen, CPU_LOGO_X_SIZE,CPU_LOGO_Y_SIZE,ILI9488_WHITE);
  tft.drawBitmap(CPU_LOGO_X+2, CPU_LOGO_Y+2, myRyzen, CPU_LOGO_X_SIZE,CPU_LOGO_Y_SIZE,ILI9488_RED);
   First_Run=1;//Just booted indicator 
}
/*
#define TX_LED A0
#define RX_LED A1
#define RX_RAMV A3
#define RX_CPUV A2*/
void Rom_In()
{
  //int addr = 0;
  //  memory[QCODE]=0;
  for (int addr =0;addr<MEM_SIZE;addr++){
   EEPROM.update(addr, memory[addr]);
   digitalWrite(RX_CPUV,!digitalRead(RX_CPUV));//indicator LED for I2C bus to ROG_EXT 
   //EEPROM.write(addr, 0); // this is only to blank eeprom , it's a test routine
//    if (addr == EEPROM.length()) {
if (addr == MEM_SIZE) {
  digitalWrite(RX_CPUV,HIGH);
    return;
  }
  }
  digitalWrite(RX_CPUV,HIGH);
}

void Rom_Out()
{
  //int addr = 0;
  
  for (int addr =0;addr<MEM_SIZE;addr++){
      digitalWrite(RX_RAMV,!digitalRead(RX_RAMV));//indicator LED for I2C bus to ROG_EXT
       memory[addr]= EEPROM.read(addr);
       if (addr == EEPROM.length()) {
        return;
  }
  }
  digitalWrite(RX_RAMV,HIGH);
}

int row_graph =1; 
int last_sample_cpu_graph =38;

int alt_count = 0;
int alt_count_two =0;
#define alt_counter_level  10

void non_block()
{
  if (alt_count>alt_counter_level)
  {
    graph();
   // tft.drawBitmap(0, 0, myRog, 146, 80,ILI9488_RED);
    Text_fan();
    Text_tvolt_one();
    Text_tvolt_two();
  //  display_byte();
   // display_byte_two();
  //  display_byte_three();
 //   display_byte_i2c();
    alt_count=0;
  }
  else
  {
    if (alt_count_two>20) {text_one();alt_count_two=0;}
    alt_count++;
    alt_count_two++;
    Text_ram();
    Text_cpu_volts();
    Text_cpu_clocks();
    if (memory[QCODE]>0) {Text_qcode();}
    Text_qcode();
  }
}

void loop() {
  //delay(2);//Delay A
  int counter = 0;
//  Serial.print("loop");
 // Serial.println(text_one());
  if (digitalRead(BUTTON_BLUE)==LOW) {
    Rom_In();
    //row_graph=460;
    }
    if (digitalRead(ROT_BUTTON)==LOW) {
    dump_lcd();
    //row_graph=460;
    }
  //counter=text_one();
//  graph();
non_block();
//else
 //{
  //if (digitalRead(BUTTON_BLUE)==HIGH) {working_sensor();}
  
}



void graph()
{
   unsigned long start, t;
  int           x1, y1, x2, y2,
               //w = tft.width(),
                //h = tft.height();
                h = ILI9488_TFTHEIGHT,
                w = ILI9488_TFTWIDTH;
  int cpu_temp = memory[CPU_TEMP];
  //tft.stroke(250,180,10);
 // if (row_graph==0) {}
 // else
 // if (row_graph>439) {}
 // else
  if (First_Run==0) 
    { 
      tft.drawLine(row_graph,(w-temp_array[row_graph])-10, row_graph, w, ILI9488_BLACK);
      //tft.drawPixel(row_graph,temp_array[row_graph],ILI9488_BLUE);
     //  tft.fillRect(row_graph,(w-temp_array[row_graph])-5, row_graph+5, (w-temp_array[row_graph])+5, ILI9488_BLACK);
  }
 
  if (cpu_temp<20) {return;}
  if (cpu_temp>130) {return;}
  if (memory[CPU_TEMP]>65) 
    tft.drawLine(row_graph, (w-last_sample_cpu_graph), row_graph, (w-cpu_temp)+1, ILI9488_RED);
  else
    if (cpu_temp>50)  
      tft.drawLine(row_graph, (w-last_sample_cpu_graph), row_graph, (w-cpu_temp)+1, ILI9488_YELLOW);
    else
      tft.drawLine(row_graph, (w-last_sample_cpu_graph), row_graph, (w-cpu_temp)+1, ILI9488_GREEN);
   
   //tft.line(xPos, TFTscreen.height() - graphHeight, xPos, TFTscreen.height());
  last_sample_cpu_graph=cpu_temp;
  temp_array[row_graph]=cpu_temp;
  row_graph++;   
   if (row_graph>ILI9488_TFTHEIGHT) {row_graph=1;First_Run=0;}
}
int last_char =0;

#define CPU_TEMP_X_TEXT 130
#define CPU_TEMP_Y_TEXT 165

void text_one ()
  {
   unsigned long start = micros();
   char cpu_mem =0;
  cpu_mem = memory[CPU_TEMP];
  if (last_char==cpu_mem) {return;}
    tft.setCursor(CPU_TEMP_Y_TEXT,CPU_TEMP_X_TEXT);
    //tft.setTextColor(ILI9488_RED);    tft.setTextSize(4);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(4);
    tft.print(last_char,DEC);
    tft.setCursor(CPU_TEMP_Y_TEXT, CPU_TEMP_X_TEXT);
    tft.setTextColor(ILI9488_YELLOW); tft.setTextSize(4);
    tft.print(cpu_mem,DEC);
    //dump_memory_lcd();
 //   return micros() - start;
 last_char=cpu_mem;
}

#define MONITOR_BYTE 0x10
#define MONITOR_BYTE_TWO 0x0
byte qcode_sample =0;
void Text_qcode ()
  {
  byte current_qcode = memory[QCODE];
    if (qcode_sample==current_qcode){return;}
    tft.setCursor(0, 0);
    //tft.setTextColor(ILI9488_RED);    tft.setTextSize(4);
    if (memory[MONITOR_BYTE]>0){tft.setTextColor(ILI9488_BLUE);}
    else
    if (memory[MONITOR_BYTE_TWO]>0){tft.setTextColor(ILI9488_RED);}
    else
    {
      tft.setTextColor(ILI9488_PINK);
    }
    tft.setTextSize(3);
    tft.print("QCODE ");
    tft.setCursor(0, 25);
    tft.setTextColor(ILI9488_BLACK);
    tft.print(qcode_sample);
    tft.setCursor(0, 25);
    tft.setTextColor(ILI9488_BLUE);
    tft.print(memory[QCODE],DEC);
   // tft.print(" ");tft.print(memory[XCODE],DEC);tft.print(memory[XCODE+1],DEC);
  qcode_sample=current_qcode;
}
#define FAN_ONE_MEM_LOW 0x60
#define FAN_ONE_MEM_HIGH 0x61
int fan_sample =0;
void Text_fan ()
  {
    int fan_speed=(memory[FAN_ONE_MEM_LOW] << 8 | memory[FAN_ONE_MEM_HIGH]);
    if (fan_sample==fan_speed) {return;}
  //Serial.print((memory[0x60] << 8 | memory[0x61]));
  //Serial.print("RPM");
  tft.setCursor(200, 0);
    //tft.setTextColor(ILI9488_RED);    tft.setTextSize(4);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(3);
    tft.print(fan_sample,DEC);

    tft.setTextColor(ILI9488_MAGENTA); tft.setTextSize(3);
    tft.setCursor(130, 0);
    tft.print("FAN");
    tft.setCursor(200, 0);
    //tft.setTextColor(ILI9488_RED);    tft.setTextSize(4);
    tft.setTextColor(ILI9488_GREEN); tft.setTextSize(3);
    tft.print(fan_speed,DEC);
    
    fan_sample=fan_speed;
}

int swap16 (int val)
{
  return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

#define CPU_VOLT_LOW 0x40
#define CPU_VOLT_HIGH 0x41
/*
 *   char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[start+i]); 
         tft.print(tmp); tft.print(" ");
         //tft.print(data[start+i]);tft.print("");
       }
 */
int tovolts (int value )
{
  return swap16(value) * VDIV2;
}
float cpu_volt_sample =0;
void Text_cpu_volts ()
  {
  //  float cpu_volts = 0;
    char volts_string[20];
    char tmp[80];
  
   float cpu_volts=((memory[CPU_VOLT_LOW] * VDIV1) + (memory[CPU_VOLT_HIGH] * VDIV2));
   //float cpu_volts=tovolts(memory[CPU_VOLT_LOW]);
   
 //  float cpu_volts=memory[CPU_VOLT_LOW];
 //  cpu_volts=(cpu_volts<<8)|memory[CPU_VOLT_HIGH];
  if (cpu_volt_sample ==cpu_volts) {return;}
    dtostrf(cpu_volt_sample, 2, 3, volts_string);
    tft.setCursor(200, 60);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(3);
    tft.print(volts_string);

    tft.setTextColor(ILI9488_MAROON); tft.setTextSize(3);
    tft.setCursor(130, 60);
    tft.print("CPU");
    tft.setCursor(200, 60);

    tft.setTextColor(ILI9488_GREEN); tft.setTextSize(3);
    dtostrf(cpu_volts, 3, 3, volts_string);
    tft.print(volts_string);
  //  digitalWrite(RX_CPUV,!digitalRead(RX_CPUV));
    cpu_volt_sample =cpu_volts;
}
#define RAM_VOLT_LOW 0x48
#define RAM_VOLT_HIGH 0x49
float ram_sample =0;
void Text_ram ()
  {
     char rams[20],ramss[20];
     
    //float ram_volts=(memory[RAM_VOLT_LOW] * VDIV1) + (memory[RAM_VOLT_HIGH] * VDIV2);
    float ram_volts=(memory[RAM_VOLT_LOW] * VDIV1) + (memory[RAM_VOLT_HIGH] * VDIV2);
    //ram_volts=1.2;
    if (ram_sample==ram_volts) {return;}
    dtostrf(ram_sample, 2, 3, ramss);
    tft.setCursor(200, 30);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(3);
    tft.print(ramss);
    dtostrf(ram_volts, 2, 3, rams);
    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(3);
    tft.setCursor(130, 30);
    tft.print("RAM");
    tft.setCursor(200, 30);

    tft.setTextColor(ILI9488_GREEN); tft.setTextSize(3);
   // tft.print(ram_volts);
    tft.print(rams);
   // tft.print(memory[0x48],DEC);tft.print(" ");
   // tft.print(memory[0x49],DEC);
  //  digitalWrite(RX_RAMV,!digitalRead(RX_RAMV));
    ram_sample=ram_volts;
}

#define BOX_TWO_X 380
#define BOX_TWO_SPACE 90
#define TVOLT_VOLT_LOW 0x30
#define TVOLT_VOLT_HIGH 0x31
float tvolt_sample =0;
void Text_tvolt_one ()
  {
     char tvolt_char[20];
     
    
    float tvolt=(memory[TVOLT_VOLT_LOW] * VDIV1) + (memory[TVOLT_VOLT_HIGH] * VDIV2);
    //ram_volts=1.2;
    if (tvolt_sample==tvolt) {return;}
    dtostrf(tvolt_sample, 2, 3, tvolt_char);
    tft.setCursor(BOX_TWO_X, 100);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.print(tvolt_char);
    dtostrf(tvolt, 2, 3, tvolt_char);
    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 100);
    tft.print("VDDSOC");
    tft.setCursor(BOX_TWO_X, 100);

    tft.setTextColor(ILI9488_PINK); tft.setTextSize(2);
   
    tft.print(tvolt_char);
    //digitalWrite(RX_TVOLTV,!digitalRead(RX_TVOLTV));
    tvolt_sample=tvolt;
}

#define TVOLT_TWO_VOLT_LOW 0x38
#define TVOLT_TWO_VOLT_HIGH 0x39
float tvolt_sample_two =0;
void Text_tvolt_two ()
  {
     char tvolt_char[20];
     
    
    float tvolt=(memory[TVOLT_TWO_VOLT_LOW] * VDIV1) + (memory[TVOLT_TWO_VOLT_HIGH] * VDIV2);
    //ram_volts=1.2;
    if (tvolt_sample_two==tvolt) {return;}
    dtostrf(tvolt_sample_two, 2, 3, tvolt_char);
    tft.setCursor(BOX_TWO_X, 120);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.print(tvolt_char);
    dtostrf(tvolt, 2, 3, tvolt_char);
    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 120);
    tft.print("1.8VPLL");
    tft.setCursor(BOX_TWO_X, 120);

    tft.setTextColor(ILI9488_ORANGE); tft.setTextSize(2);
   
    tft.print(tvolt_char);
    //digitalWrite(RX_TVOLTV,!digitalRead(RX_TVOLTV));
    tvolt_sample_two=tvolt;
}

#define BYTE_ONE 0x40

byte byte_one_sample =0;

void display_byte ()
  {
  byte byte_one = memory[BYTE_ONE];

  if (byte_one_sample==byte_one) {return;}
    tft.setCursor(BOX_TWO_X, 140);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.print(byte_one_sample);

    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 140);
    tft.print("BYTE ");
    tft.print(BYTE_ONE,HEX);
    tft.setCursor(BOX_TWO_X, 140);
    
    tft.setTextColor(ILI9488_RED); tft.setTextSize(2);
   
    tft.print(byte_one,DEC);
  byte_one_sample=memory[BYTE_ONE];

}

#define BYTE_TWO 0x41

byte byte_two_sample =0;

void display_byte_two ()
  {
  byte byte_two = memory[BYTE_TWO];

  if (byte_two_sample==byte_two) {return;}
    tft.setCursor(BOX_TWO_X, 160);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.print(byte_two_sample);

    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 160);
    tft.print("BYTE ");
    tft.print(BYTE_TWO,HEX);
    tft.setCursor(BOX_TWO_X, 160);
    
    tft.setTextColor(ILI9488_RED); tft.setTextSize(2);
   
    tft.print(byte_two,DEC);
  byte_two_sample=memory[BYTE_TWO];

}

#define BYTE_THREE 0x51

byte byte_three_sample =0;

void display_byte_three ()
  {
  word byte_three = memory[BYTE_THREE];

  if (byte_three_sample==byte_three) {return;}
    tft.setCursor(BOX_TWO_X, 180);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.print(byte_three_sample);

    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 180);
    tft.print("BYTE ");
    tft.print(BYTE_THREE,HEX);
    tft.setCursor(BOX_TWO_X, 180);
    
    tft.setTextColor(ILI9488_RED); tft.setTextSize(2);
     
    tft.print(byte_three);
   byte_three_sample=memory[BYTE_THREE];

}

/*
 * 
#define TX_LED A0
#define RX_LED A1
#define RX_RAMV A3
#define RX_CPUV A2


 */
int mem_req[6]={0,0,0,0,0,0};

void read_mem()
{
  int myvars[6]={0,0,0,0,0,0};
  int bytes=0;
  while(Wire.available())    // slave may send less than requested
    { 
      //so far not triggered but here in case the ROC_EXT asks for data, 
     digitalWrite(RX_LED,!digitalRead(RX_LED));//indicator LED for I2C bus to ROG_EXT
    // myvars[bytes] = Wire.read();
      bytes++;
    }
  //  Wire.write(memory[myvars[0]]);
   // for (int cr=0;cr<6;cr++){  
   // mem_req[cr]=myvars[cr];
   
   //   }
    mem_req[6]=bytes;
    //if called but no data request then trigger this led
    digitalWrite(RX_CPUV,!digitalRead(RX_CPUV));//indicator LED for I2C bus to ROG_EXT
}

#define led_target  5
int led_value_one =0;

void led_delayed()
{
  if (led_value_one>led_target)
  {
    led_value_one=0;
    digitalWrite(TX_LED,!digitalRead(TX_LED));//indicator LED for I2C bus to ROG_EXT
  }
  else
  led_value_one++;
}

byte last_bytes = 0;
int data_i2c[6]={0,0,0,0,0,0};

void receiveEvent(int howMany) {
  
  int bytes=0;
  byte myvars[6]={0,0,0,0,0,0};

  int ROG_addr = 0;
  int ROG_value =0;

  while(Wire.available())    // slave may send less than requested
    { 
      myvars[bytes] = Wire.read();
      //digitalWrite(TX_LED,!digitalRead(TX_LED));//indicator LED for I2C bus to ROG_EXT
      led_delayed();
    //  digitalWrite(RX_LED,!digitalRead(RX_LED));//indicator LED for I2C bus to ROG_EXT
      bytes++;
      last_bytes=bytes;
        }
      memory[myvars[0]]=myvars[1];
   for (int cr=0;cr<6;cr++){  
    data_i2c[cr]=myvars[cr];
         }
        
    }
 


#define i2c_chain 0
byte i2c_sample =0;

void display_byte_i2c ()
  {
  byte sampled_i2c = data_i2c[i2c_chain];
  //this part ignores common mem values so you can monitor the uncommon memory writes easier, common being cpu temp, fan speed, volts for ram/cpu 
  switch (sampled_i2c){
       case 0x60:
           return;
           break;
       case 0x61:
           return;
           break;
       case 0x50:
           return;
           break;
       case 0x51:
           // return;
            break;
       default:
          break;
    }
    
  if (i2c_sample==sampled_i2c) {return;}
    tft.setCursor(BOX_TWO_X, 200);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(2);
    tft.fillRect(BOX_TWO_X-BOX_TWO_SPACE,200,120,30,ILI9488_BLACK);
    //tft.print(i2c_sample);

    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(2);
    tft.setCursor(BOX_TWO_X-BOX_TWO_SPACE, 200);
    tft.print("I2C ");
    tft.print(last_bytes,HEX);
    tft.setCursor(BOX_TWO_X, 200);
    last_bytes=0;
    
    tft.setTextColor(ILI9488_RED); tft.setTextSize(2);
   
    tft.print(sampled_i2c,HEX);
    //tft.print(" ");
   // if (i2c_chain<6) {tft.print(data_i2c[i2c_chain+1],HEX);}
   // else tft.print(i2c_chain,HEX);
  i2c_sample=sampled_i2c;

}


 // int CPU_SPEED =0;
 // Serial.print((memory[0x20]*((memory[0x28] * CDIV1) + (memory[0x29] * CDIV2))));
#define CPU_CLOCK 0x20
#define CPU_CLOCK_LOW 0x28
#define CPU_CLOCK_HIGH 0x29
int cpu_CLOCK_sample =0;
void Text_cpu_clocks ()
  {
    int cpu_RATIO =((memory[CPU_CLOCK_LOW] * CDIV1) + (memory[CPU_CLOCK_HIGH] * CDIV2));
    int cpu_CLOCK=(memory[CPU_CLOCK]*cpu_RATIO);
    if (cpu_CLOCK_sample ==cpu_CLOCK) {return;}
    tft.setCursor(200, 90);
    tft.setTextColor(ILI9488_BLACK); tft.setTextSize(3);
    tft.fillRect(200,90,120,30,ILI9488_BLACK);
    //tft.print(cpu_CLOCK,DEC);

    tft.setTextColor(ILI9488_BLUE); tft.setTextSize(3);
    tft.setCursor(130, 90);
    tft.print("CLK");
    
    tft.setCursor(200, 90);
    tft.setTextColor(ILI9488_GREEN); tft.setTextSize(3);
    tft.print(cpu_CLOCK,DEC);
    
    cpu_CLOCK_sample =cpu_CLOCK;
}

void dump_lcd ()
{
  int alt_bit =1;

  while (1==1) 

  {
    if (alt_bit==1) {
        tft.setTextColor(ILI9488_WHITE); tft.setTextSize(2);
    tft.setCursor(0,0);
   // tft.println("0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F");
 tft.print("0");//that one dodgy zero, will fix later but this dirty fix i hope *NB* dirty fix works so it's staying

        for (int row=0;row<0x10;row++)
        {
          tft.print(row*0x10,HEX);
          tft.print(":");
          tft.println(PrintHex8_lcd(memory,row*0x10,0x10));
          //tft.println("");
        }
    }
    while (ROT_BUTTON==0) {delay(20);}
  if (First_Run==1) {return;}
  }
}
char PrintHex8_lcd(uint8_t *data,uint8_t start, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
     char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[start+i]); 
         tft.print(tmp); tft.print(":");
         //tft.print(data[start+i]);tft.print("");
       }
}




/*
 * 
 * Main Loop, do all you math and displays in this area, try not to adjust main i2c routine as that just updates memory array
 * 
 * 
 
 */
char PrintHex8(uint8_t *data,uint8_t start, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
     char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "0x%.2X",data[start+i]); 
         Serial.print(tmp); Serial.print(" ");
       }
}

void PrintHex16(uint16_t *data, uint8_t length) // prints 16-bit data in hex with leading zeroes
{
       char tmp[16];
       for (int i=0; i<length; i++)
       { 
         sprintf(tmp, "0x%.4X",data[i]); 
         Serial.print(tmp); Serial.print(" ");
       }
}

void mem_comp()
{
//You should run this routine one and forget the first run, try varying your delays A and B  
for (int compare=0;compare<MEM_SIZE;compare++) 
  {
    if (alt_memory[compare]==memory[compare])
    {}
    else
    {
      Serial.print("0x");
      Serial.print(compare,HEX);
      Serial.print(" 0x");
      Serial.print(memory[compare],HEX);
      Serial.print(" 0x");
      Serial.print(alt_memory[compare],HEX);
      Serial.println("   --");
    }
  }
//  delay(100); //Delay B
  for (int copy=0;copy <MEM_SIZE;copy++){alt_memory[copy]=memory[copy];}
}
