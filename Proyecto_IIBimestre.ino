#include <Wire.h>
#include <stdio.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Time.h>
#include <WiFi.h>
#include "time.h"
#include <ESP32Servo.h>
#include <HX711.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

//Firebase
/* 2. Define the API Key */
#define API_KEY "AIzaSyBZ9pbqF8mmrNDihxQpCCEppd7veViesjg"
/* 3. Define the RTDB URL */
#define DATABASE_URL "proyectoembebidosbasededatos-default-rtdb.firebaseio.com/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "proyectoesp32embebidos@gmail.com"
#define USER_PASSWORD "proyecto_1234"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config1;

//SERVOS
Servo servo1;
Servo servo2;
Servo servoG;
//BALANZAS
HX711 balanza1;
HX711 balanza2;

//Reloj
ESP32Time rtc;
//LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);
//Timer
hw_timer_t* timer = NULL;


//CONSTANTES - PINES
//HC-SCR04 (1)
const int echoPin1 = 12;
const int triggerPin1 = 14;
//HC-SCR04 (2)
const int echoPin2 = 27;
const int triggerPin2 = 26;
//SERVO (1)
const int servo1Pin = 19;  //25;
//SERVO (2)
const int servo2Pin = 18;  //33;
//SERVO (COM)
const int servoGPin = 33;  //32;
//BOMBA
const int bombaPin = 13;
//HMI LOCAL
const int enter = 17;
const int config = 36;
const int calib = 5;
//GALGAS
/*const int DT2 = 2;
const int CLK2 = 15;
const int DT1 = 4;
const int CLK1 = 0;*/
const int DT2 = 4;
const int CLK2 = 0;
const int DT1 = 15;
const int CLK1 = 2;



//Credenciales WiFi
//const char* ssid ="Claro_BARRENO"; //nombre red
//const char* password ="+e1ol1Ru8b@8";//contraseña red
//const char* ssid = "Club_Robotica";      //nombre red
//const char* password = "R0b0t1ca_1921";  //contraseña
//const char* ssid ="James"; //nombre red
//const char* password ="James888";//contraseña
//const char* ssid ="CAMPUS_EPN"; //nombre red
//const char* password ="politecnica**";//contraseña
//const char* ssid = "TvCable_Tipan-2G_EXT";      //nombre red
//const char* password = "Espoli2023";  //contraseña
const char* ssid = "Diego";      //nombre red
const char* password = "Diego2001";  //contraseña
//Servidor NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 0;


//VARIABLES
//Valores potenciometro, configuraciones y modo
int hora, minu, seg, selec = 0, i = 0;
int h1[3], h2[3], h3[3], h4[3], hp[3], ha[3];
unsigned long tiempo;
//US (1)
long distancia1 = 0;
long nivmax1 = 34;
int nivel1 = 0;
//US (2)
float distancia2 = 0.0;
float nivmax2 = 31.0;
float nivel2 = 0.0;
float suma_nivel2=0.0;
float nivel2_promedio=0.0;
float nivel2_anterior=0;
float lim_nivel2=3.5;
const int muestras=30;
//Cantidad Gr
int cant = 0;
int cant_tempo = 0;
//En que horario va
int numHora = 0;  //1 horario 1, 2 horario2......

//Calib Galgas
float peso_calibracion = 453.0;
float escala1 = 420;//236.88;
long offset1 = 81270;
long escala2 = 230; //-440.5;
long offset2 = 63500;
float adc1, adc2;
float peso1, peso2;
int auxservo=0;

int state_zero = 0;
int last_state_zero = 0;

volatile int activar = 0, activar1 = 0, enviar_firebase = 0;
int n = 0;

int aux = 0;
//Variabe Firebase
String hora1_firebase;
String minutos1_firebase;
String hora2_firebase;
String minutos2_firebase;
String hora3_firebase;
String minutos3_firebase;
String hora4_firebase;
String minutos4_firebase;
String porcion_firebase;
String Tagua;
String Tcomida;
String Aagua;
String Acomida;

String ESP_data="0";
volatile int aux_act = 0;

char caracterEliminar1 = '"';
char caracterEliminar2 = '\\';

//FUNCIONES
void configPuertosUS();
long readUSDistance1();
float readUSDistance2();
void IRAM_ATTR isr_enter();
void IRAM_ATTR isr_calib();
void IRAM_ATTR TimerBomba();
void conectar_firebase(void);
void eliminarCaracter(String& cadena, char caracterEliminar);
void Firebase_function(void);
float promedio_nivel2(void);
//void calibrar();

void setup() {
  //Iniciar LCD
  //Serial.begin(115200);
  lcd.begin();
  lcd.backlight();

  //Conexion WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println("CONECTADO");

  //Sincronizar el reloj interno del esp con el de internet
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }

  conectar_firebase();
  //Pines
  configPuertosUS();
  pinMode(enter, INPUT_PULLUP);
  pinMode(calib, INPUT_PULLUP);
  pinMode(config, INPUT);
  pinMode(bombaPin, OUTPUT);
  //digitalWrite(bombaPin, HIGH);

  //Interrupciones
  attachInterrupt(digitalPinToInterrupt(enter), isr_enter, FALLING);
  attachInterrupt(digitalPinToInterrupt(calib), isr_calib, FALLING);

  //SERVO
  servo1.attach(servo1Pin, 500, 2500);
  servo2.attach(servo2Pin, 500, 2500);
  servoG.attach(servoGPin, 500, 2500);

  servo1.write(0);
  //servo2.write(45);
  servoG.write(20);

  //BALANZAS
  balanza1.begin(DT1, CLK1);  //asigna los pines para recibir la trama de pulsos que proviene del modulo
  balanza2.begin(DT2, CLK2);  //asigna los pines para recibir la trama de pulsos que proviene del modulo

  //pinMode(calib,INPUT_PULLUP);
  //Serial.begin(115200);
  //EEPROM.read(0);

  delay(100);
  balanza1.set_scale(escala1);  // Establecemos la escala
  balanza1.set_offset(offset1);
  //balanza1.tare(20);            // El peso actual de la base se considera cero
  balanza2.set_scale(escala2);  // Establecemos la escala
  balanza2.set_offset(offset2);
  //balanza2.tare(20);            // El peso actual de la base se considera cero

  //Interrupcion Sensores
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &TimerBomba, true);
  timerAlarmWrite(timer, 5000000, true);
  timerAlarmEnable(timer);
}

void loop() {

  if (selec == 0) {
    lcd.setCursor(0, 0);
    lcd.print(rtc.getTime("%H:%M:%S  %d/%m/%Y"));

    lcd.setCursor(0, 1);
    lcd.print("Proxima alimentacion"); 
    lcd.setCursor(0, 2);
    lcd.print(hp[2]);
    lcd.print(":");
    lcd.print(hp[1]);
    lcd.print(":");
    lcd.print(hp[0]);
    lcd.print("  ");
    lcd.print(cant);
    lcd.print("gr ");

    lcd.setCursor(0, 3);
    lcd.print("TA:");
    lcd.print(nivel1);
    lcd.print("cm ");
    lcd.print("TC:");
    lcd.print(peso1);
    lcd.print("gr ");
  }

  if (selec == 1) {
    lcd.setCursor(0, 3);
    lcd.print("                     ");
    lcd.setCursor(0, 0);
    lcd.print(" Configuracion de 4 ");
    lcd.setCursor(0, 1);
    lcd.print("      horarios      ");
    lcd.setCursor(0, 2);
    lcd.print("                    ");
  }

  if (selec == 2) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 1 (HORA)    ");
    hora = analogRead(config);
    hora = 23 * hora / 4095;

    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h1[2]);
    lcd.print(":");
    lcd.print(h1[1]);
    lcd.print(":");
    lcd.print(h1[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(hora);
    lcd.print(":");
    lcd.print(h1[1]);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 3) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 1 (MINUTOS)  ");
    minu = analogRead(config);
    minu = 59 * minu / 4095;
    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h1[2]);
    lcd.print(":");
    lcd.print(h1[1]);
    lcd.print(":");
    lcd.print(h1[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(h1[2]);
    lcd.print(":");
    lcd.print(minu);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 4) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 2 (HORA)   ");
    hora = analogRead(config);
    hora = 23 * hora / 4095;

    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h2[2]);
    lcd.print(":");
    lcd.print(h2[1]);
    lcd.print(":");
    lcd.print(h2[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(hora);
    lcd.print(":");
    lcd.print(h2[1]);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 5) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 2 (MINUTOS)  ");
    minu = analogRead(config);
    minu = 59 * minu / 4095;
    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h2[2]);
    lcd.print(":");
    lcd.print(h2[1]);
    lcd.print(":");
    lcd.print(h2[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(h2[2]);
    lcd.print(":");
    lcd.print(minu);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  //Horario 3
  if (selec == 6) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 3 (HORA)  ");
    hora = analogRead(config);
    hora = 23 * hora / 4095;

    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h3[2]);
    lcd.print(":");
    lcd.print(h3[1]);
    lcd.print(":");
    lcd.print(h3[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(hora);
    lcd.print(":");
    lcd.print(h3[1]);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 7) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 3 (MINUTOS)   ");
    minu = analogRead(config);
    minu = 59 * minu / 4095;
    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h3[2]);
    lcd.print(":");
    lcd.print(h3[1]);
    lcd.print(":");
    lcd.print(h3[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(h3[2]);
    lcd.print(":");
    lcd.print(minu);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  //Horario 4
  if (selec == 8) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 4 (HORA)  ");
    hora = analogRead(config);
    hora = 23 * hora / 4095;

    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h4[2]);
    lcd.print(":");
    lcd.print(h4[1]);
    lcd.print(":");
    lcd.print(h4[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(hora);
    lcd.print(":");
    lcd.print(h4[1]);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 9) {
    lcd.setCursor(0, 0);
    lcd.print("HORARIO 4 (MINUTOS) ");
    minu = analogRead(config);
    minu = 59 * minu / 4095;
    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(h4[2]);
    lcd.print(":");
    lcd.print(h4[1]);
    lcd.print(":");
    lcd.print(h4[0]);
    lcd.print("    ");

    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(h4[2]);
    lcd.print(":");
    lcd.print(minu);
    lcd.print(":");
    lcd.print(0);
    lcd.print("  ");
  }

  if (selec == 10) {
    lcd.setCursor(0, 0);
    lcd.print("INGRESAR COMIDA     ");
    lcd.setCursor(0, 1);
    lcd.print("Actual: ");
    lcd.print(cant);
    lcd.print(" gr  ");
    cant_tempo = analogRead(config);
    cant_tempo = 250 * cant_tempo / 4095;
    lcd.setCursor(0, 2);
    lcd.print("Cambiar: ");
    lcd.print(cant_tempo);
    lcd.print(" gr  ");
  }

  if (selec == 11) {
    lcd.setCursor(0, 0);
    lcd.print("Tagua: ");
    lcd.print(nivel1);
    lcd.print(" cm  ");
    lcd.setCursor(0, 1);
    lcd.print("Ragua: ");
    lcd.print(nivel2);
    lcd.print(" cm  ");
    lcd.setCursor(0, 2);
    lcd.print("Tcomida: ");
    lcd.print(peso1);
    lcd.print(" gr   ");
    lcd.setCursor(0, 3);
    lcd.print("Rcomida: ");
    lcd.print(peso2);
    lcd.print(" gr   ");
  }


  if(aux_act){
    //HORARIOS
    hora1_firebase=String(h1[2]);
    hora2_firebase=String(h2[2]);
    hora3_firebase=String(h3[2]);
    hora4_firebase=String(h4[2]);

    Firebase.setString(fbdo, F("/App_Proyecto/Hor1"), hora1_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Hor2"), hora2_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Hor3"), hora3_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Hor4"), hora4_firebase);

    minutos1_firebase=String(h1[1]);
    minutos2_firebase=String(h2[1]);
    minutos3_firebase=String(h3[1]);
    minutos4_firebase=String(h4[1]);

    Firebase.setString(fbdo, F("/App_Proyecto/Mim1"), minutos1_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Mim2"), minutos2_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Mim3"), minutos3_firebase);
    Firebase.setString(fbdo, F("/App_Proyecto/Mim4"), minutos4_firebase);

    aux_act = 0;
  }
/*
  //HORARIOS
  hora1_firebase=String(h1[2]);
  hora2_firebase=String(h2[2]);
  hora3_firebase=String(h3[2]);
  hora4_firebase=String(h4[2]);

  Firebase.setString(fbdo, F("/App_Proyecto/Hor1"), hora1_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Hor2"), hora2_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Hor3"), hora3_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Hor4"), hora4_firebase);

  minutos1_firebase=String(h1[1]);
  minutos2_firebase=String(h2[1]);
  minutos3_firebase=String(h3[1]);
  minutos4_firebase=String(h4[1]);

  Firebase.setString(fbdo, F("/App_Proyecto/Mim1"), minutos1_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Mim2"), minutos2_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Mim3"), minutos3_firebase);
  Firebase.setString(fbdo, F("/App_Proyecto/Mim4"), minutos4_firebase);

  ESP_data="1";
  Firebase.setString(fbdo, F("/App_Proyecto/ESP_data"), ESP_data);
*/

////////////////////////////////////////////
  if ((activar == 0)&&(activar1 == 0)){
    ha[2] = rtc.getHour(true);
    ha[1] = rtc.getMinute();
    ha[0] = rtc.getSecond();
    int dm1 = ((h1[2]-ha[2])*60)+(h1[1]-ha[1]);
    if (dm1<0)  dm1=1440+dm1;
    int dm2 = ((h2[2]-ha[2])*60)+(h2[1]-ha[1]);
    if (dm2<0)  dm2=1440+dm2;
    int dm3 = ((h3[2]-ha[2])*60)+(h3[1]-ha[1]);
    if (dm3<0)  dm3=1440+dm3;
    int dm4 = ((h4[2]-ha[2])*60)+(h4[1]-ha[1]);
    if (dm4<0)  dm4=1440+dm4;
    /*
    Serial.print("1:");
    Serial.println(dm1);
    Serial.print("2:");
    Serial.println(dm2);
    Serial.print("3:");
    Serial.println(dm3);
    Serial.print("4:");
    Serial.println(dm4);*/

    hp[0] = 0;
    if((dm1<=dm2)&&(dm1<=dm3)&&(dm1<=dm4)){
      hp[2] = h1[2];
      hp[1] = h1[1];
    }
    if((dm2<dm1)&&(dm2<dm3)&&(dm2<dm4)){
      hp[2] = h2[2];
      hp[1] = h2[1];
    }
    if((dm3<dm1)&&(dm3<dm2)&&(dm3<dm4)){
      hp[2] = h3[2];
      hp[1] = h3[1];
    }
    if((dm4<dm1)&&(dm4<dm2)&&(dm4<dm3)){
      hp[2] = h4[2];
      hp[1] = h4[1];
    }
    
    if (h1[2] == ha[2] && h1[1] == ha[1] && (ha[0]<10)) {  //MENU ESPERA
      activar = 1;
      activar1 = 1;
    }

    if (h2[2] == ha[2] && h2[1] == ha[1] && (ha[0]<10)) {
      activar = 1;
      activar1 = 1;
    }
    if (h3[2] == ha[2] && h3[1] == ha[1] && (ha[0]<10)) {
      activar = 1;
      activar1 = 1;
    }
    if (h4[2] == ha[2] && h4[1] == ha[1] && (ha[0]<10)) {
      activar = 1;
      activar1 = 1;
    }
    auxservo = map(cant,50,200,20,50);

  }

///////////////////////////////////////////

if(activar || activar1){
    selec = 100;  //ESPERA    
}

///////////////////////////////////////////

  distancia1 = (0.0343 / 2) * readUSDistance1();  //Calculamos la distancia en cm
  if (distancia1 > nivmax1) {                     //limitar distancia a 50 cm
    distancia1 = nivmax1;
  }
  nivel1 = nivmax1 - distancia1;

  nivel2=promedio_nivel2();
  //Serial.println(nivel2);

/////////AGUA//////////////
  if (activar1) {
    //DAR AGUA
    if (nivel1 > 3) {
      if (nivel2 < 4) {
        digitalWrite(bombaPin, HIGH);  //Activar
      } else if (nivel2>nivel2_anterior){
        digitalWrite(bombaPin, LOW);  //Apagar
        activar1 = 0;
      }
    } else {
      digitalWrite(bombaPin, LOW);  //Apagar
      activar1 = 0;
    }
  }
  nivel2_anterior = nivel2;
  /* else {
    digitalWrite(bombaPin, LOW);  //Apagar
  }*/

////////////////////////////////////////////////////

  //PESO GALGA 1
  peso1 = balanza1.get_units();
  if(peso1<0){
    peso1=0;
  }
  //PESO GALGA 2
  peso2 = balanza2.get_units();
  if(peso2<0){
    peso2=0;
  }
///////////////COMIDA//////////////////////
  if (activar) {
    //DAR COMIDA
    if (peso1 > 20) {
      if (peso2 < cant-auxservo) {
        //Servo Compuerta
        servoG.write(0);
        //Servos Vibradores
        servo1.write(35);
        //servo2.write(0);
        delay(60);
        servo1.write(0);
        //servo2.write(45);
        delay(100);

      } else {
        servo1.write(0);
        //servo2.write(45);
        servoG.write(20);
        activar = 0;
      }
    } else {
      servo1.write(0);
      //servo2.write(45);
      servoG.write(20);
      activar = 0;
    }
  }/*
  else {
    servo1.write(0);
    servo2.write(70);
    servoG.write(35);  //Apagar
  }*/

  if ((activar == 0)&&(activar1 == 0)&&(selec == 100)){
    selec = 0;
  }

  if ((activar == 0)&&(activar1 == 0)) {
    //Firebase_function();        //RECIBIR DATOS
    if (enviar_firebase == 1) {
      Firebase_function();        //RECIBIR DATOS
      Tagua = String(nivel1);
      Tcomida = String(peso1);
      Aagua = String(nivel2);
      Acomida = String(peso2);
      Firebase.setString(fbdo, F("/App_Proyecto/Tagua"), Tagua);
      Firebase.setString(fbdo, F("/App_Proyecto/Tcomida"), Tcomida);
      //Firebase.setString(fbdo, F("/App_Proyecto/Aagua"), Aagua);
      //Firebase.setString(fbdo, F("/App_Proyecto/Acomida"), Acomida);
      enviar_firebase = 0;
    }
  }
}

//Funciones de interrupción
void IRAM_ATTR isr_enter() {
  if (millis() > tiempo + 200) {
    selec++;
    tiempo = millis();
  }
  if (selec > 11) {
    selec = 0;
  }
}

void IRAM_ATTR isr_calib() {
  //INGRESAR VALORES
  aux_act = 1;
  switch (selec) {
    case 2:
      h1[2] = hora;
    break;
    case 3:
      h1[1] = minu;
    break;
    case 4:
      h2[2] = hora;
    break;
    case 5:
      h2[1] = minu;
    break;
    case 6:
      h3[2] = hora;
    break;
    case 7:
      h3[1] = minu;
    break;
    case 8:
      h4[2] = hora;
    break;
    case 9:
      h4[1] = minu;
    break; 
    case 10:
      cant = cant_tempo;
    break;    
    default:
    break;
  }
}

void IRAM_ATTR TimerBomba() {
  enviar_firebase = 1;
}

void configPuertosUS(void) {
  //HC-SR04 (1)
  pinMode(triggerPin1, OUTPUT);    //Trigger salida
  digitalWrite(triggerPin1, LOW);  //Inicializamos apagando
  pinMode(echoPin1, INPUT);        //Echo entrada
  delay(100);
  //HC-SR04 (2)
  pinMode(triggerPin2, OUTPUT);    //Trigger salida
  digitalWrite(triggerPin2, LOW);  //Inicializamos apagando
  pinMode(echoPin2, INPUT);        //Echo entrada
}

long readUSDistance1() {
  digitalWrite(triggerPin1, LOW);   //Apagamos el emisor de sonido
  delayMicroseconds(2);             //Retrasamos 2ms la emision de sonido
  digitalWrite(triggerPin1, HIGH);  //Emitir sonido
  delayMicroseconds(10);            //Pulso 10 us para activar
  digitalWrite(triggerPin1, LOW);   //Apagamos el emisor de sonido
  return pulseIn(echoPin1, HIGH);   // Calculamos el tiempo que tardo en regresar el sonido
}

float readUSDistance2() {
  digitalWrite(triggerPin2, LOW);   //Apagamos el emisor de sonido
  delayMicroseconds(2);             //Retrasamos 2ms la emision de sonido
  digitalWrite(triggerPin2, HIGH);  //Emitir sonido
  delayMicroseconds(10);            //Pulso 10 us para activar
  digitalWrite(triggerPin2, LOW);   //Apagamos el emisor de sonido
  return pulseIn(echoPin2, HIGH);   // Calculamos el tiempo que tardo en regresar el sonido
}
void conectar_firebase(void) {
  config1.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config1.database_url = DATABASE_URL;
  //config1.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config1, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
}

void eliminarCaracter(String& cadena, char caracterEliminar) {
  int len = cadena.length();
  int idx = 0;

  for (int i = 0; i < len; i++) {
    if (cadena[i] != caracterEliminar) {
      cadena[idx] = cadena[i];
      idx++;
    }
  }
  cadena.remove(idx);
}

void Firebase_function(void) {
  Firebase.getString(fbdo, F("/App_Proyecto/NewData"));
  if (fbdo.stringData() == "1") {
    Firebase.getString(fbdo, F("/App_Proyecto/Hor1"));
    hora1_firebase = fbdo.stringData();
    h1[2] = hora1_firebase.toInt();
    Firebase.getString(fbdo, F("/App_Proyecto/Mim1"));
    minutos1_firebase = fbdo.stringData();
    h1[1] = minutos1_firebase.toInt();

    Firebase.getString(fbdo, F("/App_Proyecto/Hor2"));
    hora2_firebase = fbdo.stringData();
    h2[2] = hora2_firebase.toInt();
    Firebase.getString(fbdo, F("/App_Proyecto/Mim2"));
    minutos2_firebase = fbdo.stringData();
    h2[1] = minutos2_firebase.toInt();

    Firebase.getString(fbdo, F("/App_Proyecto/Hor3"));
    hora3_firebase = fbdo.stringData();
    h3[2] = hora3_firebase.toInt();
    Firebase.getString(fbdo, F("/App_Proyecto/Mim3"));
    minutos3_firebase = fbdo.stringData();
    h3[1] = minutos3_firebase.toInt();

    Firebase.getString(fbdo, F("/App_Proyecto/Hor4"));
    hora4_firebase = fbdo.stringData();
    h4[2] = hora4_firebase.toInt();
    Firebase.getString(fbdo, F("/App_Proyecto/Mim4"));
    minutos4_firebase = fbdo.stringData();
    h4[1] = minutos4_firebase.toInt();

    Firebase.getString(fbdo, F("/App_Proyecto/porcion"));
    porcion_firebase = fbdo.stringData();
    eliminarCaracter(porcion_firebase, caracterEliminar1);
    eliminarCaracter(porcion_firebase, caracterEliminar2);
    cant = porcion_firebase.toInt();
    eliminarCaracter(porcion_firebase, caracterEliminar1);
    eliminarCaracter(porcion_firebase, caracterEliminar2);
    Firebase.setString(fbdo, F("/App_Proyecto/NewData"), "0");
  }
}

float promedio_nivel2() {
  for (n = 0; n < muestras; n++) {
    distancia2 = (0.0343 / 2) * readUSDistance2();  //Calculamos la distancia en cm
    if (distancia2 > nivmax2) {                     //limitar distancia a 50 cm
      distancia2 = nivmax2;
    }
    nivel2 = nivmax2 - distancia2;
    suma_nivel2 = suma_nivel2 + nivel2;
  }
  nivel2_promedio = suma_nivel2 / (float)(muestras);
  suma_nivel2 = 0;
  return nivel2_promedio;
}