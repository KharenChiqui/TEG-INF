#include "DFRobotDFPlayerMini.h"
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
int luces = 0, num_bloques = 0;
int color_reg = 0;
int *memoria_colores_luces = NULL;
unsigned long tiempo_inicio_programa = 0, tiempo_fin_programa = 0;
/*-----------------------------------------------------------------*/



/*-----------------------------------------------------------------FUNCIONES DE LOGICA DE INSTRUCCIONES Y SU ALMACENAMIENTO--------------------------------------------------------------*/

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

return nueva_func;}
/*-------------------------------------------------------------------------------------------------*/


/*---CREA UN ARREGLO CON LAS FUNCIONALIDADES ESTABLECIDAS Y LO INICIALIZA CON SUS RESPECTIVOS VALORES--*/
void crear_arreglo_funcionalidades(){
  void *p = NULL ;

  if ((funcionalidades =(Funcionalidad **) malloc(sizeof (Funcionalidad *) * 42)) == NULL){
	  Serial.println("funcionalidades: error en el malloc\n");
		exit(1);
  }

  funcionalidades[0] = crear_nueva_funcionalidad("B3 54 7A 12", &prueba, 0, NULL,0); //SINCRONIZACION
  funcionalidades[1] = crear_nueva_funcionalidad("E3 F7 A7 12", &prueba, 0, NULL,0); //VOLVER A COMENZAR
  funcionalidades[2] = crear_nueva_funcionalidad("A3 CE 89 94", &prueba, 0, NULL,0); //COMENZAR PROGRAMA
  funcionalidades[3] = crear_nueva_funcionalidad("43 26 C4 12", &prueba, 0, NULL,0); //FINALIZAR PROGRAMA
  funcionalidades[4] = crear_nueva_funcionalidad("A3 3E 72 94", &prueba, 0, NULL,0); //BORRAR INSTRUCCION
  funcionalidades[5] = crear_nueva_funcionalidad("E3 FC B3 12", &prueba, 0, NULL,0); //EJECUTAR PROGRAMA
  funcionalidades[6] = crear_nueva_funcionalidad("93 02 86 94", &prueba, 0, NULL,0); //PAUSAR
  funcionalidades[7] = crear_nueva_funcionalidad("13 7C 72 94", &prueba, 0, NULL,0); //INSTRUCCIONES POR COMANDO DE VOZ (CORREGIR UID)
  funcionalidades[8] = crear_nueva_funcionalidad("53 12 73 94", &prueba, 3, NULL,3000); //MOVER CABEZA A LA IZQUIERDA
  funcionalidades[9] = crear_nueva_funcionalidad("F3 94 8B 94", &prueba, 3, NULL,3000); //MOVER CABEZA A LA DERECHA
  funcionalidades[10] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL,5000); //AGITAR COLA
  funcionalidades[11] = crear_nueva_funcionalidad("83 0E AA 12", &avanzar, 4, NULL,5000); //AVANZAR
  funcionalidades[12] = crear_nueva_funcionalidad("13 45 8A 94", &girar_derecha, 4, NULL, 1350); //GIRAR A LA DERECHA
  funcionalidades[13] = crear_nueva_funcionalidad("D3 DF 81 94", &girar_sobre_si_mismo, 4, NULL,5400); //GIRAR SOBRE SI MISMO
  funcionalidades[14] = crear_nueva_funcionalidad("B3 23 8F 94", &volver_derecha, 4, NULL,2700); //VOLVER POR LA DERECHA
  funcionalidades[15] = crear_nueva_funcionalidad("13 63 6B 94", &retroceder, 4, NULL,5000); //RETROCEDER
  funcionalidades[16] = crear_nueva_funcionalidad("83 11 6B 94", &girar_izquierda, 4, NULL, 1350); //GIRAR A LA IZQUIERDA
  funcionalidades[17] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL,15000); //EVITAR OBSTACULOS
  funcionalidades[18] = crear_nueva_funcionalidad("93 E2 20 95", &volver_izquierda, 4, NULL, 2700); //VOLVER POR LA IZQUIERDA
  funcionalidades[19] = crear_nueva_funcionalidad("23 DE 6C 94", &encender_luces, 5, NULL, 5000); //ENCENDER LUCES
  funcionalidades[20] = crear_nueva_funcionalidad("F3 A9 89 94", &prueba, 6, NULL, 2000); //ABRIR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("93 D3 80 94", &prueba, 6, NULL, 2000); //CERRAR OJOS
  funcionalidades[22] = crear_nueva_funcionalidad("63 36 C6 94", &prueba, 6, NULL, 4000); //PESTANEAR
  funcionalidades[23] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL,10000); //GRABAR AUDIO
  funcionalidades[24] = crear_nueva_funcionalidad("53 2D 89 94", &prueba,7, NULL,10000); //REPRODUCIR GRABACION
  funcionalidades[25] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido,7, NULL,10000); //EMITIR SONIDO
  funcionalidades[26] = crear_nueva_funcionalidad("E3 F3 F0 94", &prueba, 1, NULL, 5000); //BLANCO
  funcionalidades[27] = crear_nueva_funcionalidad("73 4B AA 94", &prueba, 1, NULL, 5000); //NARANJA
  funcionalidades[28] = crear_nueva_funcionalidad("83 9D 6D 94", &prueba, 1, NULL, 5000); //AMARILLO
  funcionalidades[29] = crear_nueva_funcionalidad("E3 D3 6B 12", &prueba, 1, NULL, 5000); //ROJO
  funcionalidades[30] = crear_nueva_funcionalidad("B3 05 6E 12", &prueba, 1, NULL, 5000); //AZUL
  funcionalidades[31] = crear_nueva_funcionalidad("F3 43 C6 12", &prueba, 1, NULL, 5000); //MORADO
  funcionalidades[32] = crear_nueva_funcionalidad("93 F1 C7 12", &prueba, 1, NULL, 5000); //VERDE
  funcionalidades[33] = crear_nueva_funcionalidad("83 75 A9 94", &prueba, 1, NULL, 5000); //ROSA
  funcionalidades[34] = crear_nueva_funcionalidad("A3 D2 B7 12", &prueba, 2, NULL, 0); //SIGUIENTE BLOQUE
  funcionalidades[35] = crear_nueva_funcionalidad("E3 00 69 12", &prueba, 2, NULL, 0); //ANTERIOR BLOQUE
  funcionalidades[36] = crear_nueva_funcionalidad("43 FB 27 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 0
  funcionalidades[37] = crear_nueva_funcionalidad("D3 60 39 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 1
  funcionalidades[38] = crear_nueva_funcionalidad("33 23 98 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 2
  funcionalidades[39] = crear_nueva_funcionalidad("53 3F 92 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 3
  funcionalidades[40] = crear_nueva_funcionalidad("E3 16 67 A7", &prueba, 2, NULL, 0); //SELECCIONAR COLUMNA 4
  funcionalidades[41] = crear_nueva_funcionalidad("E3 DC 14 A7", &prueba, 0, NULL, 0); //BORRAR BLOQUE INSTRUCCIONES
}
/*-------------------------------------------------------------------------------------*/


/*-CREA UN ARREGLO  DONDE SE ALMACENARAN LOS INDICES DE LAS FUNCIONALIDADES A EJECUTAR EN CADA BLOQUE-----*/
int crear_memoria_instrucciones(){

  if (( memoria_instrucciones = (int ***) malloc(5 * sizeof (int **))) == NULL){ //SE CREAN LAS COLUMNAS DE CADA TIPO DE INSTRUCCION
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
 for(int colum = 0; colum<5; colum++){  
  memoria_instrucciones[colum] = (int **) malloc(sizeof(int *) * 6); //SE CREAN LAS FILAS DE CADA COLUMNA DE INSTRUCCIONES
 }

  for(int colum = 0; colum < 5; colum++){
    for(int filas = 0 ; filas <6; filas++) //SE CREAN LOS BLOQUES DE INSTRUCCIONES
    {
      memoria_instrucciones[colum][filas] = (int *) malloc(sizeof(int) * 5); 
    }
  }
}
/*-----------------------------------------------------------------------------------------------------*/


/*------INICIALIZA EL ARREGLO(MEMORIA) QUE SE ENCARGA DE ALMACENAR LAS FUNCIONALIDADES A EJECUTAR EN EL PROGRAMA----*/
void inicializar_memoria(){

  for( int dim = 0 ; dim < 5 ; dim++){
    for(int filas = 0 ; filas < 6 ; filas++){
      for( int colum = 0 ; colum < 5 ; colum++){
        memoria_instrucciones[dim][filas][colum] = 0;
      }   
    }  
  }
}
/*----------------------------------------------------------------------------------------------------------*/


/*---IMPRIME LA MEMORIA DE INSTRUCCIONES A EJECUTAR EN EL PROGRAMA EN EL SERIAL MONITOR---*/
void imprimir_matriz(){

  for( int dim = 0 ; dim<5 ; dim++){
    Serial.print("/----DIMENSION----/: ");
    Serial.println(dim,1);
    for(int filas = 0 ; filas<6 ; filas++){
      Serial.print("\n");
      for( int colum = 0 ; colum<5 ; colum++){
        Serial.print("\t");
        Serial.print( memoria_instrucciones[dim][filas][colum],1);
        Serial.print("\t");
      }
    }
    Serial.println();
  }
}
/*------------------------------------------------------------------------------------*/

int eliminar_bloque_instrucciones(int bloque){
  int ***memoria_auxiliar = NULL;

  if(verificar_bloque_instrucciones_vacio(bloque)){
    Serial.println("ESTE BLOQUE ESTA VACIO");
    return 0;
  }

  if (( memoria_auxiliar = (int ***) malloc(5 * sizeof (int **))) == NULL){ //SE CREAN LAS COLUMNAS DE CADA TIPO DE INSTRUCCION
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
  for(int colum = 0; colum<5; colum++){  
    memoria_auxiliar[colum] = (int **) malloc(sizeof(int *) * 6); //SE CREAN LAS FILAS DE CADA COLUMNA DE INSTRUCCIONES
  }

  for(int colum = 0; colum < 5; colum++){
    for(int filas = 0 ; filas <6; filas++) //SE CREAN LOS BLOQUES DE INSTRUCCIONES
    {
      memoria_auxiliar[colum][filas] = (int *) malloc(sizeof(int) * 5); 
    }
  }

  for( int dim = 0 ; dim < 5 ; dim++){
    for(int filas = 0 ; filas < 6 ; filas++){
      for( int colum = 0 ; colum < 5 ; colum++){
        memoria_auxiliar[dim][filas][colum] = 0;
      }   
    }  
  }

  for( int dim = 0, dim_aux = 0 ; dim<5 ; dim++){
    if(dim != bloque){
      for(int colum = 0 ; colum<5 ; colum++){
        for( int filas = 0 ; filas<6 ; filas++){ 
          memoria_auxiliar[dim][filas][colum] = memoria_instrucciones[dim_aux][filas][colum];
        }
      }
      dim_aux++;
    }
  }

  for( int dim = 0 ; dim<5 ; dim++){
    for(int colum = 0 ; colum<5 ; colum++){
      for( int filas = 0 ; filas<6 ; filas++){ 
        memoria_instrucciones[dim][filas][colum] = memoria_auxiliar[dim][filas][colum];
      }
    } 
  }
  
  Serial.println("LIBERANDO MEMORIA");

  for(int colum = 0; colum < 5; colum++){
    for(int filas = 0 ; filas <6; filas++) //SE CREAN LOS BLOQUES DE INSTRUCCIONES
    {
      free(memoria_auxiliar[colum][filas]);
    }
  }

  for(int colum = 0; colum<5; colum++){  
    free(memoria_auxiliar[colum]); //SE CREAN LAS FILAS DE CADA COLUMNA DE INSTRUCCIONES
  }  

  free(memoria_auxiliar);

  Serial.println("FIN DE LIBERACION MEMORIA");

  Serial.println("/*----IMPRIMIENDO MATRIZ-----*/");
  imprimir_matriz();
  Serial.println("/*----FIN IMPRIMIR MATRIZ-----*/");
}

int verificar_instruccion_seleccionada(char *UID, char *UID_aux, int *selec_col, int colum, bool *cursor){

  if((strcmp(UID,UID_aux) == 0)){
    *cursor = true;
    Serial.println("ENTRA A VERIFICAR COLUMNA");
    if(memoria_instrucciones[sincronizacion][0][colum] == 0){
       Serial.println("VACIOO 1");
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      return 1;
    }
    (*selec_col)++;

    if(memoria_instrucciones[sincronizacion][*selec_col][colum] != 0){
      Serial.println("NO ESTA VACIA ESTA FILA");
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      imprimir_funcion_columna_matriz_funcionalidades(sincronizacion, colum , *selec_col, true); //SELECCIONA EL ELEMENTO INDICADO
      return 1;
    }else{
      Serial.print("ESTA VACIA LA FILA");
      *selec_col = -1;
      imprimir_pantalla_matriz_funcionalidades(sincronizacion);
      return 1;
    }
  }
return 0;}


void eliminar_instruccion(int colum, int fila, int bloque){
  int *columna_auxiliar = NULL;

  columna_auxiliar = (int) malloc(sizeof(int) * 6);

  for(int i = 0; i < 6; i++){
    columna_auxiliar[i] = 0;
  }
  
  for(int i = 0, j = 0; i < 6; i++){
    if(i != fila){
      columna_auxiliar[j] = memoria_instrucciones[bloque][i][colum];
      j++;
    }
  }

  for(int i = 0; i < 6; i++){
    Serial.print(columna_auxiliar[i]);
    Serial.print(" ");
  }
  Serial.println();

  for(int i = 0; i < 6; i++){
    memoria_instrucciones[bloque][i][colum] = columna_auxiliar[i];
  }

  free(columna_auxiliar);

  Serial.println("ELIMINADA INSTRUCCION");
}
/*------------ALMACENA UNA INSTRUCCION EN LA MEMORIA DE INSTRUCCIONES DADO SU UID--------*/
int almacenar_instruccion(char *UID,  int *selec_col_0, int *selec_col_1, int *selec_col_2, int *selec_col_3, int *selec_col_4){
  char  *UID_aux = NULL;
  int tipo_inst = -1;
  bool cursor = false;
   //Serial.print("-------UID-----");
  //Serial.println(UID);

  for(int i = 0; i <= 41 ; i++){ //REVISA TODAS LAS FUNCIONALIDADES
    UID_aux = NULL;
    UID_aux = funcionalidades[i]->UID;

    if(strcmp(UID,UID_aux) == 0){  //SI ENCUENTRA LA INSTRUCCION
      imprimir_imagen_tarjeta(i);
      tipo_inst = funcionalidades[i]->type;

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

      if((strcmp(UID,"23 DE 6C 94") == 0)){ //SI SE HA ESCANEADO TAG ENCENDER LUCES
        luces++;
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

      verificar_color_tarjeta(UID, "E3 F3 F0 94", 0 , &tipo_inst);
      verificar_color_tarjeta(UID, "73 4B AA 94", 1 , &tipo_inst);
      verificar_color_tarjeta(UID, "83 9D 6D 94", 2 , &tipo_inst);
      verificar_color_tarjeta(UID, "E3 D3 6B 12", 3 , &tipo_inst);
      verificar_color_tarjeta(UID, "B3 05 6E 12", 4 , &tipo_inst);
      verificar_color_tarjeta(UID, "F3 43 C6 12", 5 , &tipo_inst);
      verificar_color_tarjeta(UID, "93 F1 C7 12", 6 , &tipo_inst);
      verificar_color_tarjeta(UID, "83 75 A9 94", 7 , &tipo_inst);
   
      //si no encontraste que le indicaron un color a las luces ponte en modo semaforo
      if((strcmp(UID,"A3 D2 B7 12") == 0) && num_bloques > sincronizacion ){ //si verificaste que hay un bloque despues
        sincronizacion++;
      }

      if((strcmp(UID,"E3 00 69 12") == 0) && num_bloques > 0 ){ //si verificaste que hay un bloque antes
        sincronizacion--;
      }

      if((strcmp(UID,"A3 3E 72 94") == 0)){ //SI SE HA ESCANEADO BORRAR ISNTRUCCION

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

      if((strcmp(UID,"E3 DC 14 A7") == 0) ){
        if(eliminar_bloque_instrucciones(sincronizacion) == 1){//si esta vacio no hagas nada
          Serial.println("BLOQUE ELIMINADO");
          num_bloques--;
          sincronizacion = 0;
        }
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


/*---INTRODUCE EL INDICE DE UNA FUNCIONALIDAD EN LA MEMORIA DE INSTRUCCIONES DADO SU INDICE, LA COLUMNA Y EL BLOQUE DONDE SERA INTRODUCIDA--------*/
void introducir_inst_columna_memoria(int indice_func, int colum, int dim){
  bool almacenado = false;

  for( int j = 0 ; j<8 ; j++){
    if(memoria_instrucciones[dim][j][colum] == 0){
      almacenado = true;
      memoria_instrucciones[dim][j][colum] = indice_func;
      return;
    } 
  }   
}
/*----------------------------------------------------------------------------------------------------------------------------------*/


/*-----------------------------ESCANEA LAS TARJETAS-----------------------------------------------*/
void escanear_instrucciones(){
  unsigned long tiempo_ahora = 0, tiempo_previo_0 = 0,tiempo_previo_1 = 0, tiempo_previo_2 = 0, tiempo_previo_3 = 0;
  int selec_col_0 = -1, selec_col_1 = -1, selec_col_2 = -1, selec_col_3 = -1, selec_col_4 = -1;
  bool ojo_n_0 = true, ojo_n_1 = false, ojo_n_2 = false, ojo_n_3 = false;
  Serial.println("escaneando tags");
  myFiles.load(5, 0, 310, 480, "escanear_tarjeta.RAW", 1 , 0);
 
  while(!ejecutar_programa){

    if((millis() - tiempo_previo_0 >= 3000) && ojo_n_0 ){
      ojos_neutros_pos_0();
      ojo_n_0 = false;
      ojo_n_1 = true;
      tiempo_previo_1 = millis();
      Serial.println("OJOS 0");
    }
  
    if((millis() - tiempo_previo_1 >= 3000) && ojo_n_1){
      ojos_neutros_pos_1();
      ojo_n_1 = false;
      ojo_n_2 = true;
      tiempo_previo_2 = millis();
      Serial.println("OJOS 1");
    }

    if((millis() - tiempo_previo_2 >= 3000) && ojo_n_2){
      ojos_neutros_pos_2();
      ojo_n_2 = false;
      ojo_n_3 = true;
      tiempo_previo_3 = millis();
      Serial.println("OJOS 2");
    }

    if((millis() - tiempo_previo_3 >= 3000) && ojo_n_3){
      ojos_neutros_pos_3();
      ojo_n_3 = false;
      ojo_n_0 = true;
      tiempo_previo_0 = millis();
      Serial.println("OJOS 3");
    }

   Serial.println("Vamos a escanear las tags");
    if (nfc.tagPresent()){
      Serial.println("encontro una tag");
      MP3.play(6);
      tiempo_ahora = millis();   //RETRASO DE 1250MS
       myFiles.load(5, 0, 310, 480, "tarjeta_escaneada.RAW", 1 , 0);
      while(millis() < tiempo_ahora + 500 );

      NfcTag tag = nfc.read();
      String TagUID = tag.getUidString();
      char *ptrUID = NULL;
      ptrUID = new char[TagUID.length() + 1];
      strcpy(ptrUID, TagUID.c_str());
      Serial.println(ptrUID);
  
      if((strcmp(ptrUID,"13 7C 72 94" ) == 0) && !finalizar_programa && comenzar_programa ){ //SI SE HA ESCANEADO TAG COMANDO VOZ Y ESTA AUN NO HA SIDO ESCANEADA PERO YA SE COMENZO LA ESCRITURA DE INSTRUCCIONES FINALIZALA
        myFiles.load(5, 0, 310, 480, "comando_voz.RAW", 1 , 0);
        //llamar a funcion 
       // delay(2000);
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
        if (almacenar_instruccion(ptrUID, &selec_col_0, &selec_col_1, &selec_col_2, &selec_col_3, &selec_col_4)){
          //imprimir_matriz();
        }
      }
      if((strcmp(ptrUID,"E3 FC B3 12") == 0) && comenzar_programa && finalizar_programa){ //SI SE HA ESCANEADO TAG EJECUTAR PROGRAMA Y YA SE REALIZO LA ESCRITURA CORRESPONDIENTE DE INSTRUCCIONES
        ejecutar_programa = true; 
        comenzar_programa = false;
        finalizar_programa = false;
      }

      imprimir_matriz();

      tiempo_ahora = millis();   //RETRASO DE 200MS
    
      while(millis() < tiempo_ahora + 200 );    
    }
  }
}
/*--------------------------------------------------------------------------------------------*/



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


/*--VERIFICA CUAL COLOR HA SIDO SELECCIONADO Y AGREGA SU INDICE A LA MEMORIA DE LUCES DE COLORES EN CASO DE SER IGUALES LOS UID---*/
void verificar_color_tarjeta(char *UID, char *UID_aux, int pos_color, int *tipo_inst){
  
  if((strcmp(UID,UID_aux) == 0) && (luces >= 0) && (luces > color_reg)){ 
    *tipo_inst = 0;
    color_reg++;
    agregar_color_luces_memoria(pos_color);
  }
}
/*---------------------------------------------------------------------------------------------------------------------------------*/


/*---VERIFICA SI UN BLOQUE TIENE INSTRUCCIONES ALMACENADAS. DEVUELVE TRUE EN CASO DE QUE SE ENCUENTRE VACIO---*/
bool verificar_bloque_instrucciones_vacio(int bloque){ //devuelve 1 si se encuentra vacio, 0 si esta lleno
  int vacio = 0;

  for(int i = 0 ; i<5 ; i++){
    if(memoria_instrucciones[bloque][0][i] == 0){ //SI LO QUE ESTA EN LA PRIMERA POSICION DE LA COLUMNA ES CERO
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


/*---DETERMINA EL TIEMPO DE EJECUCION DE UNA COLUMNA DADO EL BLOQUE DE INSTRUCCIONES AL QUE PERTENECE---*/
unsigned long tiempo_eje_columna_instrucciones(int bloque, int colum){
  unsigned long duracion = 0, duracion_max = 0;
    
    for(int j = 0; j < 8; j++){
      if(memoria_instrucciones[bloque][j][colum] != 0){
        duracion += funcionalidades[memoria_instrucciones[bloque][j][colum]]->exec_time;
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

  for(int i = 0 ; i < 5; i++){
    duracion = 0;
    for(int j = 0; j < 8; j++){
      if(memoria_instrucciones[bloque][j][i] != 0){
        duracion += funcionalidades[memoria_instrucciones[bloque][j][i]]->exec_time;
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

void ejecutar_columna_instrucciones(int *bloque, int columna, int *fila, unsigned long *tiempo_inicio_bloque, unsigned long *tiempo_fin_bloque, unsigned long *tiempo_eje_bloque, bool *ejecutada, unsigned long *tiempo_inicio_col,unsigned long *tiempo_fin_col, bool *tiempo_exed_col, unsigned long *tiempo_inicio_inst, unsigned long *tiempo_inst, unsigned long *tiempo_eje_col){
  
  void (*Funcionalidad)(bool) = NULL;

  Serial.print("COLUMNA: ");
  Serial.println(columna);

  if((*tiempo_fin_bloque) - (*tiempo_inicio_bloque) >= 0){//VERIFICA SI EL TIEMPO TRANSCURRIDO ES MAYOR A 0 MICROSEGUNDOS
    if(memoria_instrucciones[*bloque][*fila][columna] != 0){ //VERIFICA SI EXISTE UNA INSTRUCCION PARA EJECUTAR
      if(!(*ejecutada) ){ //VERIFICA SI NO SE HA EJECUTADO LA INSTRUCCION
        Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][*fila][columna]])->Ptr_func;
        Funcionalidad(false);
        *ejecutada = true; 

       /* if(columna == 1){
          verificar_color_semaforo_movimiento_traslacion(memoria_instrucciones[*bloque][*fila][columna]);
        }*/
        if((*fila) == 0){ //VERIFICA SI ES LA PRIMERA INSTRUCCION DE LA COLUMNA DADA
          *tiempo_inicio_inst = *tiempo_inicio_bloque;
        }else{
          *tiempo_inicio_inst = millis(); 
        }
      }else{ //VERIFICA SI LA INSTRUCCION YA FUE EJECUTADA
        *tiempo_fin_col = millis();

        if(!(*tiempo_exed_col)){ //VERIFICA SI AUN NO HAY TIEMPO EXCENDENTE EN ESTA COLUMNA 
          *tiempo_inst = funcionalidades[memoria_instrucciones[*bloque][*fila][columna]]->exec_time;
      
          //VERIFICA SI AUN NO SE HA ESTABLECIDO EL TIEMPO DEESTA COLUMNA Y LO ESTABLECE
          if((*tiempo_eje_col) == 0){
            *tiempo_eje_col = tiempo_eje_columna_instrucciones(*bloque, columna);
          }

          //VERIFICA SI YA SE CUMPLIO EL TIEMPO DE LA INSTRUCCION, SI A ESE BLOQUE AUN LE QUEDAN INSTRUCCIONES POR EJECUTAR Y SI EL TIEMPO DE EJECUCION DE LA COLUMNA LLEGO A SU FIN
          if(  ((*tiempo_fin_col) - (*tiempo_inicio_col)  >= (*tiempo_eje_col)) && ((*tiempo_fin_col) - (*tiempo_inicio_col)  < *tiempo_eje_bloque) ){
            *tiempo_inst += *tiempo_eje_bloque - (*tiempo_fin_col - *tiempo_inicio_col);
            *tiempo_exed_col = true;
          }
        }

        //VERIFICA SI EL TIEMPO TRANSCURRIDO DESDE QUE COMENZO A EJECUTARSE LA INSTRUCCION ES IGUAL AL ESTABLECIDO PARA SU EJECUCION O SI EL TIEMPO ESTABLECIDO PARA LA EJECUCION DE LA COLUMNA YA SE CUMPLIO.
        if(((*tiempo_fin_col) - (*tiempo_inicio_inst)  >= *tiempo_inst) || ((*tiempo_fin_col) - (*tiempo_inicio_col)  >= (*tiempo_eje_bloque)) ){ 
          Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][*fila][columna]])->Ptr_func;
          Funcionalidad(true); //DESACTIVA LA INSTRUCCION
          (*fila)++;// INCREMENTA LA FILA A RECORRER
          *ejecutada = false; 
          *tiempo_exed_col = false;
        }
      }    
    }
  }
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------FUNCIONALIDADES ROBOT--------------------------------------------------------------------------------------------------------------*/

void comando_voz(){
 myFiles.load(5, 0, 310, 480, "comando_voz.RAW", 1 , 0);
 delay(1000);
  /*bool fin_eje_inst = false;
  long unsigned tiempo_inicio = millis();

  voz_comando = true;
  
  do{
    if(!fin_eje_inst){
      fin_eje_inst = true;
      myFiles.load(5, 0, 310, 480, "comando_voz.RAW", 1 , 0);

      for(int i = 0; i < NUM_LEDS ; i++){
        color_led.setPixelColor(i, color_led.Color(93,193,185));
      }
      color_led.show();
      //leer_comando si lo reconoces muestra la pantalla correspondiente y luego 
    }
  }
  while(millis() - tiempo_inicio < 10000);

  for(int i = 0; i < NUM_LEDS ; i++){
    color_led.setPixelColor(i, color_led.Color(0,0,0));
  }
  color_led.show();*/
}

/*--FUNCION VACIA PARA LAS FUNCIONALIDADES ADMINISTRATIVAS--*/
void prueba(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("ACTIVANDO INSTRUCCION");
  }else{
    Serial.println("DESACTIVANDO INSTRUCCION"); 
  } 
}
/*----------------------------------------------------------*/


/*----EMITE SONIDO DADA LA VARIABLE QUE INDICA SI SE HA LLEGADO AL FIN DE LA EJECUCION DE LA INSTRUCCION---*/
void emitir_sonido(bool fin_eje_inst){

  if(!fin_eje_inst){
    MP3.play(1); 
  }else{
    MP3.pause();   
  }
}
/*-----------------------------------------------------------------------------------------------------------*/


/*----ENCIENDE O APAGA LAS LUCES LEDS DEL ROBOT DADA LA VARIABLE QUE INDICA SI HA LLEGADO EL TIEMPO DE FINALIZACION DE LA INSTRUCCION-----*/
void encender_luces(bool fin_eje_inst){

  if(!fin_eje_inst){
    if(memoria_colores_luces[luces] == -1){
      luces_semaforo = true;
    }else{
      for(int i = 0; i < NUM_LEDS ; i++){
        color_led.setPixelColor(i, color_led.Color(colores[memoria_colores_luces[luces]]->R,colores[memoria_colores_luces[luces]]->G, colores[memoria_colores_luces[luces]]->B));
      }
      color_led.show();
    }
  }else{
    if(memoria_colores_luces[luces] == -1){
      luces_semaforo = false;
    }else{
      for(int i = 0; i < NUM_LEDS ; i++){
        color_led.setPixelColor(i, color_led.Color(0,0,0));
      }
      color_led.show();
    }
    luces++;   
  }
}
/*---------------------------------------------------------------------------------------------------------------------------------------*/


/*----GRABA EL AUDIO INDICADO POR EL MICROFONO DURANTE UN TIEMPO ESTIMADO-----*/
void grabar_audio(){
  bool fin_eje_inst = false;
  long unsigned tiempo_inicio = millis();

  grabacion = true;
  
  //LA LOGICA DE LA GRABACION DEBE SER REIMPLEMENTADA EN ESTE MODULO

  do{
    if(!fin_eje_inst){
      fin_eje_inst = true;
      myFiles.load(5, 0, 310, 480, "grabando_sonido.RAW", 1 , 0);
      for(int i = 0; i < NUM_LEDS ; i++){
        color_led.setPixelColor(i, color_led.Color(0,0,255));
      }
      color_led.show();
      //digitalWrite(REC,HIGH);
    }
  }
  while(millis() - tiempo_inicio < 10000);

  for(int i = 0; i < NUM_LEDS ; i++){
    color_led.setPixelColor(i, color_led.Color(0,0,0));
  }
  color_led.show();

  //digitalWrite(REC,LOW);
 
}
/*---------------------------------------------------------------------------*/


/*----REPRODUCE LA GRABACION DADA LA VARIABLE QUE INDICA SI HA LLEGADO EL TIEMPO DE FINALIZACION DE LA INSTRUCCION-----*/
void reproducir_grabacion(bool fin_eje_inst){


}
/*-------------------------------------------------------------------------------------------------------------------*/

void agitar_cola(){

  unsigned long tiempo_inicio = 0, tiempo_fin = 0;
  unsigned long tiempo_ahora = 0;
  tiempo_inicio = millis();
  tiempo_fin = millis();

  while(tiempo_fin - tiempo_inicio <= 5000){
    for(int pos = 90; pos <= 180; pos++){
      tiempo_ahora = millis();
      cola.write(pos);
      while(millis() < tiempo_ahora + 10 );
    }

    for(int pos = 180; pos >= 0; pos--){
      tiempo_ahora = millis();
      cola.write(pos);
      while(millis() < tiempo_ahora + 10 );
    }

    for(int pos = 0; pos <= 90; pos++){
      tiempo_ahora = millis();
      cola.write(pos);
      while(millis() < tiempo_ahora + 10 );
    }
    tiempo_fin = millis();
  }
}

void mover_cabeza_der(){
  unsigned long tiempo_ahora = 0;

  for(int pos = 90; pos <= 180; pos++){
    tiempo_ahora = millis();
    cabeza.write(pos);
    while(millis() < tiempo_ahora + 25 );
  }

  tiempo_ahora = millis();

  while(millis() < tiempo_ahora + 2000 );
    
  for(int pos = 180; pos >= 90; pos--){
    tiempo_ahora = millis();
    cabeza.write(pos);
    while(millis() < tiempo_ahora + 25 );
  }
}

void mover_cabeza_izq(){
  unsigned long tiempo_ahora = 0;

  for(int pos = 90; pos >= 0; pos--){
    tiempo_ahora = millis();
    cabeza.write(pos);
    while(millis() < tiempo_ahora + 25 );
  }

  tiempo_ahora = millis();

  while(millis() < tiempo_ahora + 2000 );
    
  for(int pos = 0; pos <= 90; pos++){
    tiempo_ahora = millis();
    cabeza.write(pos);
    while(millis() < tiempo_ahora + 25 );
  }
}


/*------------------AVANZAR EL ROBORT--------------*/
void avanzar(bool fin_eje_inst){

  Serial.println("AVANZAR");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(0);
    rueda_der.write(180);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
  
}
/*-------------------------------------------------*/

/*---------------------RETROCEDER------------------*/
void retroceder(bool fin_eje_inst){
  Serial.println("RETROCEDER");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(180);
    rueda_der.write(0);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
  
}
/*-------------------------------------------------*/

/*----------------GIRAR A LA IZQUIERDA--------------*/
void girar_izquierda(bool fin_eje_inst){
  Serial.println("GIRAR IZQUIERDA");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(180);
    rueda_der.write(90);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
  
}
/*-------------------------------------------------*/

/*----------------GIRAR A LA DERECHA--------------*/
void girar_derecha(bool fin_eje_inst){
  Serial.println("GIRAR DERECHA");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(0);
    rueda_der.write(90);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
  
}
/*-------------------------------------------------*/

/*--------------VOLVER POR LA IZQUIERDA------------*/
void volver_izquierda(bool fin_eje_inst){
  Serial.println("VOLVER POR LA IZQUIERDA");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(180);
    rueda_der.write(90);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
}
/*-------------------------------------------------*/


/*--------------VOLVER POR LA DERECHA------------*/
void volver_derecha(bool fin_eje_inst){
  Serial.println("VOLVER POR LA DERECHA");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(0);
    rueda_der.write(90);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
}
/*-------------------------------------------------*/

/*--------------GIRAR SOBRE SI MISMO------------*/
void girar_sobre_si_mismo(bool fin_eje_inst){
  Serial.println("GIRAR SOBRE SI MISMO");
  if(!fin_eje_inst){
    Serial.println("COMIENZO A MOVER RUEDA");
    rueda_izq.write(180);
    rueda_der.write(90);
  }else{
    Serial.println("TERMINO DE MOVER RUEDA");
    rueda_izq.write(90);
    rueda_der.write(90);
  }
}
/*-------------------------------------------------*/

/*---------------------------------------------------------------------------------------------OTRAS FUNCIONALIDADES-------------------------------------------------------------------*/

/*------INICIALIZA PANTALLA TFT-------*/
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

/*--------MUESTRA IMAGEN EN GRANDE DE TARJETA-------*/
void imprimir_imagen_tarjeta(int indice){

  switch(indice){
    case 0:{
      Serial.println("ENTRO EN EL CASE 0");
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
      myFiles.load(5, 0, 310, 480, "tag_abrir_ojos.RAW", 1 , 0);
      break;
    }
    case 21 :{
      myFiles.load(5, 0, 310, 480, "tag_cerrar_ojos.RAW", 1 , 0);
      break;
    }
    case 22 :{
      myFiles.load(5, 0, 310, 480, "tag_pestanar.RAW", 1 , 0);
      break;
    }
    case 23 :{
      myFiles.load(5, 0, 310, 480, "tag_grabar_audio.RAW", 1 , 0);
      break;
    }
    case 24 :{
      myFiles.load(5, 0, 310, 480, "tag_repro_audio.RAW", 1 , 0);
      break;
    }
    case 25 :{
      myFiles.load(5, 0, 310, 480, "tag_emitir_sonido.RAW", 1 , 0);
      break;
    }
    case 26 :{
      //myFiles.load(5, 0, 310, 480, "tag_.RAW", 1 , 0);
      break;
    }
    case 27 :{
      myFiles.load(5, 0, 310, 480, "luces_naranjas.RAW", 1 , 0);
      break;
    }
    case 28 :{
      myFiles.load(5, 0, 310, 480, "luces_amarillas.RAW", 1 , 0);
      break;
    }
    case 29 :{
      myFiles.load(5, 0, 310, 480, "luces_rojas.RAW", 1 , 0);
      break;
    }
    case 30 :{
      myFiles.load(5, 0, 310, 480, "luces_azules.RAW", 1 , 0);
      break;
    }
    case 31 :{
      myFiles.load(5, 0, 310, 480, "luces_moradas.RAW", 1 , 0);
      break;
    }
    case 32 :{
      myFiles.load(5, 0, 310, 480, "luces_verdes.RAW", 1 , 0);
      break;
    }
    case 33 :{ 
      myFiles.load(5, 0, 310, 480, "luces_rosadas.RAW", 1 , 0);
      break;
    }
    case 34 :{ 
      myFiles.load(5, 0, 310, 480, "tag_ir_siguiente_bloque.RAW", 1 , 0);
      break;
    }
    case 35 :{ 
      myFiles.load(5, 0, 310, 480, "tag_ir_bloque_anterior.RAW", 1 , 0);
      break;
    }  
  }
  unsigned long tiempo_ahora = 0;
  
  tiempo_ahora = millis();   //RETRASO DE 1250MS
  while(millis() < tiempo_ahora + 500 );
  
}

void imprimir_funcion_columna_matriz_funcionalidades(int bloque, int columna, int indice_fila_selec, bool seleccion){
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
      imprimir_icono_tarjeta(1, memoria_instrucciones[bloque][fila][columna], cord_y); 
    }
    if(fila < indice_fila_selec && !seleccion){
      imprimir_icono_tarjeta(-1, memoria_instrucciones[bloque][fila][columna], cord_y); 
    }
    if(fila < indice_fila_selec && seleccion){
      imprimir_icono_tarjeta(0, memoria_instrucciones[bloque][fila][columna], cord_y); 
    }
    if(fila > indice_fila_selec){
      imprimir_icono_tarjeta(0, memoria_instrucciones[bloque][fila][columna], cord_y); 
    }      
  }
}


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
      imprimir_icono_tarjeta(0, memoria_instrucciones[bloque][fila][columna], cord_y); 
    }
  }

  if(num_bloques > bloque ){ //si verificaste que hay un bloque despues
    myFiles.load(218, 455, 97, 25, "f_siguiente.RAW", 1 , 0);
  }

  if(num_bloques > 0 && bloque != 0 ){ //si verificaste que hay un bloque antes
    myFiles.load(5, 455, 99, 25, "f_anterior.RAW", 1 , 0);
  }


  //myGLCD.clrScr();
}

void imprimir_icono_tarjeta(int estado, int indice, int pos_y){
  
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
        myFiles.load(195, pos_y, 60, 60, "abrir_ojos.RAW", 1 , 0);
      }
      if(estado == 1){
         myFiles.load(195, pos_y, 60, 60, "selec_abrir_ojos.RAW", 1 , 0);
      }
      if(estado == -1){
        myFiles.load(195, pos_y, 60, 60, "eje_abrir_ojos.RAW", 1 , 0);
      }
      break;
    }
    case 21 :{
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
    case 22 :{
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
    case 23 :{
      //myFiles.load(5, 0, 310, 480, "tag_grabar_audio.RAW", 1 , 0);
      break;
    }
    case 24 :{
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
    case 25 :{
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
  }
}
/*---------IMPRIME UN MENSAJE EN PANTALLA TFT------*/
void imprimir_pantalla_tft(char *UID){
  myGLCD.clrScr();
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(UID, CENTER, 15);
  myGLCD.print("*SE HA ESCANEADO UNA TAG*", CENTER, 1);
}
/*------------------------------------------------*/


/*-----INICIALIZA EL MODULO DE SONIDO----*/
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


/*-----INICIALIZA LA MEMORIA QUE ALMACENA LOS INDICES DE LOS COLORES A UTILIZAR------*/
void inicializar_memoria_colores_luces(){
  if ((memoria_colores_luces = (int) malloc(sizeof (int) * 8)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

  for(int i = 0; i <= 8; i++){
    memoria_colores_luces[i] = -1;
  }

}
/*---------------------------------------------------------------------------------*/


/*-----AGREGA LA POSICION QUE TIENE UN COLOR EN EL ARREGLO DE COLORES A LA MEMORIA DONDE ESTOS SE ALMACENAN PARA SER EJECUTADOS-------*/
void agregar_color_luces_memoria(int pos_color){ //registra la posicion de memoria del color en el arreglo de colores
  memoria_colores_luces[luces -1] = pos_color;
  
}
/*-------------------------------------------------------------------------------------------------------------------------------------*/

void displayEmotion(byte left[8], byte right[8]) {
  lc.clearDisplay(addrL);
  lc.clearDisplay(addrR);
  for(int row=0;row<8;row++) {
    lc.setRow(addrL,row,left[row]);
    lc.setRow(addrR,row,right[row]);
  }
}


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

  // turn on all LEDs for a test
  /*for(int row = 0; row<8 ;row++) {
    lc.setRow(addrL, row, 255);
    lc.setRow(addrR, row, 255);
    while(millis() - tiempo_actual < 100);
  }*/

}

void ojos_neutros_pos_0(){
  unsigned long tiempo_actual = 0;
  byte left_u[8] = {
  0b00000000,
  0b01111010,
  0b11100101,
  0b11100101,
  0b11111101,
  0b11111101,
  0b01111010,
  0b00000000
};

  displayEmotion(left_u, left_u);
}

void ojos_neutros_pos_1(){
  unsigned long tiempo_actual = 0;
 
  byte right_u[8] = {
  0b00000000,
  0b01111010,
  0b11111101,
  0b11111101,
  0b11100101,
  0b11100101,
  0b01111010,
  0b00000000
};

  displayEmotion(right_u, right_u);
}

void ojos_neutros_pos_2(){
  unsigned long tiempo_actual = 0;

  byte left_d[8] = {
  0b00000000,
  0b01111010,
  0b10011101,
  0b10011101,
  0b11111101,
  0b11111101,
  0b01111010,
  0b00000000
};

  displayEmotion(left_d, left_d);
}

void ojos_neutros_pos_3(){
  unsigned long tiempo_actual = 0;
 
  byte right_d[8] = {
  0b00000000,
  0b01111010,
  0b11111101,
  0b11111101,
  0b10011101,
  0b10011101,
  0b01111010,
  0b00000000
};
  displayEmotion(right_d, right_d);
}

void ojos_cerrados(){
  unsigned long tiempo_actual = 0;

  byte left[8] = {
  0b00000000,
  0b01111000,
  0b11110000,
  0b11110000,
  0b11110000,
  0b11110000,
  0b01111000,
  0b00000000
};
  displayEmotion(left, left); 
}


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

void verificar_color_semaforo_movimiento_traslacion(int indice_func){

  if((indice_func == 11) || (indice_func == 12) || (indice_func == 13) || (indice_func == 14) || (indice_func == 16) || (indice_func == 18) ){
    avanzando = true;
  }
  if((indice_func == 15) || (indice_func == 17) ){
    precaucion = true;
  }
}



/*------SELECCIONAR COLUMNA---*/
void seleccionar_columna(int *colum_selec){

}
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
  inicializar_memoria_colores_luces();
  color_led.begin();
  rueda_izq.write(90);
  rueda_der.write(90);
  cola.write(90);
  cabeza.write(90);
  inicializar_pantalla_tft();
  inicializar_ojos();
  //ojos_neutros();
 
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

  escanear_instrucciones();

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
    delay(1500); //ver si se reemplza por millis
    myFiles.load(5, 0, 310, 480, "memoria_instrucciones.RAW", 1 , 0);
    delay(1000); //ver si se reemplza por millis
    Serial.print("**************ITERACION NRO: ");
    Serial.print(volver_a_comenzar);
    Serial.println("**************");
    tiempo_inicio_programa = millis();
    tiempo_fin_programa = millis();
   
    luces = 0;

    while(!fin_programa){ //REPITE MIENTRAS EL TIEMPO TRANSCURRIDO SEA MENOR AL TIEMPO DE EJECUCION DEL PROGRAMA
      
      if (!inicio_bloque){ //SI ESTE BLOQUE AUN NO SE EJECUTA INICIALIZA TODAS LAS VARIABLES CORRESPONDIENTES
        myGLCD.clrScr();
        inicio_bloque = true;
        tiempo_fin_bloque = millis();
        tiempo_inicio_bloque = millis();
        tiempo_inicio_col_0 = tiempo_inicio_bloque;
        tiempo_inicio_col_1 = tiempo_inicio_bloque;
        tiempo_inicio_col_2 = tiempo_inicio_bloque;
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
      //VERIFICA SI LA COLUMNA 0 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA
      if(memoria_instrucciones[bloque][0][0] != 0){ 
        ejecutar_columna_instrucciones(&bloque,0, &fila_col_0, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_0, &tiempo_inicio_col_0, &tiempo_fin_col_0, &tiempo_exed_col_0, &tiempo_inicio_inst_col_0, &tiempo_eje_inst_0, &tiempo_eje_colum_0);
        imprimir_funcion_columna_matriz_funcionalidades(bloque, 0, fila_col_0, false);
      }   
      //VERIFICA SI LA COLUMNA 1 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA
      if(memoria_instrucciones[bloque][0][1] != 0){
        ejecutar_columna_instrucciones(&bloque,1, &fila_col_1, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_1, &tiempo_inicio_col_1, &tiempo_fin_col_1, &tiempo_exed_col_1,&tiempo_inicio_inst_col_1, &tiempo_eje_inst_1, &tiempo_eje_colum_1);
        imprimir_funcion_columna_matriz_funcionalidades(bloque, 1, fila_col_1, false);
      }
      //VERIFICA SI LA COLUMNA 2 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA
      if(memoria_instrucciones[bloque][0][2] != 0){
        ejecutar_columna_instrucciones(&bloque,2, &fila_col_2, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_2, &tiempo_inicio_col_2, &tiempo_fin_col_2, &tiempo_exed_col_2,&tiempo_inicio_inst_col_2, &tiempo_eje_inst_2, &tiempo_eje_colum_2);
        imprimir_funcion_columna_matriz_funcionalidades(bloque, 2, fila_col_2, false);
      }
      //VERIFICA SI LA COLUMNA 4 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA
      if(memoria_instrucciones[bloque][0][3] != 0){
        ejecutar_columna_instrucciones(&bloque,3, &fila_col_3, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_3, &tiempo_inicio_col_3, &tiempo_fin_col_3, &tiempo_exed_col_3,&tiempo_inicio_inst_col_3, &tiempo_eje_inst_3, &tiempo_eje_colum_3);
        imprimir_funcion_columna_matriz_funcionalidades(bloque, 4, fila_col_4, false);
      }
      
      //VERIFICA SI LA COLUMNA 4 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA
      if(memoria_instrucciones[bloque][0][4] != 0){
        ejecutar_columna_instrucciones(&bloque,4, &fila_col_4, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_4, &tiempo_inicio_col_4, &tiempo_fin_col_4, &tiempo_exed_col_4,&tiempo_inicio_inst_col_4, &tiempo_eje_inst_4, &tiempo_eje_colum_4);
        imprimir_funcion_columna_matriz_funcionalidades(bloque, 4, fila_col_4, false);
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
  luces = 0;
  comenzar_programa = false;
  finalizar_programa = false;
  sincronizacion = 0;
  inicializar_memoria();
  inicializar_memoria_colores_luces();
  color_reg = 0;
  ejecutar_programa = false;
}
/*------------------------------------------*/




