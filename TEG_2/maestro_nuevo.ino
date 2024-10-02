#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <Adafruit_NeoPixel.h>
#if 0
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif



PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);
#else


PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
#endif

#define MAX_COLORES 8
#define PIN        8
#define NUM_LEDS    20

Adafruit_NeoPixel color_led(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

typedef struct Color{
  uint8_t R;
  uint8_t G;
  uint8_t B;
}Color;
/*-----------------------------------*/

Color colores[MAX_COLORES];

bool escanea_tags = true;
SoftwareSerial mySerial(12, 13); // RX, TX
DFRobotDFPlayerMini dfPlayer;
uint8_t bloque;


/*--------CREA UN NUEVO COLOR DADO SUS VALORES EN RGB------*/
void crear_nuevo_color(uint8_t pos, uint8_t R, uint8_t G, uint8_t B ){

  if (pos < 0 || pos >= MAX_COLORES) {
    Serial.println("Error: posición fuera de rango");
    return;
  }
  colores[pos].R = R;
  colores[pos].G = G;
  colores[pos].B = B;
}

/*----------------------------------------------------------*/

/*-----ESTABLECE EL ARREGLO DE LOS COLORES DISPONIBLES EN LA APLICACION----*/
void inicializar_colores_led(){

  crear_nuevo_color(0,255,255,255); //blanco
  crear_nuevo_color(1,255,45,0); //naranja
  crear_nuevo_color(2,255,125,0); //amarillo
  crear_nuevo_color(3,255,0,0); //rojo
  crear_nuevo_color(4,0,0,255); //azul 
  crear_nuevo_color(5,160,25,150); //morado
  crear_nuevo_color(6,0,255,0); //verde
  crear_nuevo_color(7,166,9,28); //rosa
 
}

/*****************FUNCIONALIDADES ASOCIADAS A LOS LEDS***************************/
/*void encender_luces_semaforo(bool fin_eje_inst){
  
  uint8_t R = 0, G = 0, B = 0;

  if(!fin_eje_inst){
    Serial.println("ENTRA EN CONDICIONAL ENCENDER LUCES LEDS SEMAFORO FUNCION");
    /*if(sistema.luz_verde && !sistema.luz_roja && !sistema.luz_amarilla){
      Serial.println("SE ENCIENDEN LUCES VERDES DEL SEMAFORO");
      sistema.luz_verde = false;
    }
    if(!sistema.luz_verde && sistema.luz_roja && !sistema.luz_amarilla){
      Serial.println("SE ENCIENDEN LUCES ROJAS DEL SEMAFORO");
      sistema.luz_roja = false;
    }
    if(!sistema.luz_verde && !sistema.luz_roja && sistema.luz_amarilla){
      Serial.println("SE ENCIENDEN LUCES AMARILLO DEL SEMAFORO");
      sistema.luz_amarilla = false;
    }*/

  //}else{
    //Serial.println("ENTRA EN CONDICIONAL APAGAR LUCES LEDS SEMAFORO FUNCION");
    /*for(int i = 0; i < NUM_LEDS ; i++){
      color_led.setPixelColor(i, color_led.Color(0,0,0));
    }
    color_led.show();*/

    /*sistema.luz_roja = true;
    sistema.luz_verde =  false;
    sistema.luz_amarilla = false;*/
  //}
 
//}
/*----ENCIENDE O APAGA LAS LUCES LEDS DEL ROBOT DADA LA VARIABLE QUE INDICA SI HA LLEGADO EL TIEMPO DE FINALIZACION DE LA INSTRUCCION-----*/
void encender_luces(){
  Serial.println("ENTRA A ENCENDER LUCES");
  String color_texto = Serial1.readStringUntil('_');
  int color_num = color_texto.toInt();  // Convierte el String a un int
  Serial.println(color_num); 

  uint8_t R = 0, G = 0, B = 0;

  if(color_num == 0){ //BLANCO
    Serial.println("ENTRA A ENCENDER LUCES BLANCAS");
    R = colores[0].R;
    G = colores[0].G;
    B = colores[0].B;
  }
  if(color_num == 1){ //NARANJA
    R = colores[1].R;
    G = colores[1].G;
    B = colores[1].B;
    Serial.println("ENTRA A ENCENDER NARANJAS");
  }
  if(color_num == 2){ //AMARILLO
    R = colores[2].R;
    G = colores[2].G;
    B = colores[2].B;
    Serial.println("ENTRA A ENCENDER LUCES AMARILLAS");
  }
  if(color_num == 3){ //ROJO
    R = colores[3].R;
    G = colores[3].G;
    B = colores[3].B;
    Serial.println("ENTRA A ENCENDER ROJAS");
  }
  if(color_num == 4){ //AZUL
    R = colores[4].R;
    G = colores[4].G;
    B = colores[4].B;
    Serial.println("ENTRA A ENCENDER LUCES AZULES");
  }
  if(color_num == 5){ //MORADO
    R = colores[5].R;
    G = colores[5].G;
    B = colores[5].B;
    Serial.println("ENTRA A ENCENDER LUCES  MORADAS");
  }
  if(color_num == 6){ //VERDE
    R = colores[6].R;
    G = colores[6].G;
    B = colores[6].B;
    Serial.println("ENTRA A ENCENDER LUCES VERDES");
  }
  if(color_num == 7){ //ROSA
    R = colores[7].R;
    G = colores[7].G;
    B = colores[7].B;
    Serial.println("ENTRA A ENCENDER LUCES ROSAS");
  }

  for(uint8_t i = 0; i < NUM_LEDS ; i++){
    color_led.setPixelColor(i, R,G, B);
  }
  color_led.show();
  
  
}

void apagar_luces_leds(){

  for(uint8_t i = 0; i < NUM_LEDS ; i++){
    color_led.setPixelColor(i, color_led.Color(0,0,0));
  }
  color_led.show();
  Serial.println("ENTRA A APAGAR LUCES");
}

void setup() {
  Serial.begin(9600); 
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial.println("NDEF Reader");
  nfc.begin();
  mySerial.begin(9600);

  if (!dfPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini no encontrado.");
    while (true);
  }
  Serial.println("DFPlayer Mini listo.");
  dfPlayer.volume(30);
  bloque = 0;
  inicializar_colores_led();
}


void loop() { 
  delay(1000);
  
  if(escanea_tags){
    Serial.println("ESCANEAR TAGS ES TRUE");
    if (nfc.tagPresent()){
      Serial.println("encontro una tag");
      dfPlayer.play(5);
      NfcTag tag = nfc.read();
      String TagUID = tag.getUidString();

      if (TagUID.length() < 15 - 1) {
        char ptrUID[15];
        strcpy(ptrUID, TagUID.c_str());  // Copia el UID al arreglo
        strcat(ptrUID, "_");              // Añade el delimitador
        Serial.print("CONCATENADO-->");
        Serial.println(ptrUID);
        Serial1.print(ptrUID);             // Envía el UID
        Serial.println("Mando a almacenar el UID al esclavo");
      }
      escanea_tags = false;

    }

  }else{
    if (Serial1.available() > 0) { // Verifica si hay datos disponibles
      Serial.println("HAY DATOS DISPONIBLES PLACA A");
      String respuesta = Serial1.readStringUntil('_'); // Lee la respuesta
    
      Serial.print("REPUESTA A--->");
      Serial.println(respuesta);


      if (respuesta == "TAG-EJECUTAR-PROGRAMA") {
        Serial.println("RECIBIDO TAG-EJECUTAR PROCEDE A EJECUTAR EL PROGRAMA");
        //MANDAR A INCIIALIZAR BLOQUE Y FILAS A 0 EN TAG ESCLAVO GRABACIONES
        escanea_tags = false;
        Serial1.print("EJECUTAR-PROGRAMA_");
      }

      if (respuesta == "FINALIZO-ESCANEAR-TAG") {
        Serial.println("VOLVER A ACTIVAR ESCANEO DE TAGS LUEGO DE HABER ESCANEADO CON ANTERIORIDAD");
        escanea_tags = true;
      }

      if(respuesta == "FINALIZO-EJECUCION-PROGRAMA"){
        Serial.println("VOLVER A ACTIVAR ESCANEO DE TAGS LUEGO DE EJECUTAR PROGRAMA");
        escanea_tags = true;
        //MANDAR A INCIIALIZAR BLOQUE, FILAS Y CONT_GRABACION A 0 EN TAG ESCLAVO GRABACIONES
        //inicializar_memoria_grabaciones();
        Serial2.print("MEMORIA-GRABACIONES_");
      }

      if (respuesta == "EMITIR-SONIDO") {
        Serial.println("RECIBIDO EMITIR-SONIDO PROCEDE A EMITIRLO");
        dfPlayer.play(1);
      }

      if (respuesta == "DETENER-SONIDO") {
        Serial.println("RECIBIDO DETENER-SONIDO PROCEDE A DETENERLO");
        dfPlayer.stop();
      }
      
      if (respuesta == "GRABAR-AUDIO") {
        Serial.println("INICIANDO GRABACION");
        Serial2.print("GRABAR-AUDIO_"); 
      }

      if (respuesta == "REPRODUCIR-GRABACION") {
        Serial.println("REPRODUCIR GRABACION ALMACENADA");
        Serial2.print("REPRODUCIR-GRABACION_"); 
      }

      if (respuesta == "BLOQUE-ANTERIOR") {
        Serial.println("DETECTA UN BLOQUE ANTERIOR");
        //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES
        //Serial2.print("BLOQUE-ANTERIOR_");
       // bloques--;
       bloque--;
       Serial.print("BLOQUE-->");
       Serial.println(bloque);
      }

      if (respuesta == "SIGUIENTE-BLOQUE") {
        Serial.println("DETECTA UN SIGUIENTE BLOQUE");
          //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES
       // Serial2.print("SIGUIENTE-BLOQUE_");
        bloque++;
        Serial.print("BLOQUE-->");
        Serial.println(bloque);
      }

      if (respuesta == "SINCRONIZACION") {
        Serial.println("DETECTA UNA SINCRONIZACION DE NUEVO BLOQUE");
        bloque++;
        Serial.print("BLOQUE-->");
        Serial.println(bloque);
       // Serial2.print("SINCRONIZACION_");
          //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES
      }
      if (respuesta == "BLOQUE-ELIMINADO") {
        Serial.println("BLOQUE ELIMINADO");
        //Serial2.print("BLOQUE-ELIMINADO_");
        bloque = 0;
        Serial.print("BLOQUE-->");
        Serial.println(bloque);
          //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES ELIMINAR BLOQUE DE GRABACIONES CORRRESPONDIENTE
      }
      if (respuesta == "PROGRAMA-RESETEADO") {
        Serial.println("PROGRAMA RESETEADO");
        //Serial2.print("BLOQUE-ELIMINADO_");
        bloque = 0;
        Serial.print("BLOQUE-->");
        Serial.println(bloque);
          //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES ELIMINAR BLOQUE DE GRABACIONES CORRRESPONDIENTE
      }
      if (respuesta == "ACTUALIZAR-BLOQUE") {
        Serial.println("ACTUALIZA EL BLOQUE ACTUAL");
        //Serial2.print("ACTUALIZAR-BLOQUE_");
          //ACTUALIZAR BLOQUE EN ESCLAVO GRABACIONES
        bloque++; //PENDIENTE CON LA REIMPLEMENTACION DE VOLVER A COMENZAR Y EL SETEADO DE LA VARIABLE BLOQUES (COMUNICARSE PARA QUE SEPAS CUANDO SETEARLA)
        Serial.print("BLOQUE-->");
        Serial.println(bloque);
        //filas = 0;
      }
    }
  }
}