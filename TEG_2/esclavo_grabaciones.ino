#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>

int memoria_grabaciones[5][6];
uint8_t   bloques, cont_grabacion , filas;

#define SD_ChipSelectPin 10  //example uses hardware SS pin 53 on Mega2560
#define microphone_pin A0
TMRpcm audio;   // create an object for use in this sketch 

void inicializar_memoria_grabaciones(){

  for(uint8_t columna = 0; columna < 5; columna++){
    for(uint8_t fila = 0; fila < 6; fila++){
      memoria_grabaciones[columna][fila] = -1;
    }
  }
}

void imprimir_memoria_grabaciones(){
  for(uint8_t fila = 0; fila < 6; fila++){
    for(uint8_t columna = 0; columna < 5; columna++){
      Serial.print(memoria_grabaciones[columna][fila]);
      Serial.print(" ");
    }
    Serial.println();
  } 
}

void setup() {
  Serial.begin(9600);
  audio.setVolume(7);  // Establecer el volumen (0 a 7, donde 7 es el máximo)

  if (!SD.begin(SD_ChipSelectPin)) {  
    Serial.println("sd faild"); 
    return;
  }else{
    Serial.println("SD OK"); 
  }
  // The audio library needs to know which CS pin to use for recording
  audio.CSPin = SD_ChipSelectPin;
  audio.speakerPin = 9;

  bloques = 0;
  cont_grabacion = 0;
  filas = 0 ;

  Serial.println("MEMORIA GRABACIONES INICIALIZADA");
  inicializar_memoria_grabaciones();
  imprimir_memoria_grabaciones();

}

void loop() {

    if (Serial.available() > 0) {
    String mensaje = Serial.readStringUntil('_');  // Leer el mensaje recibido
    Serial.println("Mensaje recibido: " + mensaje);  // Mostrar en el monitor serial

    if(mensaje == "GRABAR-AUDIO"){
      Serial.println("GRABANDO AUDIO");
      Serial.println("INICIANDO GRABACION");
      char nombre_grabacion[7];
      sprintf(nombre_grabacion, "%d.wav", cont_grabacion);
      Serial.print("El número como cadena es:-->");
      Serial.println(nombre_grabacion);
      audio.startRecording(nombre_grabacion,16000,microphone_pin); 
      delay(10000);
      audio.stopRecording(nombre_grabacion); 

      for(uint8_t fila = 0; fila<6; fila++){
        if(memoria_grabaciones[bloques][fila] == -1){
          Serial.print("BLOQUE--> ");
          Serial.println(bloques);
          Serial.print("FILA--> ");
          Serial.println(fila);
          memoria_grabaciones[bloques][fila] = cont_grabacion;
          Serial.print("CONTADOR GRABACIONES--> ");
          Serial.println(cont_grabacion);
          Serial.println("--------GRABANDO: ALMACENADO MEMORIA GRABACIONES-----");
          imprimir_memoria_grabaciones();
          break;
        }
        if(fila == 5){
          Serial.println("NO HAY POSICIONES DISPONIBLES PARA ALMACENAR ESTA GRABACION");
        }
      }

      cont_grabacion++;
    }

    if(mensaje == "REPRODUCIR-GRABACION"){
      Serial.println("REPRODUCIENDO GRABACION");
      Serial.println("REPRODUCIR GRABACION ALMACENADA");
      uint8_t aux_cont_grabacion = 0;
      aux_cont_grabacion = memoria_grabaciones[bloques][filas]; //cada vez que se actualice un bloque fila = 0
      filas++;
      char nombre_grabacion[7];
      sprintf(nombre_grabacion, "%d.wav", aux_cont_grabacion);
      Serial.print("El número como cadena es:-->");
      Serial.println(nombre_grabacion);
      if (SD.exists(nombre_grabacion)) {  // Verificar si el archivo existe
          audio.play(nombre_grabacion);     // Reproducir el archivo de audio
      } else {
        Serial.print("No se encontró el archivo-->");
        Serial.println(nombre_grabacion);
      }

      Serial.println("--------REPRODUCIENDO: ALMACENADO MEMORIA GRABACIONES-----");
      imprimir_memoria_grabaciones();
    }
  }

}
