#include "DFRobotDFPlayerMini.h"                                                                                                          /*-REALIZADO POR KHAREN URDANETA NIÑO-*/
#include "SoftwareSerial.h"
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <SdFat.h>
#include <UTFT.h>
#include <UTFT_SdRaw.h>
#include "LedControl.h"

#define SD_CHIP_SELECT  53 
SdFat sd;
const int addrL = 0;  // first LED matrix - Left robot eye
const int addrR = 1;  // second LED matrix - Right robot eye
UTFT myGLCD(CTE40,38,39,40,41);
UTFT_SdRaw myFiles(&myGLCD);
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define PIN        8
#define NUM_LEDS    20

Adafruit_NeoPixel color_led(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

#define cant_colum 4 //NUMERO DE COLUMNAS MEMORIA
#define cant_filas 7 //NUMERO DE FILAS MEMORIA

SoftwareSerial DFP(13,12); //RX, TX
Servo rueda_der;
Servo rueda_izq;
Servo cola;
Servo cabeza;
LedControl lc=LedControl(9, 11, 10, 2); //din , clk , cs
DFRobotDFPlayerMini MP3;
extern uint8_t SmallFont[];
#if 0                                               //USANDO CONVENCION SNAKE_CASE
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_SPI pn532spi(SPI, 8);
NfcAdapter nfc = NfcAdapter(pn532spi);

#else

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

#endif

/*------------TIPOS DE DATOS-----------*/
typedef struct Funcionalidad {
	char *UID;
	void *Ptr_func;
  int type;
  struct Funcionalidad *next_func; 
  unsigned long exec_time;
}Funcionalidades;


typedef struct Color{
  uint8_t R;
  uint8_t G;
  uint8_t B;
}Colores;
/*-----------------------------------*/


/*---------------------------VARIABLES GLOBALES-------------------*/
Funcionalidad **funcionalidades = NULL;
int ***memoria_instrucciones = NULL;
int sincronizacion = 0;
bool comenzar_programa = false, finalizar_programa = false, ejecutar_programa = false, grabacion = false;
int volver_a_comenzar = 0;
bool voz_comando = false, luces_semaforo = false;
bool avanzando = false, precaucion = false;
unsigned long *tiempos_eje_bloques = NULL;
Color **colores = NULL;
int num_bloques = 0;
unsigned long tiempo_inicio_programa = 0, tiempo_fin_programa = 0;

/*-----------------------------------------------------------------*/



/*********************************************************************FUNCIONES DE LOGICA DE INSTRUCCIONES Y ALMACENAMIENTO*********************************************************************/

/*----CREA UNA NUEVA FUNCIONALIDAD PARTIENDO DE SU UID,  EL PUNTERO A LA FUNCION QUE LE CORRESPONDE Y SU TIEMPO DE EJECUCION----*/
Funcionalidad *crear_nueva_funcionalidad(String UID, void (*Ptr)(), int type, Funcionalidad *next_func, unsigned long exec_time){
	Funcionalidad *nueva_func = NULL;
  char *puntero = NULL;

	if ((nueva_func = (Funcionalidad *) malloc(sizeof (Funcionalidad))) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
  nueva_func-> UID = NULL;
  nueva_func-> Ptr_func = NULL;
  nueva_func -> type = -1;
  nueva_func -> next_func = NULL;
  nueva_func -> exec_time = 0;

  String cadena = UID;
  puntero = new char[cadena.length() + 1];
  strcpy(puntero, cadena.c_str());
  int t  = cadena.length();
  puntero[11] = '\0';
	(nueva_func-> UID) = puntero;
  
  nueva_func-> Ptr_func = Ptr;
  nueva_func -> type = type;
  nueva_func -> next_func = next_func;
  nueva_func -> exec_time = exec_time;

return nueva_func;
}
/*-------------------------------------------------------------------------------------------------*/

/*---CREA UN ARREGLO CON LAS FUNCIONALIDADES ESTABLECIDAS Y LO INICIALIZA CON SUS RESPECTIVOS VALORES---*/
void crear_arreglo_funcionalidades(){

  if ((funcionalidades =(Funcionalidad **) malloc(sizeof (Funcionalidad *) * 46)) == NULL){
	  Serial.println("funcionalidades: error en el malloc\n");
		exit(1);
  }

  funcionalidades[0] = crear_nueva_funcionalidad("B3 54 7A 12", &prueba, 0, NULL,0); //SINCRONIZACION
  funcionalidades[1] = crear_nueva_funcionalidad("E3 F7 A7 12", &prueba, 0, NULL,0); //VOLVER A COMENZAR
  funcionalidades[2] = crear_nueva_funcionalidad("A3 CE 89 94", &prueba, 0, NULL,0); //COMENZAR PROGRAMA
  funcionalidades[3] = crear_nueva_funcionalidad("43 26 C4 12", &prueba, 0, NULL,0); //FINALIZAR PROGRAMA
  funcionalidades[4] = crear_nueva_funcionalidad("A3 3E 72 94", &prueba, 0, NULL,0); //BORRAR INSTRUCCION
  funcionalidades[5] = crear_nueva_funcionalidad("E3 FC B3 12", &prueba, 0, NULL,0); //EJECUTAR PROGRAMA
  funcionalidades[6] = crear_nueva_funcionalidad("93 02 86 94", &prueba, 0, NULL,0); //RESETEAR PROGRAMA
  funcionalidades[7] = crear_nueva_funcionalidad("13 7C 72 94", &prueba, 0, NULL,0); //INSTRUCCIONES POR COMANDO DE VOZ (CORREGIR UID)
  funcionalidades[8] = crear_nueva_funcionalidad("53 12 73 94", &prueba, 3, NULL,3000); //MOVER CABEZA A LA IZQUIERDA
  funcionalidades[9] = crear_nueva_funcionalidad("F3 94 8B 94", &prueba, 3, NULL,3000); //MOVER CABEZA A LA DERECHA
  funcionalidades[10] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL,5000); //AGITAR COLA
  funcionalidades[11] = crear_nueva_funcionalidad("83 0E AA 12", &prueba, 4, NULL,5000); //AVANZAR
  funcionalidades[12] = crear_nueva_funcionalidad("13 45 8A 94", &prueba, 4, NULL, 1350); //GIRAR A LA DERECHA
  funcionalidades[13] = crear_nueva_funcionalidad("D3 DF 81 94", &prueba, 4, NULL,5400); //GIRAR SOBRE SI MISMO
  funcionalidades[14] = crear_nueva_funcionalidad("B3 23 8F 94", &prueba, 4, NULL,2700); //VOLVER POR LA DERECHA
  funcionalidades[15] = crear_nueva_funcionalidad("13 63 6B 94", &prueba, 4, NULL,5000); //RETROCEDER
  funcionalidades[16] = crear_nueva_funcionalidad("83 11 6B 94", &prueba, 4, NULL, 1350); //GIRAR A LA IZQUIERDA
  funcionalidades[17] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL,15000); //EVITAR OBSTACULOS
  funcionalidades[18] = crear_nueva_funcionalidad("93 E2 20 95", &prueba, 4, NULL, 2700); //VOLVER POR LA IZQUIERDA
  funcionalidades[19] = crear_nueva_funcionalidad("23 DE 6C 94", &prueba, 1, NULL, 5000); //ENCENDER LUCES SEMAFORO
  funcionalidades[20] = crear_nueva_funcionalidad("93 D3 80 94", &cerrar_ojos, 6, NULL, 6000); //CERRAR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("C3 7B 0A 95", &mover_ojos_sorprendidos, 6, NULL, 5000); //MIRAR SORPRENDIDO
  funcionalidades[22] = crear_nueva_funcionalidad("53 5A 84 94", &mover_ojos_felices, 6, NULL, 6000); //MIRAR FELIZ
  funcionalidades[23] = crear_nueva_funcionalidad("53 CF 25 95", &mover_ojos_tristes, 6, NULL, 10000); //MIRAR TRISTE
  funcionalidades[24] = crear_nueva_funcionalidad("63 84 6F 94", &mover_ojos_enojados, 6, NULL, 10000); //MIRAR ENOJADO
  funcionalidades[25] = crear_nueva_funcionalidad("03 F2 D3 A8", &mover_ojos_enamorados, 6, NULL, 6000); //MIRAR ENAMORADO
  funcionalidades[26] = crear_nueva_funcionalidad("63 36 C6 94", &pestanar, 6, NULL, 10000); //PESTANEAR
  funcionalidades[27] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL,10000); //GRABAR AUDIO
  funcionalidades[28] = crear_nueva_funcionalidad("53 2D 89 94", &prueba,7, NULL,10000); //REPRODUCIR GRABACION
  funcionalidades[29] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido,7, NULL,10000); //EMITIR SONIDO
  funcionalidades[30] = crear_nueva_funcionalidad("E3 F3 F0 94", &prueba, 1, NULL, 5000); //BLANCO
  funcionalidades[31] = crear_nueva_funcionalidad("73 4B AA 94", &prueba, 1, NULL, 5000); //NARANJA
  funcionalidades[32] = crear_nueva_funcionalidad("83 9D 6D 94", &prueba, 1, NULL, 5000); //AMARILLO
  funcionalidades[33] = crear_nueva_funcionalidad("E3 D3 6B 12", &prueba, 1, NULL, 5000); //ROJO
  funcionalidades[34] = crear_nueva_funcionalidad("B3 05 6E 12", &prueba, 1, NULL, 5000); //AZUL
  funcionalidades[35] = crear_nueva_funcionalidad("F3 43 C6 12", &prueba, 1, NULL, 5000); //MORADO
  funcionalidades[36] = crear_nueva_funcionalidad("93 F1 C7 12", &prueba, 1, NULL, 5000); //VERDE
  funcionalidades[37] = crear_nueva_funcionalidad("83 75 A9 94", &prueba, 1, NULL, 5000); //ROSA
  funcionalidades[38] = crear_nueva_funcionalidad("A3 D2 B7 12", &prueba, 2, NULL, 0); //SIGUIENTE BLOQUE
  funcionalidades[39] = crear_nueva_funcionalidad("E3 00 69 12", &prueba, 2, NULL, 0); //ANTERIOR BLOQUE
  funcionalidades[40] = crear_nueva_funcionalidad("43 FB 27 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 0
  funcionalidades[41] = crear_nueva_funcionalidad("D3 60 39 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 1
  funcionalidades[42] = crear_nueva_funcionalidad("33 23 98 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 2
  funcionalidades[43] = crear_nueva_funcionalidad("53 3F 92 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 3
  funcionalidades[44] = crear_nueva_funcionalidad("E3 16 67 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 4
  funcionalidades[45] = crear_nueva_funcionalidad("E3 DC 14 A7", &prueba, 0, NULL, 0); //BORRAR BLOQUE INSTRUCCIONES

}
/*-------------------------------------------------------------------------------------*/

/*------CREA UN ARREGLO  DONDE SE ALMACENARAN LOS INDICES DE LAS FUNCIONALIDADES A EJECUTAR EN CADA BLOQUE-----*/
int crear_memoria_instrucciones(){

  if (( memoria_instrucciones = (int ***) malloc(sizeof (int **) * 5)) == NULL){ //SE CREAN LAS DIMENSIONES DE LA MEMORIA
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
  for(int dim = 0; dim<5; dim++){  
    memoria_instrucciones[dim] = (int **) malloc(sizeof(int *) * 5); //SE CREAN LAS COLUMNAS DE CADA DIMENSION DE MEMORIA
  }

  for(int dim = 0; dim < 5; dim++){
    for(int colum = 0 ; colum <5; colum++) //SE CREAN LAS FILAS DE CADA COLUMNA
    {
      memoria_instrucciones[dim][colum] = (int *) malloc(sizeof(int) * 6); 
    }
  }
}
/*---------------------------------------------------------------------------------------------------------------*/

/*------INICIALIZA EL ARREGLO(MEMORIA) QUE SE ENCARGA DE ALMACENAR LAS FUNCIONALIDADES A EJECUTAR EN EL PROGRAMA----*/
void inicializar_memoria(){

  for( int dim = 0 ; dim < 5 ; dim++){
    for(int colum = 0 ; colum < 5 ; colum++){
      for( int fila = 0 ; fila < 6 ; fila++){
        memoria_instrucciones[dim][colum][fila] = 0;
      }   
    }  
  }
}
/*----------------------------------------------------------------------------------------------------------*/

/*---VERIFICA SI UN BLOQUE TIENE INSTRUCCIONES ALMACENADAS. DEVUELVE TRUE EN CASO DE QUE SE ENCUENTRE VACIO---*/
bool verificar_bloque_instrucciones_vacio(int bloque){ //devuelve 1 si se encuentra vacio, 0 si esta lleno
  int vacio = 0;

  for(int colum = 0 ; colum<5 ; colum++){
    if(memoria_instrucciones[bloque][colum][0] == 0){ //SI LO QUE ESTA EN LA PRIMERA POSICION DE LA COLUMNA ES CERO
      vacio ++;
    }
  }
  if(vacio == 5){
    return true;
  }else{
    return false;
  } 
}
/*-----------------------------------------------------------------------------------------------------------*/

/*--------------------ALMACENA UNA INSTRUCCION EN LA MEMORIA DE INSTRUCCIONES DADO SU UID--------------------*/
int almacenar_instruccion(char *UID,  int *selec_col_0, int *selec_col_1, int *selec_col_2, int *selec_col_3, int *selec_col_4){
  char  *UID_aux = NULL;
  int tipo_inst = -1;
  bool cursor = false;

  for(int i = 0; i < 46 ; i++){ //REVISA TODAS LAS FUNCIONALIDADES
    UID_aux = NULL;
    UID_aux = funcionalidades[i]->UID;

    if(strcmp(UID,UID_aux) == 0){  //SI ENCUENTRA LA INSTRUCCION
      imprimir_imagen_tarjeta(i);
      tipo_inst = funcionalidades[i]->type;
      Serial.println("ENCONTRO TAG EN ALMACENAMIENTO FUNCTION");
      if((strcmp(UID,"B3 54 7A 12") == 0) && !verificar_bloque_instrucciones_vacio(0) && num_bloques < 5 ){ //SI SE HA ESCANEADO TAG SINCRONIZACION , EL BLOQUE NO ESTA VACIO Y EL NUMERO DE BLOQUES ES MENOR QUE 5
     //SE PASA AL SIGUIENTE BLOQUE (SINCRONIZACION == BLOQUE)
        num_bloques++;
        sincronizacion = num_bloques;
      }
      
      if((strcmp(UID,"D3 12 74 94") == 0)){ //SI SE HA ESCANEADO TAG GRABAR AUDIO
        void (*Funcionalidad)() = NULL;
        Funcionalidad = (funcionalidades[i])->Ptr_func;
        Funcionalidad();
        tipo_inst = 0;
      }

      if((strcmp(UID,"53 2D 89 94") == 0) && !grabacion){ //SI SE HA ESCANEADO TAG REPRODUCIR AUDIO Y NO HAY GRABACION PREVIA
        tipo_inst = 0;
        Serial.println("NO HAY GRABACION PARA SER REPRODUCIDA");
      }

      if(verificar_instruccion_seleccionada(UID, "43 FB 27 A7", selec_col_0, 0, &cursor) == 1){
        *selec_col_1 = -1;
        *selec_col_2 = -1;
        *selec_col_3 = -1;
        *selec_col_4 = -1;
      }
      if(verificar_instruccion_seleccionada(UID, "D3 60 39 A7", selec_col_1, 1, &cursor) == 1){
        *selec_col_0 = -1;
        *selec_col_2 = -1;
        *selec_col_3 = -1;
        *selec_col_4 = -1;
      }
      
      if(verificar_instruccion_seleccionada(UID, "33 23 98 A7", selec_col_2, 2, &cursor) == 1){
        *selec_col_0 = -1;
        *selec_col_1 = -1;
        *selec_col_3 = -1;
        *selec_col_4 = -1;
      }

      if(verificar_instruccion_seleccionada(UID, "53 3F 92 A7", selec_col_3, 3, &cursor) == 1){
        *selec_col_0 = -1;
        *selec_col_1 = -1;
        *selec_col_2 = -1;
        *selec_col_4 = -1;
      }
      if(verificar_instruccion_seleccionada(UID, "E3 16 67 A7", selec_col_4, 4, &cursor) == 1){
        *selec_col_0 = -1;
        *selec_col_1 = -1;
        *selec_col_2 = -1;
        *selec_col_3 = -1;      
      }
   
      //si no encontraste que le indicaron un color a las luces ponte en modo semaforo
      if((strcmp(UID,"A3 D2 B7 12") == 0) && num_bloques > sincronizacion ){ //si verificaste que hay un bloque despues
        sincronizacion++;
      }

      if((strcmp(UID,"E3 00 69 12") == 0) && num_bloques > 0 ){ //si verificaste que hay un bloque antes
        sincronizacion--;
      }

      if((strcmp(UID,"A3 3E 72 94") == 0)){ //SI SE HA ESCANEADO BORRAR INSTRUCCION

        if(*selec_col_0 != -1){
          eliminar_instruccion(0, *selec_col_0, sincronizacion);
          *selec_col_0 = -1;
        }

        if(*selec_col_1 != -1){
          eliminar_instruccion(1, *selec_col_1, sincronizacion);
          *selec_col_1 = -1;
        }

        if(*selec_col_2 != -1){
          eliminar_instruccion(2, *selec_col_2, sincronizacion);
          *selec_col_2 = -1;
        }
        if(*selec_col_3 != -1){
          eliminar_instruccion(3, *selec_col_3, sincronizacion);
          *selec_col_3 = -1;
        }
        if(*selec_col_4 != -1){
          eliminar_instruccion(4, *selec_col_4, sincronizacion);
          *selec_col_4 = -1;
        } 
      }

      if(strcmp(UID,"E3 DC 14 A7") == 0 ){ 
        if(eliminar_bloque_instrucciones(sincronizacion) != 0){//si esta vacio no hagas nada
          Serial.println("BLOQUE ELIMINADO");
        }
      }

      if(strcmp(UID,"93 02 86 94") == 0){
        resetear_programa();
        Serial.println("RESETEADO EL PROGRAMA");
      }

      switch(tipo_inst){
        case 3: {
          introducir_inst_columna_memoria(i,0,sincronizacion);
          break;
        }
        case 4: {
          introducir_inst_columna_memoria(i,1,sincronizacion);
          break;
        }
        case 5: {
          introducir_inst_columna_memoria(i,2,sincronizacion);
          break;
        }
        case 6: {
          introducir_inst_columna_memoria(i,3,sincronizacion);
          break;
        }
        case 7:{
          introducir_inst_columna_memoria(i,4,sincronizacion);
          break;
        }
      } 
      
      if(!cursor){
        imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      }
      return 1;
    } 
  }
return 0;
}
/*----------------------------------------------------------------------------------------------*/


/*---INTRODUCE EL INDICE DE UNA FUNCIONALIDAD EN LA MEMORIA DE INSTRUCCIONES DADO SU INDICE, LA COLUMNA Y EL BLOQUE DONDE SERA INTRODUCIDA---*/
void introducir_inst_columna_memoria(int indice_func, int colum, int dim){
  bool almacenado = false;

  for( int fila = 0 ; fila<7 ; fila++){
    if(memoria_instrucciones[dim][colum][fila] == 0){
      almacenado = true;
      memoria_instrucciones[dim][colum][fila] = indice_func;
      return;
    } 
  }   
}
/*-------------------------------------------------------------------------------------------------------------------------------------------*/

/*-----------ESCANEA LAS TARJETAS RECIBIENDO COMO PARAMETROS LOS TIEMPOS Y LOS TURNOS DE OJOS PARA CONTROLAR SU ANIMACION-------------*/
void escanear_instrucciones( unsigned long *tiempos_previos_ojos, bool *turnos_ojos){
  char *ptrUID = NULL;
  unsigned long tiempo_ahora = 0;
  int selec_col_0 = -1, selec_col_1 = -1, selec_col_2 = -1, selec_col_3 = -1, selec_col_4 = -1;

  myFiles.load(5, 0, 310, 480, "escanear_tarjeta.RAW", 1 , 0);
  Serial.println("/*----EMPEZANDO----------*/");
  imprimir_matriz();
  Serial.println("/*---------------------------*/");

  while(!ejecutar_programa){
    //mover_ojos_neutros(tiempos_previos_ojos, turnos_ojos);
    //llamar a funcion
    Serial.write("1");
    Serial.println("Vamos a escanear las tags");
    if (nfc.tagPresent()){
      Serial.println("encontro una tag");
      //MP3.play(5);
      tiempo_ahora = millis();   //RETRASO DE 1250MS
      myFiles.load(5, 0, 310, 480, "tarjeta_escaneada.RAW", 1 , 0);

      while(millis() < tiempo_ahora + 500 );
      NfcTag tag = nfc.read();
      String TagUID = tag.getUidString();
      ptrUID = NULL;
      ptrUID = new char[TagUID.length() + 1];
      strcpy(ptrUID, TagUID.c_str());
      Serial.println(ptrUID);
  
      if((strcmp(ptrUID,"13 7C 72 94" ) == 0) && !finalizar_programa && comenzar_programa ){ //SI SE HA ESCANEADO TAG COMANDO VOZ Y ESTA AUN NO HA SIDO ESCANEADA PERO YA SE COMENZO LA ESCRITURA DE INSTRUCCIONES FINALIZALA
        myFiles.load(5, 0, 310, 480, "comando_voz.RAW", 1 , 0);
        //llamar a funcion 
        //delay(2000);
        voz_comando = true;
      }

      if((strcmp(ptrUID, "A3 CE 89 94") == 0) && !comenzar_programa){ //SI SE HA ESCANEADO TAG COMENZAR PROGRAMA Y ESTA NO HA SIDO ESCANEADA SE COMIENZA LA ESCRITURA DEL MISMO 
        comenzar_programa = true;
        imprimir_imagen_tarjeta(2);
      }

      if((strcmp(ptrUID,"43 26 C4 12") == 0) && !finalizar_programa && comenzar_programa){ //SI SE HA ESCANEADO TAG FINALIZAR PROGRAMA Y ESTA AUN NO HA SIDO ESCANEADA PERO YA SE COMENZO LA ESCRITURA DE INSTRUCCIONES FINALIZALA
        finalizar_programa = true; 
      } 

      if((strcmp(ptrUID,"E3 F7 A7 12") == 0) && comenzar_programa && finalizar_programa){ //SI SE HA ESCANEADO LA TAG VOLVER A COMENZAR Y YA SE REALIZO LA ESCRITURA CORRESPONDIENTE DE INSTRUCCIONES INDICA NUEVA ITERACION
        volver_a_comenzar++; //indica el numero de iteraciones a realizar
        imprimir_imagen_tarjeta(1);
      }

      if(comenzar_programa && !finalizar_programa){ //SI SE HA INICIADO LA ESCRITURA DEL PROGRAMA Y NO SE HA FINALIZADO COMIENZA A ALMACENAR LAS PROXIMAS TAG ESCANEADAS EN MEMORIA
        almacenar_instruccion(ptrUID, &selec_col_0, &selec_col_1, &selec_col_2, &selec_col_3, &selec_col_4);
        imprimir_matriz(); 
      }
      
      if((strcmp(ptrUID,"E3 FC B3 12") == 0) && comenzar_programa && finalizar_programa){ //SI SE HA ESCANEADO TAG EJECUTAR PROGRAMA Y YA SE REALIZO LA ESCRITURA CORRESPONDIENTE DE INSTRUCCIONES
        ejecutar_programa = true; 
        comenzar_programa = false;
        finalizar_programa = false;
      }

      tiempo_ahora = millis();   //RETRASO DE 200MS
      while(millis() < tiempo_ahora + 200 );   
    }
  }
}
/*-------------------------------------------------------------------------------------------------------------------------------*/

/*---VERIFICA SI SE HA SELECCIONADO UNA INSTRUCCION EN UNA COLUMNA DADO EL UID DE LA TARJETA JUNTO CON EL UID A COMPARAR, LA POSICION DE LA FUNCIONALIDAD EN LA MATRIZ Y EL ESTADO DEL CURSOR ------*/
int verificar_instruccion_seleccionada(char *UID, char *UID_aux, int *selec_col, int colum, bool *cursor){

  if((strcmp(UID,UID_aux) == 0)){
    *cursor = true;
    if(memoria_instrucciones[sincronizacion][colum][0] == 0){
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      return 1;
    }
    (*selec_col)++;

    if(memoria_instrucciones[sincronizacion][colum][*selec_col] != 0){
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      imprimir_funcion_selec_columna_matriz_funcionalidades(sincronizacion, colum , *selec_col, true); //SELECCIONA EL ELEMENTO INDICADO
      return 1;
    }else{
      *selec_col = -1;
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      return 1;
    }
  }
return 0;
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*---IMPRIME LA MEMORIA DE INSTRUCCIONES A EJECUTAR EN EL PROGRAMA EN EL SERIAL MONITOR---*/
void imprimir_matriz(){

  for( int dim = 0 ; dim<5 ; dim++){
    Serial.print("/----DIMENSION----/: ");
    Serial.println(dim,1);
    for(int filas = 0 ; filas<6 ; filas++){
      Serial.print("\n");
      for( int colum = 0 ; colum<5 ; colum++){
        Serial.print("\t");
        Serial.print( memoria_instrucciones[dim][colum][filas],1);
        Serial.print("\t");
      }
    }
    Serial.println();
  }
}
/*------------------------------------------------------------------------------------*/


 
/*******************************************FUNCIONALIDADES ASOCIADAS A LA EJECUCION DE LAS INTRUCCIONES**********************************************/

/*-----------------EJECUTA LA INSTRUCCION CORRESPONDIENTE DADO EL BLOQUE, LA COLUMNA Y LA FILA DONDE ESTA SE ENCUENTRA ASI COMO OTROS PARAMETROS QUE SE INDICAN A CONTINUACION---------------------------------*/

//TIEMPO_INICIO_BLOQUE : TIEMPO EN EL QUE INICIA EL BLOQUE DADO
//TIEMPO_FIN_BLOQUE : TIEMPO ACTUAL TRANSCURRIDO DESDE QUE COMENZO A EJECUTARSE EL BLOQUE DADO
//TIEMPO_EJE_BLOQUE : TIEMPO ESTABLECIDO QUE DURA EL BLOQUE
//EJECUTADA : BANDERA QUE INDICA SI LA INSTRUCCION YA FUE EJECUTADA PARA NO VOLVER A HACERLO
//TIEMPO_INICIO_COL : TIEMPO EN EL CUAL COMENZO A EJECUTARSE LA COLUMNA DADA
//TIEMPO_FIN_COL : TIEMPO ACTUAL TRANSCURRIDO DESDE QUE SE COMENZO A EJECUTAR LA COLUMNA DADA
//TIEMPO_EXED_COL : BANDERA QUE INDICA SI EN LA COLUMNA DADA SE PRESENTA UN TIEMPO EXCEDENTE EN COMPARACION CON LA DURACION ESTABLECIDA PARA EL BLOQUE
//TIEMPO_INICIO_INST : TIEMPO EN EL CUAL COMENZO A EJECUTARSE LA INSTRUCCION ACTUAL
//TIEMPO_INST : TIEMPO ESTABLECIDO PARA LA EJECUCION DE LA INSTRUCCION ACTUAL
//TIEMPO_EJE_COL : TIEMPO ESTABLECIDO PARA LA EJECUCION DE LA COLUMNA DADA

void ejecutar_columna_instrucciones(int *bloque, int columna, int *fila, unsigned long *tiempo_inicio_bloque, unsigned long *tiempo_fin_bloque, unsigned long *tiempo_eje_bloque, bool *ejecutada, unsigned long *tiempo_inicio_col,unsigned long *tiempo_fin_col, bool *tiempo_exed_col, unsigned long *tiempo_inicio_inst, unsigned long *tiempo_inst, unsigned long *tiempo_eje_col, unsigned long *tiempos_previos_ojos, bool *turnos_ojos){

  void (*Funcionalidad)(bool) = NULL;
  char *ptrUID = NULL;
  
  Serial.print("COLUMNA: ");
  Serial.println(columna);

  if((*tiempo_fin_bloque) - (*tiempo_inicio_bloque) >= 0){//VERIFICA SI EL TIEMPO TRANSCURRIDO ES MAYOR A 0 MICROSEGUNDOS
    if(memoria_instrucciones[*bloque][columna][*fila] != 0){ //VERIFICA SI EXISTE UNA INSTRUCCION PARA EJECUTAR
      if(!(*ejecutada) ){ //VERIFICA SI NO SE HA EJECUTADO LA INSTRUCCION
        Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][columna][*fila]])->Ptr_func;
        ptrUID = (funcionalidades[memoria_instrucciones[*bloque][columna][*fila]])->UID;

        if(columna != 3 && columna != 2){
          Funcionalidad(false);
        }else{
          if(columna == 3){
            inicializar_turnos_ojos(turnos_ojos);
            inicializar_tiempos_ojos(tiempos_previos_ojos);
            realizar_movimiento_ojos(ptrUID, tiempos_previos_ojos, turnos_ojos, 1);   
          }
          if(columna == 2){
            encender_luces(false, ptrUID);
          }
        }
        *ejecutada = true; 

        if((*fila) == 0){ //VERIFICA SI ES LA PRIMERA INSTRUCCION DE LA COLUMNA DADA
          *tiempo_inicio_inst = *tiempo_inicio_bloque;
        }else{
          *tiempo_inicio_inst = millis(); 
        }

      }else{ //VERIFICA SI LA INSTRUCCION YA FUE EJECUTADA
        *tiempo_fin_col = millis();

        if(columna == 3){
          ptrUID = (funcionalidades[memoria_instrucciones[*bloque][columna][*fila]])->UID;
          realizar_movimiento_ojos(ptrUID, tiempos_previos_ojos, turnos_ojos, 2);
        }

        if(!(*tiempo_exed_col)){ //VERIFICA SI AUN NO HAY TIEMPO EXCENDENTE EN ESTA COLUMNA 
          *tiempo_inst = funcionalidades[memoria_instrucciones[*bloque][columna][*fila]]->exec_time;
      
          //VERIFICA SI AUN NO SE HA ESTABLECIDO EL TIEMPO DE ESTA COLUMNA Y LO ESTABLECE
          if((*tiempo_eje_col) == 0){
            *tiempo_eje_col = tiempo_eje_columna_instrucciones(*bloque, columna);
          }

          //VERIFICA SI YA SE CUMPLIO EL TIEMPO DE LA INSTRUCCION, SI A ESE BLOQUE AUN LE QUEDAN INSTRUCCIONES POR EJECUTAR Y SI EL TIEMPO DE EJECUCION DE LA COLUMNA LLEGO A SU FIN
          if(  ((*tiempo_fin_col) - (*tiempo_inicio_col)  >= (*tiempo_eje_col)) && ((*tiempo_fin_col) - (*tiempo_inicio_col)  < *tiempo_eje_bloque) ){
            Serial.println("ENTRA EN EXCEDENTE");
            *tiempo_inst += *tiempo_eje_bloque - (*tiempo_fin_col - *tiempo_inicio_col);
            *tiempo_exed_col = true;
          }
        }

        //VERIFICA SI EL TIEMPO TRANSCURRIDO DESDE QUE COMENZO A EJECUTARSE LA INSTRUCCION ES IGUAL AL ESTABLECIDO PARA SU EJECUCION O SI EL TIEMPO ESTABLECIDO PARA LA EJECUCION DE LA COLUMNA YA SE CUMPLIO.
        if(((*tiempo_fin_col) - (*tiempo_inicio_inst)  >= *tiempo_inst) || ((*tiempo_fin_col) - (*tiempo_inicio_col)  >= (*tiempo_eje_bloque)) ){ 
          Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][columna][*fila]])->Ptr_func;
          Serial.println("FINALIZO LA INSTRUCCION");
          if(columna !=3 && columna != 2){
            Funcionalidad(true); //DESACTIVA LA INSTRUCCION
          }

          if(columna == 2){
            encender_luces(true, ptrUID);
          }
          (*fila)++;// INCREMENTA LA FILA A RECORRER
          *ejecutada = false; 
          *tiempo_exed_col = false;
        }
      }    
    }
  }
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*---DETERMINA EL TIEMPO DE EJECUCION DE UNA COLUMNA DADO EL BLOQUE DE INSTRUCCIONES AL QUE PERTENECE---*/
unsigned long tiempo_eje_columna_instrucciones(int bloque, int colum){
  unsigned long duracion = 0, duracion_max = 0;
    
    for(int fila = 0; fila < 6; fila++){
      if(memoria_instrucciones[bloque][colum][fila] != 0){
        duracion += funcionalidades[memoria_instrucciones[bloque][colum][fila]]->exec_time;
      }else{
        break;
      }
    }
return duracion;
}
/*--------------------------------------------------------------------------------------------------------*/


/*---DETERMINA EL TIEMPO MAXIMO QUE DURA UN BLOQUE PARTIENDO DE LA COLUMNA QUE TARDA MAS EN EJECUTARSE---*/
unsigned long tiempo_duracion_bloque_instrucciones(int bloque){ 
  unsigned long duracion = 0, duracion_max = 0;

  for(int colum = 0 ; colum < 5; colum++){
    duracion = 0;
    for(int fila = 0; fila < 6; fila++){
      if(memoria_instrucciones[bloque][colum][fila] != 0){
        duracion += funcionalidades[memoria_instrucciones[bloque][colum][fila]]->exec_time;
      }else{
        break;
      }
    }
    
    if(duracion != 0){
      if(duracion > duracion_max){
        duracion_max = duracion;
      }
    }
  }
return duracion_max;
}
/*-------------------------------------------------------------------------------------------------*/


/*---DETERMINA EL TIEMPO QUE DURA EL PROGRAMA PARTIENDO DEL TIEMPO DE EJECUCION DE CADA BLOQUE---*/
unsigned long determinar_duracion_programa(){
  unsigned long tiempo_eje_programa = 0;

  for(int i = 0; i< 5 ; i++){
    tiempo_eje_programa += tiempos_eje_bloques[i];
    Serial.println(tiempos_eje_bloques[i]);
  }

return tiempo_eje_programa;
}
/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------FUNCIONES RELACIONADAS A LA CONCURRENCIA DE EJECUCION DE INSTRUCCIONES DE LA MATRIZ MEMORIA---------------------------------------------------------------------------------------*/

/*--------CREA UN ARREGLO CON LOS TIEMPOS DE EJECUCION DE CADA BLOQUE DE INSTRUCCIONES-----*/
unsigned long *crear_arreglo_tiempos_ejecucion_bloques(){
  unsigned long *nuevos_tiempos_eje_bloques = NULL;

  if ((nuevos_tiempos_eje_bloques = (unsigned long *) malloc(sizeof (unsigned long) * 5)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

return nuevos_tiempos_eje_bloques;}
/*------------------------------------------------------------------------------------------*/


/*---INICIALIZA EL ARREGLO QUE ALMACENA LOS TIEMPOS DE EJECUCION DE CADA BLOQUE---*/
void inicializar_arreglo_tiempos_ejecucion_bloques(){ 

  for(int i = 0; i < 5; i++){
    tiempos_eje_bloques[i] = 0;
  }
}
/*--------------------------------------------------------------------------------*/


/*--ESTABLECE LOS TIEMPOS DE EJECUCION DE CADA BLOQUE DE INSTRUCCIONES EN EL ARREGLO QUE LOS ALMACENA--*/
void establecer_tiempos_eje_arreglo_tiempos_bloques(unsigned long tiempo_bloque_0,unsigned long tiempo_bloque_1,unsigned long tiempo_bloque_2, unsigned long tiempo_bloque_3, unsigned long tiempo_bloque_4){

  tiempos_eje_bloques[0] = tiempo_bloque_0;
  tiempos_eje_bloques[1] = tiempo_bloque_1;
  tiempos_eje_bloques[2] = tiempo_bloque_2;
  tiempos_eje_bloques[3] = tiempo_bloque_3;
  tiempos_eje_bloques[4] = tiempo_bloque_4;
}
/*----------------------------------------------------------------------------------------------------*/



/***********************************************FUNCIONALIDADES ADMINISTRATIVAS*****************************************/

/*------ELIMINA UNA INSTRUCCION DE LA MEMORIA DADO EL BLOQUE, LA COLUMNA Y LA FILA DONDE ESTA SE UBICA-----*/
void eliminar_instruccion(int colum, int fila, int bloque){
  int *columna_auxiliar = NULL;

  columna_auxiliar = (int) malloc(sizeof(int) * 6);

  for(int i = 0; i < 6; i++){
    columna_auxiliar[i] = 0;
  }
  
  for(int i = 0, j = 0; i < 6; i++){
    if(i != fila){
      columna_auxiliar[j] = memoria_instrucciones[bloque][colum][i];
      j++;
    }
  }

  for(int i = 0; i < 6; i++){
    Serial.print(columna_auxiliar[i]);
    Serial.print(" ");
  }
  Serial.println();

  for(int i = 0; i < 6; i++){
    memoria_instrucciones[bloque][colum][i] = columna_auxiliar[i];
  }

  free(columna_auxiliar);

  Serial.println("ELIMINADA INSTRUCCION");
}
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------ELIMINA UN BLOQUE DADO EL INDICE DEL MISMO------------------------------------*/
int eliminar_bloque_instrucciones(int bloque){
  int ***memoria_auxiliar = NULL;

  if(verificar_bloque_instrucciones_vacio(bloque)){
    Serial.println("ESTE BLOQUE ESTA VACIO DEBE SER LLENADO");
    return 0;
  }

  if (( memoria_auxiliar = (int ***) malloc(5 * sizeof (int **))) == NULL){ //SE CREAN LOS BLOQUES
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
  for(int dim = 0; dim<5; dim++){  
    memoria_auxiliar[dim] = (int **) malloc(sizeof(int *) * 5); //SE CREAN LAS COLUMNAS DE CADA BLOQUE DE INSTRUCCIONES
  }

  for(int dim = 0; dim < 5; dim++){
    for(int colum = 0 ; colum <5; colum++) //SE CREAN LAS FILAS DE INSTRUCCIONES
    {
      memoria_auxiliar[dim][colum] = (int *) malloc(sizeof(int) * 6); 
    }
  }

  for( int dim = 0 ; dim < 5 ; dim++){
    for(int colum = 0 ; colum < 5 ; colum++){
      for( int filas = 0 ; filas < 6 ; filas++){
        memoria_auxiliar[dim][colum][filas] = 0;
      }   
    }  
  }

  for( int dim = 0, dim_aux = 0 ; dim<5 ; dim++){
    if(dim != bloque){
      for(int colum = 0 ; colum<5 ; colum++){
        for( int filas = 0 ; filas<6 ; filas++){ 
          memoria_auxiliar[dim_aux][colum][filas] = memoria_instrucciones[dim][colum][filas];
        }
      }
      dim_aux++;
    }
  }

  for( int dim = 0 ; dim<5 ; dim++){
    for(int colum = 0 ; colum<5 ; colum++){
      for( int filas = 0 ; filas<6 ; filas++){ 
        memoria_instrucciones[dim][colum][filas] = memoria_auxiliar[dim][colum][filas];
      }
    } 
  }
  
  Serial.println("LIBERANDO MEMORIA");

  for(int dim = 0; dim < 5; dim++){
    for(int colum = 0 ; colum <5; colum++) //SE CREAN LOS BLOQUES DE INSTRUCCIONES
    {
      free(memoria_auxiliar[dim][colum]);
    }
  }

  for(int dim = 0; dim<5; dim++){  
    free(memoria_auxiliar[dim]); //SE CREAN LAS FILAS DE CADA COLUMNA DE INSTRUCCIONES
  }

  Serial.println("FIN DE LIBERACION MEMORIA");

  Serial.println("/*----IMPRIMIENDO MATRIZ LUEGO DE LA ELIMINACION-----*/");
  imprimir_matriz();
  Serial.println("/*----FIN IMPRIMIR MATRIZ DE LA ELIMINACION-----*/");

  if(num_bloques != 0){
    num_bloques--;
  }
  sincronizacion = 0;

return 1;}
/*---------------------------------------------------------------------------------------------------------------------*/

/*----------------RESETEA TODAS LAS VARIABLES DEL PROGRAMA PARA DAR INICIO A UNA NUEVA PROGRAMACION--------------------*/
void resetear_programa(){
  grabacion = false;
  volver_a_comenzar = 0;
  comenzar_programa = false;
  finalizar_programa = false;
  sincronizacion = 0;
  inicializar_memoria();
  //inicializar_memoria_colores_luces();
  //INICIALIZAR MEMORIA GRABACIONES
  voz_comando = false;
  //INICIALIZAR BANDERAS LUCES SEMAFOROS
  ejecutar_programa = false;
  num_bloques = 0;

  Serial.println("MEMORIA RESETEADA");
  imprimir_matriz();
}
/*---------------------------------------------------------------------------------------------------------------------*/

/*****************************************FUNCIONALIDADES ASOCIADAS A LOS COLORES***************************************/

/*--------CREA UN NUEVO COLOR DADO SUS VALORES EN RGB------*/
void crear_nuevo_color(int pos, uint8_t R, uint8_t G, uint8_t B ){

  if ((colores[pos] = (Color *) malloc(sizeof (Color))) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
  colores[pos]->R = R;
  colores[pos]->G = G;
  colores[pos]->B = B;

}
/*----------------------------------------------------------*/

/*-----ESTABLECE EL ARREGLO DE LOS COLORES DISPONIBLES EN LA APLICACION----*/
void inicializar_colores_led(){
  if ((colores = (Color **) malloc(sizeof (Color) * 8)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
  crear_nuevo_color(0,255,255,255); //blanco
  crear_nuevo_color(1,255,45,0); //naranja
  crear_nuevo_color(2,255,125,0); //amarillo
  crear_nuevo_color(3,255,0,0); //rojo
  crear_nuevo_color(4,0,0,255); //azul 
  crear_nuevo_color(5,160,25,150); //morado
  crear_nuevo_color(6,0,255,0); //verde
  crear_nuevo_color(7,166,9,28); //rosa
 
}
/*------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------------*/

void verificar_color_semaforo_movimiento_traslacion(int indice_func){

  if((indice_func == 11) || (indice_func == 12) || (indice_func == 13) || (indice_func == 14) || (indice_func == 16) || (indice_func == 18) ){
    avanzando = true;
  }
  if((indice_func == 15) || (indice_func == 17) ){
    precaucion = true;
  }
}
/****************************************************************************************************************/


/*****************************************************FUNCIONES ASOCIADAS A LOS OJOS*****************************************************/

void displayEmotion(byte left[8], byte right[8]) {
  lc.clearDisplay(addrL);
  lc.clearDisplay(addrR);
  for(int row=0;row<8;row++) {
    lc.setRow(addrL,row,left[row]);
    lc.setRow(addrR,row,right[row]);
  }
}

/*--------SE ENCARGA DE INICIALIZAR LOS DISPLAYS DE OJOS--------*/
void inicializar_ojos(){
  unsigned long tiempo_actual = 0; 
  lc.shutdown(addrL,false);
  lc.shutdown(addrR,false);
  /* Set the brightness to max values */
  lc.setIntensity(addrL,15);
  lc.setIntensity(addrR,15);
  /* and clear the display */
  lc.clearDisplay(addrL);
  lc.clearDisplay(addrR);
}
/*------------------------------------------------------------*/

/*--------CREA EL ARREGLO DONDE SE ALMACENAN LOS TURNOS CORRESPONDIENTES QUE CONTROLAN LA ANIMACION DE LOS OJOS-----------*/
bool *crear_arreglo_turnos_ojos(){
  bool *turnos_ojos = NULL;
  if ((turnos_ojos = (bool *) malloc(sizeof (bool) * 6)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

return turnos_ojos;}
/*-------------------------------------------------------------------------------------------------------------------------*/

/*--------------CREA EL ARREGLO DONDE SE ALMACENAN LOS TIEMPOS TRANSCURRIDOS QUE CONTROLAN LA ANIMACION DE LOS OJOS--------*/
unsigned long *crear_arreglo_tiempos_ojos(){
  unsigned long *tiempos_previos_ojos = NULL;

  if ((tiempos_previos_ojos = (unsigned long *) malloc(sizeof (unsigned long) * 6)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

return tiempos_previos_ojos;}
/*-------------------------------------------------------------------------------------------------------------------------*/

/*-----------INICIALIZA EL ARREGLO DONDE SE ALMACENA LOS TIEMPO TRANSCURRIDOS QUE CONTROLAN LA ANIMACION DE LOS OJOS-------*/
void inicializar_tiempos_ojos(unsigned long *tiempos_previos_ojos){
  
  for(int i = 0; i< 6 ; i++){
    tiempos_previos_ojos[i] = 0;
  }
}
/*-------------------------------------------------------------------------------------------------------------------------*/

/*----INICIALIZA  EL ARREGLO DONDE SE ALMACENAN LOS TURNOS CORRESPONDIENTES QUE CONTROLAN LA ANIMACION DE LOS OJOS---------*/
void inicializar_turnos_ojos(bool *turnos_ojos){
  turnos_ojos[0] = true;

  for(int i = 1; i< 6 ; i++){
    turnos_ojos[i] = false;
  }
}
/*-------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 0 DE LA ANIMACION DE LOS OJOS NEUTROS------*/
void ojos_neutros_pos_0(){

  byte left_u[8] = {
    0b00000000,
    0b00111100,
    0b01001110,
    0b01001110,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00000000
  };

  displayEmotion(left_u, left_u);
}
/*----------------------------------------------------------------------------*/


/*-------ESTABLECE LA POSICION NRO 1 DE LA ANIMACION DE LOS OJOS NEUTROS------*/
void ojos_neutros_pos_1(){
 
  byte right_u[8] = {
    0b00000000,
    0b00111100,
    0b01110010,
    0b01110010,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00000000
  };

  displayEmotion(right_u, right_u);
}
/*-----------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 2 DE LA ANIMACION DE LOS OJOS NEUTROS------*/
void ojos_neutros_pos_2(){
 
  byte right_d[8] = {
    0b00000000,
    0b00111100,
    0b01111110,
    0b01111110,
    0b01110010,
    0b01110010,
    0b00111100,
    0b00000000
  };
  displayEmotion(right_d, right_d);
}
/*-----------------------------------------------------------------------------*/


/*-------ESTABLECE LA POSICION NRO 3 DE LA ANIMACION DE LOS OJOS NEUTROS------*/
void ojos_neutros_pos_3(){

  byte left_d[8] = {
    0b00000000,
    0b00111100,
    0b01111110,
    0b01111110,
    0b01001110,
    0b01001110,
    0b00111100,
    0b00000000
  };

  displayEmotion(left_d, left_d);
}
/*-----------------------------------------------------------------------------*/


/*--REALIZA EL MOVIMIENTO DE ANIMACION DE LOS OJOS NEUTROS RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void mover_ojos_neutros(unsigned long *tiempo_previo, bool *turnos_ojos ){

  if((millis() - tiempo_previo[0] >= 3000) && turnos_ojos[0] ){
    ojos_neutros_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
    //Serial.println("OJOS 0");
  }
  
  if((millis() - tiempo_previo[1] >= 3000) && turnos_ojos[1]){
    ojos_neutros_pos_1();
    turnos_ojos[1] = false;
    turnos_ojos[2] = true;
    tiempo_previo[2] = millis();
    //Serial.println("OJOS 1");
  }

  if((millis() - tiempo_previo[2] >= 3000) && turnos_ojos[2]){
    ojos_neutros_pos_2();
    turnos_ojos[2] = false;
    turnos_ojos[3] = true;
    tiempo_previo[3] = millis();
    //Serial.println("OJOS 2");
  }

  if((millis() - tiempo_previo[3] >= 3000) && turnos_ojos[3]){
    ojos_neutros_pos_3();
    turnos_ojos[3] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
    //Serial.println("OJOS 3");
  }
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 0 DE LA ANIMACION DE LOS OJOS ENAMORADOS------*/
void ojos_enamorados_pos_0(){

  byte left[8] = {
    0b00000000,
    0b00011100,
    0b00111100,
    0b01111000,
    0b01111000,
    0b00111100,
    0b00011100,
    0b00000000
  };

  displayEmotion(left, left);

}
/*--------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 1 DE LA ANIMACION DE LOS OJOS ENAMORADOS------*/
void ojos_enamorados_pos_1(){

  byte right[8] = {
    0b00001100,
    0b00011110,
    0b00111110,
    0b01111100,
    0b01111100,
    0b00111110,
    0b00011110,
    0b00001100
  };

  displayEmotion(right, right);

}
/*-------------------------------------------------------------------------------*/

/*--REALIZA EL MOVIMIENTO DE ANIMACION DE LOS OJOS ENAMORADOS RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void mover_ojos_enamorados(unsigned long *tiempo_previo, bool *turnos_ojos){
  
  if((millis() - tiempo_previo[0] >= 1000) && turnos_ojos[0]){
    ojos_enamorados_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
  }
  
  if((millis() - tiempo_previo[1] >= 500) && turnos_ojos[1]){
    ojos_enamorados_pos_1();
    turnos_ojos[1] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
  }

}
/*----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 0 DE LA ANIMACION DE LOS OJOS ENOJADOS------*/
void ojos_enojados_pos_0(){

  byte left[8] = {
    0b00000000,
    0b01111100,
    0b11110010,
    0b11110010,
    0b11111100,
    0b11111000,
    0b01110000,
    0b00000000
  };

  byte right[8] = {
    0b00000000,
    0b01110000,
    0b11111000,
    0b11111100,
    0b11110010,
    0b11110010,
    0b01111100,
    0b00000000
  };

 
  displayEmotion(left, right);
}
/*-------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 1 DE LA ANIMACION DE LOS OJOS ENOJADOS------*/
void ojos_enojados_pos_1(){

  byte left_l[8] = {
    0b00000000,
    0b01111100,
    0b10011110,
    0b10011110,
    0b11111100,
    0b11111000,
    0b01110000,
    0b00000000
  };

  byte right_l[8] = {
    0b00000000,
    0b01110000,
    0b10011000,
    0b10011100,
    0b11111110,
    0b11111110,
    0b01111100,
    0b00000000

  };

  displayEmotion(left_l, right_l);

}
/*-------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 2 DE LA ANIMACION DE LOS OJOS ENOJADOS------*/
void ojos_enojados_pos_2(){
 
  byte left_u[8] = {
    0b00000000,
    0b01111100,
    0b11111110,
    0b11111110,
    0b10011100,
    0b10011000,
    0b01110000,
    0b00000000
  };

  byte right_u[8] = {
    0b00000000,
    0b01110000,
    0b11111000,
    0b11111100,
    0b10011110,
    0b10011110,
    0b01111100,
    0b00000000
  };

  displayEmotion(left_u, right_u);

}
/*-----------------------------------------------------------------------------*/

/*--REALIZA EL MOVIMIENTO DE ANIMACION DE LOS OJOS ENOJADOS RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void mover_ojos_enojados(unsigned long *tiempo_previo, bool *turnos_ojos){
  
  if((millis() - tiempo_previo[0] >= 2500) && turnos_ojos[0] ){
    ojos_enojados_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
  }
  
  if((millis() - tiempo_previo[1] >= 2500) && turnos_ojos[1]){
    ojos_enojados_pos_1();
    turnos_ojos[1] = false;
    turnos_ojos[2] = true;
    tiempo_previo[2] = millis();
  }

  if((millis() - tiempo_previo[2] >= 2500) && turnos_ojos[2]){
    ojos_enojados_pos_2();
    turnos_ojos[2] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
  }

}
/*----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 0 DE LA ANIMACION DE LOS OJOS TRISTES------*/
void ojos_tristes_pos_0(){
  
  byte left[8] = {
    0b00000000,
    0b01111000,
    0b10110000,
    0b10110000,
    0b11110000,
    0b11110000,
    0b01111000,
    0b00000000
  };

  displayEmotion(left, left);
 
}
/*-----------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 1 DE LA ANIMACION DE LOS OJOS TRISTES------*/
void ojos_tristes_pos_1(){

  byte right[8] = {
    0b00000000,
    0b01111000,
    0b11110000,
    0b11110000,
    0b10110000,
    0b10110000,
    0b01111000,
    0b00000000
  };
  displayEmotion(right, right);

}
/*----------------------------------------------------------------------------*/

/*--REALIZA EL MOVIMIENTO DE ANIMACION DE LOS OJOS TRISTES RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void mover_ojos_tristes(unsigned long *tiempo_previo, bool *turnos_ojos){
  
  if((millis() - tiempo_previo[0] >= 2500) && turnos_ojos[0] ){
    ojos_tristes_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
  }
  
  if((millis() - tiempo_previo[1] >= 2500) && turnos_ojos[1]){
    ojos_tristes_pos_1();
    turnos_ojos[1] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
  }
}
/*------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 0 DE LA ANIMACION DE LOS OJOS FELICES------*/
void ojos_felices_pos_0(){

  byte left[8] = {
    0b00000000,
    0b00111100,
    0b00011010,
    0b00011010,
    0b00011110,
    0b00011110,
    0b00111100,
    0b00000000
  };

  displayEmotion(left, left);
}
/*----------------------------------------------------------------------------*/

/*-------ESTABLECE LA POSICION NRO 1 DE LA ANIMACION DE LOS OJOS FELICES------*/
void ojos_felices_pos_1(){

  byte right[8] = {
    0b00000000,
    0b00111100,
    0b00011110,
    0b00011110,
    0b00011010,
    0b00011010,
    0b00111100,
    0b00000000
  };

  displayEmotion(right, right);

}
/*----------------------------------------------------------------------------*/

/*--REALIZA EL MOVIMIENTO DE ANIMACION DE LOS OJOS FELICES RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void mover_ojos_felices(unsigned long *tiempo_previo, bool *turnos_ojos){
  
  if((millis() - tiempo_previo[0] >= 750) && turnos_ojos[0] ){
    ojos_felices_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
  }
  
  if((millis() - tiempo_previo[1] >= 750) && turnos_ojos[1]){
    ojos_felices_pos_1();
    turnos_ojos[1] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
  }
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-------ESTABLECE LA ANIMACION DE LOS OJOS CERRADOS------*/
void cerrar_ojos(){

  byte left[8] = {
    0b00000000,
    0b00100000,
    0b01100000,
    0b01100000,
    0b01100000,
    0b01100000,
    0b00100000,
    0b00000000
  };

  displayEmotion(left, left); 

}
/*-------------------------------------------------------*/

/*-------ESTABLECE LA ANIMACION DE LOS OJOS SORPRENDIDOS------*/
void mover_ojos_sorprendidos(){

  byte left[8] = {
    0b00000000,
    0b00111100,
    0b01111110,
    0b01100110,
    0b01100110,
    0b01111110,
    0b00111100,
    0b00000000
  };

  displayEmotion(left, left); 

}
/*------------------------------------------------------------*/

/*--REALIZA EL MOVIMIENTO DE ANIMACION DE PESTANEO RECIBIENDO COMO PARAMETROS EL TIEMPO Y EL ESTADO DE EJECUCION(TURNO) DE CADA POSICION DE OJOS--*/
void pestanar(unsigned long *tiempo_previo, bool *turnos_ojos){
  
  if((millis() - tiempo_previo[0] >= 2000) && turnos_ojos[0]){
    ojos_neutros_pos_0();
    turnos_ojos[0] = false;
    turnos_ojos[1] = true;
    tiempo_previo[1] = millis();
  }
  
  if((millis() - tiempo_previo[1] >= 500) && turnos_ojos[1]){
    cerrar_ojos();
    turnos_ojos[1] = false;
    turnos_ojos[2] = true;
    tiempo_previo[2] = millis();
  }

  if((millis() - tiempo_previo[2] >= 2000) && turnos_ojos[2]){
    ojos_neutros_pos_3();
    turnos_ojos[2] = false;
    turnos_ojos[3] = true;
    tiempo_previo[3] = millis();
  }

  if((millis() - tiempo_previo[3] >= 500) && turnos_ojos[3]){
    cerrar_ojos();
    turnos_ojos[3] = false;
    turnos_ojos[0] = true;
    tiempo_previo[0] = millis();
  }
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/*---REALIZA EL MOVIMIENTO CORRESPONDIENTE DE OJOS DE ACUERDO A EL UID PROPORCIONADO------*/
//TIPO 1: INDICA QUE NO REQUIERE ANIMACION 
//TIPO 2: REQUIERE ANIMACION POR LO TANTO USA EL ARREGLO DE TIEMPOS Y TURNOS

void realizar_movimiento_ojos(char *ptrUID, unsigned long *tiempos_previos_ojos, bool *turnos_ojos, int tipo){

  if(tipo == 1){
    if(strcmp(ptrUID, "93 D3 80 94") == 0){
      Serial.println("CERRAR OJOS");
      cerrar_ojos();
    }
    if(strcmp(ptrUID, "C3 7B 0A 95") == 0){
      Serial.println("MIRAR SORPRENDIDO");
      mover_ojos_sorprendidos();
    }
  }

  if(tipo == 2){
    if(strcmp(ptrUID, "53 5A 84 94") == 0){
      Serial.println("MIRAR FELIZ");
      mover_ojos_felices(tiempos_previos_ojos, turnos_ojos);
    }
    if(strcmp(ptrUID, "53 CF 25 95") == 0){
      Serial.println("MIRAR TRISTE");            
      mover_ojos_tristes(tiempos_previos_ojos, turnos_ojos);
    }
    if(strcmp(ptrUID, "63 84 6F 94") == 0){
      mover_ojos_enojados(tiempos_previos_ojos, turnos_ojos);
    }
    if(strcmp(ptrUID, "03 F2 D3 A8") == 0){
      Serial.println("MIRAR ENAMORADO");
      mover_ojos_enamorados(tiempos_previos_ojos, turnos_ojos);
    }
    if(strcmp(ptrUID, "63 36 C6 94") == 0){
      Serial.println("PESTANEAR");
      pestanar(tiempos_previos_ojos, turnos_ojos);
    }
  }
}
/***************************************************FUNCIONES ASOCIADAS A LA PANTALLA****************************************************/

/*------INICIALIZA LA PANTALLA TFT-------*/
void inicializar_pantalla_tft(){
  Serial.println(F("Initialising SD card..."));
  bool mysd = 0;

  while (!mysd)
  {
    if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) {
      Serial.println(F("Card failed, or not present"));
      Serial.println(F("Retrying...."));
    }
    else
    {
      mysd = 1;
      Serial.println(F("Card initialised."));
    }
  }
  Serial.println(F("Initialising LCD."));
  myGLCD.InitLCD(PORTRAIT);
  myGLCD.clrScr();
  Serial.println(F("LCD initialised."));
}
/*-------------------------------------*/

/*--IMPRIME EL ICONO DE LA FUNCION SELECCIONADA DE LA MATRIZ DE FUNCIONALIDADES DADO EL BLOQUE, LA COLUMNA, EL INDICE DE LA FUNCIONALIDAD Y EL ESTADO CORRESPONDIENTE DE LA MISMA---*/
void imprimir_funcion_selec_columna_matriz_funcionalidades(int bloque, int columna, int indice_fila_selec, bool seleccion){
  int cord_y = 0;

  switch(bloque){
    case 0: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_0_1.RAW", 1 , 0);
      break;
    }
    case 1: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_1_1.RAW", 1 , 0);
      break;
    }
    case 2: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_2_1.RAW", 1 , 0);
      break;
    }
    case 3: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_3_1.RAW", 1 , 0);
      break;
    }
    case 4: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_4_1.RAW", 1 , 0);
      break;
    }
  }

  for(int fila = 0; fila < 6 ; fila++){
    cord_y = cord_y + 65;
    if(fila == indice_fila_selec){
      imprimir_icono_funcionalidad(1, memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila < indice_fila_selec && !seleccion){
      imprimir_icono_funcionalidad(-1, memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila < indice_fila_selec && seleccion){
      imprimir_icono_funcionalidad(0, memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila > indice_fila_selec){
      imprimir_icono_funcionalidad(0, memoria_instrucciones[bloque][columna][fila], cord_y); 
    }      
  }
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-----IMPRIME EL ICONO DE LA FUNCIONALIDAD EN LA PANTALLA DADO EL ESTADO DE LA MISMA, SU INDICE Y LA POSICION EN EL EJE Y----*/
void imprimir_icono_funcionalidad(int estado, int indice, int pos_y){
  
  switch(indice){
    case 8 :{
      if(estado == 0){
        myFiles.load(0, pos_y, 60, 60, "mover_cab_izq.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(0, pos_y, 60, 60, "selec_mover_cab_izq.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(0, pos_y, 60, 60, "eje_mover_cab_izq.RAW", 1 , 0);
      }
      break;
    }
    case 9 :{
      if(estado == 0){
        myFiles.load(0, pos_y, 60, 60, "mover_cab_der.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(0, pos_y, 60, 60, "selec_mover_cab_der.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(0, pos_y, 60, 60, "eje_mover_cab_der.RAW", 1 , 0);
      }
      break;
    }
    case 10 :{
      if(estado == 0){
        myFiles.load(0, pos_y, 60, 60, "agitar_cola.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(0, pos_y, 60, 60, "selec_agitar_cola.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(0, pos_y, 60, 60, "eje_agitar_cola.RAW", 1 , 0);
      }
      break;
    }
    case 11 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "avanzar.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_avanzar.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_avanzar.RAW", 1 , 0);
      }
      break;
    }
    case 12 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "girar_der.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_girar_der.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_girar_der.RAW", 1 , 0);
      }
      break;
    }
    case 13 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "girar_sobre_si.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_girar_sobre_si.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_girar_sobre_si.RAW", 1 , 0);
      }
      break;
    }
    case 14 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "volver_der.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_volver_der.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_volver_der.RAW", 1 , 0);
      }
      break;
    }
    case 15 :{
       if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "retroceder.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_retroceder.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_retroceder.RAW", 1 , 0);
      }
      break;
    }
    case 16 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "girar_izq.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_girar_izq.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_girar_izq.RAW", 1 , 0);
      }
      break;
    }
    case 17 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "evitar_obs.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_evitar_obs.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_evitar_obs.RAW", 1 , 0);
      }
      break;
    }
    case 18 :{
      if(estado == 0){
        myFiles.load(65, pos_y, 60, 60, "volver_izq.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(65, pos_y, 60, 60, "selec_volver_izq.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(65, pos_y, 60, 60, "eje_volver_izq.RAW", 1 , 0);
      }
      break;
    }
    case 19 :{
      if(estado == 0){
        myFiles.load(130, pos_y, 60, 60, "encender_leds.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(130, pos_y, 60, 60, "selec_encender_leds.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(130, pos_y, 60, 60, "eje_encender_leds.RAW", 1 , 0);
      }
      break;
    }
    case 20 :{
      if(estado == 0){
        myFiles.load(195, pos_y, 60, 60, "cerrar_ojos.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(195, pos_y, 60, 60, "selec_cerrar_ojos.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(195, pos_y, 60, 60, "eje_cerrar_ojos.RAW", 1 , 0);
      }
      break;
    }
    case 21 :{
      /*if(estado == 0){
        myFiles.load(195, pos_y, 60, 60, "cerrar_ojos.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(195, pos_y, 60, 60, "selec_cerrar_ojos.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(195, pos_y, 60, 60, "eje_cerrar_ojos.RAW", 1 , 0);
      }*/
      break;
    }
    case 22 :{
      /*if(estado == 0){
        myFiles.load(195, pos_y, 60, 60, "pestanar.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(195, pos_y, 60, 60, "selec_pestanar.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(195, pos_y, 60, 60, "eje_pestanar.RAW", 1 , 0);
      }*/
      break;
    }
    case 23 :{
      //myFiles.load(5, 0, 310, 480, "tag_grabar_audio.RAW", 1 , 0);
      break;
    }
    case 24 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "repro_audio.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_repro_audio.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_repro_audio.RAW", 1 , 0);
      }*/
      break;
    }
    case 25 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 26 :{
      if(estado == 0){
        myFiles.load(195, pos_y, 60, 60, "pestanar.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(195, pos_y, 60, 60, "selec_pestanar.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(195, pos_y, 60, 60, "eje_pestanar.RAW", 1 , 0);
      }
      break;
    }
    case 27 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "repro_audio.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_repro_audio.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_repro_audio.RAW", 1 , 0);
      }*/
      break;
    }
    case 28 :{
      if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "repro_audio.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_repro_audio.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_repro_audio.RAW", 1 , 0);
      }
      break;
    }
    case 29 :{
      if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }
      break;
    }
    case 30 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 31 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 32 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 33 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 34 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 35 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 36 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }
    case 37 :{
      /*if(estado == 0){
        myFiles.load(260, pos_y, 60, 60, "emitir_sonido.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(260, pos_y, 60, 60, "selec_emitir_sonido.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(260, pos_y, 60, 60, "eje_emitir_sonido.RAW", 1 , 0);
      }*/
      break;
    }    
  }
}
/*----------------------------------------------------------------------------------------------------------------------*/


/*-----------------MUESTRA LA IMAGEN EN GRANDE DE TARJETA EN LA PANTALLA----------------*/
void imprimir_imagen_tarjeta(int indice){

  switch(indice){
    case 0:{
      myFiles.load(5, 0, 310, 480, "tag_sincro.RAW", 1 , 0);
      break;
    }
    case 1 :{
      myFiles.load(5, 0, 310, 480, "tag_volver_comenzar.RAW", 1 , 0);
      break;
    }
    case 2 :{
      myFiles.load(5, 0, 310, 480, "tag_comenzar_programa.RAW", 1 , 0);
      break;
    }
    case 3 :{
      myFiles.load(5, 0, 310, 480, "tag_finalizar_programa.RAW", 1 , 0);
      break;
    }
    case 4 :{
      myFiles.load(5, 0, 310, 480, "tag_borrar_inst.RAW", 1 , 0);
      break;
    }
    case 5 :{
      myFiles.load(5, 0, 310, 480, "tag_eje_pro.RAW", 1 , 0);
      break;
    }
    case 6 :{
      myFiles.load(5, 0, 310, 480, "tag_reset_pro.RAW", 1 , 0);
      break;
    }
    case 7 :{
      myFiles.load(5, 0, 310, 480, "tag_grabar_inst.RAW", 1 , 0);
      break;
    }
    case 8 :{
      myFiles.load(5, 0, 310, 480, "tag_mover_cab_izq.RAW", 1 , 0);
      break;
    }
    case 9 :{
      myFiles.load(5, 0, 310, 480, "tag_mover_cab_der.RAW", 1 , 0);
      break;
    }
    case 10 :{
      myFiles.load(5, 0, 310, 480, "tag_agitar_cola.RAW", 1 , 0);
      break;
    }
    case 11 :{
      myFiles.load(5, 0, 310, 480, "tag_avanzar.RAW", 1 , 0);
      break;
    }
    case 12 :{
      myFiles.load(5, 0, 310, 480, "tag_girar_der.RAW", 1 , 0);
      break;
    }
    case 13 :{
      myFiles.load(5, 0, 310, 480, "tag_girar_sobre_si.RAW", 1 , 0);
      break;
    }
    case 14 :{
      myFiles.load(5, 0, 310, 480, "tag_volver_der.RAW", 1 , 0);
      break;
    }
    case 15 :{
      myFiles.load(5, 0, 310, 480, "tag_retroceder.RAW", 1 , 0);
      break;
    }
    case 16 :{
      myFiles.load(5, 0, 310, 480, "tag_girar_izq.RAW", 1 , 0);
      break;
    }
    case 17 :{
      myFiles.load(5, 0, 310, 480, "tag_evitar_obs.RAW", 1 , 0);
      break;
    }
    case 18 :{
      myFiles.load(5, 0, 310, 480, "tag_volver_izq.RAW", 1 , 0);
      break;
    }
    case 19 :{
      myFiles.load(5, 0, 310, 480, "tag_encender_leds.RAW", 1 , 0);
      break;
    }
    case 20 :{
      myFiles.load(5, 0, 310, 480, "tag_cerrar_ojos.RAW", 1 , 0);
      break;
    }
    case 21 :{
      //myFiles.load(5, 0, 310, 480, "tag_cerrar_ojos.RAW", 1 , 0); //MIRAR SORPRENDIDO
      break;
    }
    case 22 :{
      //myFiles.load(5, 0, 310, 480, "tag_pestanar.RAW", 1 , 0); //MIRAR FELIZ
      break;
    }
    case 23 :{
      //myFiles.load(5, 0, 310, 480, "tag_grabar_audio.RAW", 1 , 0); //MIRAR TRISTE
      break;
    }
    case 24 :{
      //myFiles.load(5, 0, 310, 480, "tag_repro_audio.RAW", 1 , 0); //MIRAR ENOJADO
      break;
    }
    case 25 :{
      //myFiles.load(5, 0, 310, 480, "tag_emitir_sonido.RAW", 1 , 0); //MIRAR ENAMORADO
      break;
    }
    case 26 :{
      myFiles.load(5, 0, 310, 480, "tag_pestanar.RAW", 1 , 0);
      break;
    }
    case 27 :{
      myFiles.load(5, 0, 310, 480, "tag_grabar_audio.RAW", 1 , 0);
      //myFiles.load(5, 0, 310, 480, "luces_naranjas.RAW", 1 , 0);
      break;
    }
    case 28 :{
      myFiles.load(5, 0, 310, 480, "tag_repro_audio.RAW", 1 , 0);
      //myFiles.load(5, 0, 310, 480, "luces_amarillas.RAW", 1 , 0);
      break;
    }
    case 29 :{
      myFiles.load(5, 0, 310, 480, "tag_emitir_sonido.RAW", 1 , 0);
      //myFiles.load(5, 0, 310, 480, "luces_rojas.RAW", 1 , 0);
      break;
    }
    case 30 :{
      //myFiles.load(5, 0, 310, 480, "luces_azules.RAW", 1 , 0); //COLOR BLANCO
      break;
    }
    case 31 :{
      //myFiles.load(5, 0, 310, 480, "luces_moradas.RAW", 1 , 0); //COLOR NARANJA
      break;
    }
    case 32 :{
      //myFiles.load(5, 0, 310, 480, "luces_verdes.RAW", 1 , 0); //COLOR AMARILLO
      break;
    }
    case 33 :{ 
      //myFiles.load(5, 0, 310, 480, "luces_rosadas.RAW", 1 , 0); //COLOR ROJO
      break;
    }
    case 34 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_siguiente_bloque.RAW", 1 , 0); //COLOR AZUL
      break;
    }
    case 35 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //COLOR MORADO
      break;
    } 
    case 36 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //COLOR VERDE
      break;
    }  
    case 37 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //COLOR ROSA
      break;
    }  
    case 38 :{ 
      myFiles.load(5, 0, 310, 480, "tag_ir_siguiente_bloque.RAW", 1 , 0);
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0);
      break;
    }  
    case 39 :{ 
      myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0);
      break;
    }  
    case 40 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //SELECCIONAR COLUM 0
      break;
    }  
    case 41 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //SELECCIONAR COLUM 1
      break;
    }  
    case 42 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //SELECCIONAR COLUM 2
      break;
    }  
    case 43 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //SELECCIONAR COLUM 3
      break;
    }  
    case 44 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //SELECCIONAR COLUM 4
      break;
    }  
    case 45 :{ 
      //myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0); //BORRAR BLOQUE INSTRUCCIONES
      break;
    }    
  }
  unsigned long tiempo_ahora = 0;
  
  tiempo_ahora = millis();   //RETRASO DE 1250MS
  while(millis() < tiempo_ahora + 500 );
  
}
/*--------------------------------------------------------------------------------*/


/*------IMPRIME LA MATRIZ DE FUNCIONALIDADES A EJECUTAR DADO EL BLOQUE CORRESPONDIENTE-----*/
void imprimir_pantalla_matriz_funcionalidades(int bloque){
 
  myGLCD.clrScr();

  switch(bloque){
    case 0: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_0_1.RAW", 1 , 0);
      break;
    }
    case 1: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_1_1.RAW", 1 , 0);
      break;
    }
    case 2: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_2_1.RAW", 1 , 0);
      break;
    }
    case 3: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_3_1.RAW", 1 , 0);
      break;
    }
    case 4: {
      myFiles.load(2, 5, 317, 53, "titulo_bloque_4_1.RAW", 1 , 0);
      break;
    }
  }
  
  for(int columna = 0; columna < 5; columna++){
    int cord_y = 0;
    for(int fila = 0; fila < 6; fila++){
      cord_y = cord_y + 65;
      imprimir_icono_funcionalidad(0, memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
  }

  if(num_bloques > bloque ){ //si verificaste que hay un bloque despues
    myFiles.load(218, 455, 97, 25, "f_siguiente.RAW", 1 , 0);
  }

  if(num_bloques > 0 && bloque != 0 ){ //si verificaste que hay un bloque antes
    myFiles.load(5, 455, 99, 25, "f_anterior.RAW", 1 , 0);
  }
 
}
/*------------------------------------------------------------------------------------*/

/******************************************************************************************************************************************/


/**************FUNCIONALIDADES ASOCIADAS AL SONIDO***************/

/*-----INICIALIZA EL MODULO DE SONIDO-----*/
void inicializar_sonido(){
  DFP.begin(9600);

  if(!MP3.begin(DFP)){
    Serial.println(F("ERROR!!! DFPlayer"));
    while(true){
      delay(0);
    }
  }

  Serial.println(F("DFPlayer en linea :D"));
  MP3.volume(30);
}
/*-----------------------------------------*/


/*----EMITE SONIDO DADA LA VARIABLE QUE INDICA SI SE HA LLEGADO AL FIN DE LA EJECUCION DE LA INSTRUCCION---*/
void emitir_sonido(bool fin_eje_inst){

  if(!fin_eje_inst){
    MP3.play(1); 
  }else{
    MP3.pause();   
  }
}
/*--------------------------------------------------------------------------------------------------------*/


/*********************************************************************************************************/



/********************FUNCIONALIDADES ASOCIADAS A LAS LUCES LEDS*******************************************/

/*----ENCIENDE O APAGA LAS LUCES LEDS DEL ROBOT DADA LA VARIABLE QUE INDICA SI HA LLEGADO EL TIEMPO DE FINALIZACION DE LA INSTRUCCION-----*/
void encender_luces(bool fin_eje_inst, char *UID){
  Serial.println("ENTRA A ENCENDER LUCES");
  if(!fin_eje_inst){
    uint8_t R = 0, G = 0, B = 0;
    if(strcmp(UID, "E3 F3 F0 94") == 0){ //BLANCO
      Serial.println("ENTRA A ENCENDER LUCES BLANCAS");
      R = colores[0]->R;
      G = colores[0]->G;
      B = colores[0]->B;
    }
    if(strcmp(UID, "73 4B AA 94") == 0){ //NARANJA
      R = colores[1]->R;
      G = colores[1]->G;
      B = colores[1]->B;
      Serial.println("ENTRA A ENCENDER NARANJAS");
    }
    if(strcmp(UID, "83 9D 6D 94") == 0){ //AMARILLO
      R = colores[2]->R;
      G = colores[2]->G;
      B = colores[2]->B;
      Serial.println("ENTRA A ENCENDER LUCES AMARILLAS");
    }
    if(strcmp(UID, "E3 D3 6B 12") == 0){ //ROJO
      R = colores[3]->R;
      G = colores[3]->G;
      B = colores[3]->B;
      Serial.println("ENTRA A ENCENDER ROJAS");
    }
    if(strcmp(UID, "B3 05 6E 12") == 0){ //AZUL
      R = colores[4]->R;
      G = colores[4]->G;
      B = colores[4]->B;
      Serial.println("ENTRA A ENCENDER LUCES AZULES");
    }
    if(strcmp(UID, "F3 43 C6 12") == 0){ //MORADO
      R = colores[5]->R;
      G = colores[5]->G;
      B = colores[5]->B;
      Serial.println("ENTRA A ENCENDER LUCES  MORADAS");
    }
    if(strcmp(UID, "93 F1 C7 12") == 0){ //VERDE
      R = colores[6]->R;
      G = colores[6]->G;
      B = colores[6]->B;
      Serial.println("ENTRA A ENCENDER LUCES VERDES");
    }
    if(strcmp(UID, "83 75 A9 94") == 0){ //ROSA
      R = colores[7]->R;
      G = colores[7]->G;
      B = colores[7]->B;
      Serial.println("ENTRA A ENCENDER LUCES ROSAS");
    }

    for(int i = 0; i < NUM_LEDS ; i++){
      color_led.setPixelColor(i, R,G, B);
    }
    color_led.show();
  }else{
    for(int i = 0; i < NUM_LEDS ; i++){
      color_led.setPixelColor(i, color_led.Color(0,0,0));
    }
    color_led.show();
    Serial.println("ENTRA A APAGAR LUCES");
  }  
  
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/



/*--FUNCION VACIA PARA LAS FUNCIONALIDADES ADMINISTRATIVAS--*/
void prueba(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("ACTIVANDO INSTRUCCION");
  }else{
    Serial.println("DESACTIVANDO INSTRUCCION"); 
  } 
}
/*----------------------------------------------------------*/


/*---------FUNCIONES QUE SE EJECUTAN UNA SOLA VEZ-----------*/
void setup(void) {
  Serial.begin(9600);

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif
  // END of Trinket-specific code.

  while(!Serial){
      
  }

  rueda_izq.attach(5); //SE ESTABLECE EL PIN AL QUE ESTA CONECTADO EL SERVO
  rueda_der.attach(4); //SE ESTABLECE EL PIN AL QUE ESTA CONECTADO EL SERVO
  cola.attach(6);
  cabeza.attach(7);
  crear_arreglo_funcionalidades();  
  crear_memoria_instrucciones();
  inicializar_memoria();
  nfc.begin();
  inicializar_sonido(); 
  inicializar_colores_led();
  color_led.begin();
  rueda_izq.write(90);
  rueda_der.write(90);
  cola.write(90);
  cabeza.write(90);
  inicializar_pantalla_tft();
  inicializar_ojos();

}
/*---------------------------------------------------------*/


/**-------------------------------------------------------------------------------------PROGRAMA PRINCIPAL---------------------------------------------------------------------------------------------------*/
void loop(void) {

  unsigned long tiempo_eje_programa = 0, tiempo_inicio_bloque = 0, tiempo_fin_bloque = 0, tiempo_eje_bloque = 0 ; 

  unsigned long tiempo_inicio_col_0 = 0, tiempo_inicio_col_1 = 0, tiempo_inicio_col_2 = 0,tiempo_inicio_col_3 = 0,tiempo_inicio_col_4 = 0; //TIEMPOS DE INICIO DE CADA COLUMNA DE LA MATRIZ MEMORIA

  unsigned long tiempo_fin_col_0 = 0, tiempo_fin_col_1 = 0, tiempo_fin_col_2 = 0,tiempo_fin_col_3 = 0,tiempo_fin_col_4 = 0; //TIEMPOS DE FINALIZACION DE CADA COLUMNA DE LA MATRIZ MEMORIA

  unsigned long  tiempo_eje_colum_0 = 0, tiempo_eje_colum_1 = 0, tiempo_eje_colum_2 = 0, tiempo_eje_colum_3 = 0, tiempo_eje_colum_4 = 0; //TIEMPOS DE EJECUCION DE CADA COLUMNA DE LA MATRIZ MEMORIA

  unsigned long tiempo_inicio_inst_col_0 = 0, tiempo_inicio_inst_col_1 = 0, tiempo_inicio_inst_col_2 = 0,tiempo_inicio_inst_col_3 = 0,tiempo_inicio_inst_col_4 = 0;  //TIEMPO DE INICIO DE LA INSTRUCCION ACTUAL EJECUTANDOSE DE CADA COLUMNA

  bool tiempo_exed_col_0 = false,  tiempo_exed_col_1 = false, tiempo_exed_col_2 = false ,tiempo_exed_col_3 = false ,tiempo_exed_col_4 = false; //BANDERA QUE INDICA SI EN UNA COLUMNA HAY QUE APLICAR TIEMPO EXCEDENTE
  
  bool inicio_bloque = false, ejecutada_0 = false, ejecutada_1 = false, ejecutada_2 = false, ejecutada_3 = false, ejecutada_4 = false, fin_programa = false, imprimir_pantalla = false;

  unsigned long tiempo_eje_inst_0 = 0, tiempo_eje_inst_1 = 0, tiempo_eje_inst_2 = 0, tiempo_eje_inst_3 = 0, tiempo_eje_inst_4 = 0; //TIEMPOS DE EJECUCION DE LA INSTRUCCION ACTUAL EJECUTANDOSE EN CADA COLUMNA DE LA MATRIZ MEMORIA

  int bloque = 0; //INDICA EL BLOQUE ACTUAL QUE SE ESTA EJECUTANDO DE LA MATRIZ DE MEMORIA. INCREMENTA LUEGO DE CULMINADO UN BLOQUE

  int fila_col_0 = 0, fila_col_1 = 0, fila_col_2 = 0, fila_col_3 = 0, fila_col_4 = 0; //INDICA LA FILA DE LA MATRIZ DE MEMORIA QUE ACTUALMENTE DE SE EJECUTA. INCREMENTA LUEGO DE FINALIZADA UNA INSTRUCCION

  unsigned long *tiempos_previos_ojos;

  bool *turnos_ojos;

  tiempos_previos_ojos = crear_arreglo_tiempos_ojos();
  inicializar_tiempos_ojos(tiempos_previos_ojos);
  turnos_ojos = crear_arreglo_turnos_ojos();
  inicializar_turnos_ojos(turnos_ojos);
  escanear_instrucciones(tiempos_previos_ojos, turnos_ojos);


  if(tiempos_eje_bloques == NULL){ //SI AUN NO HAZ CREADO EL ARREGLO DE BLOQUE
    tiempos_eje_bloques = crear_arreglo_tiempos_ejecucion_bloques();
  }
 
  inicializar_arreglo_tiempos_ejecucion_bloques();
  
  establecer_tiempos_eje_arreglo_tiempos_bloques(tiempo_duracion_bloque_instrucciones(0),tiempo_duracion_bloque_instrucciones(1),tiempo_duracion_bloque_instrucciones(2),tiempo_duracion_bloque_instrucciones(3),tiempo_duracion_bloque_instrucciones(4));
  
  tiempo_eje_programa = determinar_duracion_programa();

  Serial.print("*********************TIEMPO EJECUCION PROGRAMA********************");
  Serial.println(tiempo_eje_programa);
  Serial.println();
  Serial.println();
  while(volver_a_comenzar >= 0){ //REPITE HASTA QUE HAYAS COMPLETADO TODAS LAS ITERACIONES INDICADAS
    myFiles.load(5, 0, 312, 480, "programa_comenzar.RAW", 1 , 0);
    delay(1500); //ver si se reemplaza por millis
    myFiles.load(5, 0, 310, 480, "memoria_instrucciones.RAW", 1 , 0);
    delay(1000); //ver si se reemplza por millis
    Serial.print("**************ITERACION NRO: ");
    Serial.print(volver_a_comenzar);
    Serial.println("**************");
    tiempo_inicio_programa = millis();
    tiempo_fin_programa = millis();
    inicializar_turnos_ojos(turnos_ojos);
    inicializar_tiempos_ojos(tiempos_previos_ojos);

    while(!fin_programa){ //REPITE MIENTRAS EL TIEMPO TRANSCURRIDO SEA MENOR AL TIEMPO DE EJECUCION DEL PROGRAMA
      
      if (!inicio_bloque){ //SI ESTE BLOQUE AUN NO SE EJECUTA INICIALIZA TODAS LAS VARIABLES CORRESPONDIENTES
        myGLCD.clrScr();
        inicio_bloque = true;
        tiempo_fin_bloque = millis();
        tiempo_inicio_bloque = millis();
        tiempo_inicio_col_0 = tiempo_inicio_bloque;
        tiempo_inicio_col_1 = tiempo_inicio_bloque;
        tiempo_inicio_col_2 = tiempo_inicio_bloque;
        tiempo_inicio_col_3 = tiempo_inicio_bloque;
        tiempo_inicio_col_4 = tiempo_inicio_bloque;
        fila_col_0 = 0;
        fila_col_1 = 0;
        fila_col_2 = 0;
        fila_col_3 = 0;
        fila_col_4 = 0;
        tiempo_eje_colum_0 = 0;
        tiempo_eje_colum_1 = 0;
        tiempo_eje_colum_2 = 0;
        tiempo_eje_colum_3 = 0;
        tiempo_eje_colum_4 = 0;
      }

      //ESTABLECE EL TIEMPO DE EJECUCION DEL BLOQUE ACTUAL
      if(bloque == 0){
        tiempo_eje_bloque = tiempos_eje_bloques[0];
      }
      if(bloque == 1){
        tiempo_eje_bloque = tiempos_eje_bloques[1];
      }
      if(bloque == 2){
        tiempo_eje_bloque = tiempos_eje_bloques[2];
      }
      if(bloque == 3){
        tiempo_eje_bloque = tiempos_eje_bloques[3];
      }
      if(bloque == 4){
        tiempo_eje_bloque = tiempos_eje_bloques[4];
      }
      //--------------------------------------------
      //VERIFICA SI LA COLUMNA 0 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV EXTREMIDADES
      if(memoria_instrucciones[bloque][0][0] != 0){ 
        ejecutar_columna_instrucciones(&bloque,0, &fila_col_0, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_0, &tiempo_inicio_col_0, &tiempo_fin_col_0, &tiempo_exed_col_0, &tiempo_inicio_inst_col_0, &tiempo_eje_inst_0, &tiempo_eje_colum_0,tiempos_previos_ojos, turnos_ojos);
        imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 0, fila_col_0, false);
      }   
      //VERIFICA SI LA COLUMNA 1 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV CUERPO
      if(memoria_instrucciones[bloque][1][0] != 0){
        ejecutar_columna_instrucciones(&bloque,1, &fila_col_1, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_1, &tiempo_inicio_col_1, &tiempo_fin_col_1, &tiempo_exed_col_1,&tiempo_inicio_inst_col_1, &tiempo_eje_inst_1, &tiempo_eje_colum_1,tiempos_previos_ojos, turnos_ojos);
        imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 1, fila_col_1, false);
      }
      //VERIFICA SI LA COLUMNA 2 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //LUCES LEDS
      if(memoria_instrucciones[bloque][2][0] != 0){
        ejecutar_columna_instrucciones(&bloque,2, &fila_col_2, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_2, &tiempo_inicio_col_2, &tiempo_fin_col_2, &tiempo_exed_col_2,&tiempo_inicio_inst_col_2, &tiempo_eje_inst_2, &tiempo_eje_colum_2,tiempos_previos_ojos, turnos_ojos);
        imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 2, fila_col_2, false);
      }
      //VERIFICA SI LA COLUMNA 3 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV OJOS
      if(memoria_instrucciones[bloque][3][0] != 0){
        ejecutar_columna_instrucciones(&bloque,3, &fila_col_3, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_3, &tiempo_inicio_col_3, &tiempo_fin_col_3, &tiempo_exed_col_3,&tiempo_inicio_inst_col_3, &tiempo_eje_inst_3, &tiempo_eje_colum_3,tiempos_previos_ojos, turnos_ojos);
        imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 3, fila_col_3, false);
      }else{
        mover_ojos_neutros(tiempos_previos_ojos,turnos_ojos);
      }
      
      //VERIFICA SI LA COLUMNA 4 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //SONIDO
      if(memoria_instrucciones[bloque][4][0] != 0){
        ejecutar_columna_instrucciones(&bloque,4, &fila_col_4, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_4, &tiempo_inicio_col_4, &tiempo_fin_col_4, &tiempo_exed_col_4,&tiempo_inicio_inst_col_4, &tiempo_eje_inst_4, &tiempo_eje_colum_4,tiempos_previos_ojos, turnos_ojos);
        imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 4, fila_col_4, false);
      }
      
      tiempo_fin_bloque = millis();
      tiempo_fin_programa = millis();

      //VERIFICA SI HA LLEGADO EL TIEMPO DE FINALIZAR UN BLOQUE DE INSTRUCCIONES
      if(tiempo_fin_bloque - tiempo_inicio_bloque >= tiempo_eje_bloque){ 
        if(!ejecutada_0 && !ejecutada_1 && !ejecutada_2 && !ejecutada_3 && !ejecutada_4){ //SI YA TODAS LAS INSTRUCCIONES HAN SIDO DESACTIVADAS PROCEDE A CULMINAR EL BLOQUE
          inicio_bloque = false;
          if(bloque < sincronizacion){
            bloque++;
          }
          
          unsigned long tiempo_ahora = 0;
  
          tiempo_ahora = millis();   //RETRASO DE 1250MS
          while(millis() < tiempo_ahora + 500 );

          myGLCD.clrScr();
        }
      }
 
      //VERIFICA SI HA LLEGADO EL TIEMPO DE CULMINAR EL PROGRAMA
      if((tiempo_fin_programa - tiempo_inicio_programa >= tiempo_eje_programa) && !inicio_bloque){
        fin_programa = true; 
        myFiles.load(5, 0, 310, 480, "programa_finalizado.RAW", 1 , 0);
        delay(1500); //ver si se reemplza por millis
        unsigned long tiempo_ahora = 0;
  
        tiempo_ahora = millis();   //RETRASO DE 1500MS
        while(millis() < tiempo_ahora + 1500 );
      }
    } 

    //SE INICIALIZAN LAS VARIABLES PERTINENTES PARA VOLVER A REALIZAR UNA ITERACION DEL PROGRAMA EN CASO DE SER REQUERIRLO
    fin_programa = false;
    bloque = 0;
    volver_a_comenzar--;
  }

  Serial.println("/*-------NUEVO ESCANEO-----*/");

  //SE INICIALIZAN LAS VARIABLES PERTINENTES PARA REALIZAR UNA NUEVA ESCRITURA DE PROGRAMA
  grabacion = false;
  volver_a_comenzar = 0;
  comenzar_programa = false;
  finalizar_programa = false;
  sincronizacion = 0;
  inicializar_memoria();
  ejecutar_programa = false;
}
/*------------------------------------------*/



