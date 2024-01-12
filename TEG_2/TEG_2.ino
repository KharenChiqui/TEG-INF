#include "DFRobotDFPlayerMini.h"
#include "SoftwareSerial.h"
#define cant_colum 4 //NUMERO DE COLUMNAS MEMORIA
#define cant_filas 7 //NUMERO DE FILAS MEMORIA

SoftwareSerial DFP(10,11); //comentar TX RX
DFRobotDFPlayerMini MP3;

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



/*----TIPOS DE DATOS---*/
typedef struct Funcionalidad {
	char *UID;
	void *Ptr_func;
  int type;
  struct Funcionalidad *next_func; 
  unsigned long exec_time;
}Funcionalidades;

/*----------------------*/

/*------VARIABLES GLOBALES------*/
int count = 0;
Funcionalidad **funcionalidades = NULL;
int ***memoria_instrucciones = NULL;
int sincronizacion = 0;
bool sonido = false;
bool comenzar_programa = false;
bool finalizar_programa = false;
int volver_a_comenzar = 0;
bool ejecutar_programa = false;
/*-------------------------------*/



/*--------------------------------------------FUNCIONALIDADES---------------------------------------*/


/*----CREA UNA NUEVA FUNCIONALIDAD PARTIENDO DE SU UID Y EL PUNTERO A LA FUNCION QUE CORRESPONDE----*/
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

int crear_memoria_instrucciones(){

  if (( memoria_instrucciones = (int ***) malloc(5* sizeof (int **))) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }
 
 for(int dim = 0; dim<5; dim++){
  memoria_instrucciones[dim] = (int **) malloc(sizeof(int *)*8);
 }

  for(int colum = 0; colum < 5; colum++){
    for(int filas = 0 ; filas <8; filas++)
    {
      memoria_instrucciones[colum][filas] = (int *) malloc(sizeof(int) * 5); 
    }
  }
}

/*---CREA UN ARREGLO CON LAS FUNCIONALIDADES Y LO INICIALIZA CON SUS RESPECTIVOS VALORES--*/
void crear_arreglo_funcionalidades(){
  void *p = NULL ;

  if ((funcionalidades =(Funcionalidad **) malloc(sizeof (Funcionalidad *) * 35)) == NULL){
	  Serial.println("funcionalidades: error en el malloc\n");
		exit(1);
  }

  funcionalidades[0] = crear_nueva_funcionalidad("B3 54 7A 12", &prueba, 1, NULL,0); //SINCRONIZACION
  funcionalidades[1] = crear_nueva_funcionalidad("E3 F3 F0 94", &prueba, 1, NULL,0); //VOLVER A COMENZAR
  funcionalidades[2] = crear_nueva_funcionalidad("A3 CE 89 94", &prueba, 1, NULL,0); //COMENZAR PROGRAMA
  funcionalidades[3] = crear_nueva_funcionalidad("43 26 C4 12", &prueba, 1, NULL,0); //FINALIZAR PROGRAMA
  funcionalidades[4] = crear_nueva_funcionalidad("A3 3E 72 94", &prueba, 0, NULL,0); //BORRAR
  funcionalidades[5] = crear_nueva_funcionalidad("E3 FC B3 12", &prueba, 2, NULL,0); //EJECUTAR PROGRAMA
  funcionalidades[6] = crear_nueva_funcionalidad("93 02 86 94", &prueba, 2, NULL,0); //PAUSAR
  funcionalidades[7] = crear_nueva_funcionalidad("53 12 73 94", &prueba, 3, NULL,5000); //MOVER CABEZA A LA IZQUIERDA
  funcionalidades[8] = crear_nueva_funcionalidad("F3 94 8B 94", &prueba, 3, NULL,5000); //MOVER CABEZA A LA DERECHA
  funcionalidades[9] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL,5000); //AGITAR COLA
  funcionalidades[10] = crear_nueva_funcionalidad("83 0E AA 12", &prueba, 4, NULL,5000); //AVANZAR
  funcionalidades[11] = crear_nueva_funcionalidad("13 45 8A 94", &prueba, 4, NULL, 5000); //GIRAR A LA DERECHA
  funcionalidades[12] = crear_nueva_funcionalidad("D3 DF 81 94", &prueba, 4, NULL,5000); //GIRAR SOBRE SI MISMO
  funcionalidades[13] = crear_nueva_funcionalidad("B3 23 8F 94", &prueba, 4, NULL,5000); //VOLVER POR LA DERECHA
  funcionalidades[14] = crear_nueva_funcionalidad("13 63 6B 94", &prueba, 4, NULL,5000); //RETROCEDER
  funcionalidades[15] = crear_nueva_funcionalidad("83 11 6B 94", &prueba, 4, NULL, 5000); //GIRAR A LA IZQUIERDA
  funcionalidades[16] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL,5000); //EVITAR OBSTACULOS
  funcionalidades[17] = crear_nueva_funcionalidad("93 E2 20 95", &prueba, 4, NULL, 5000); //VOLVER POR LA IZQUIERDA
  funcionalidades[18] = crear_nueva_funcionalidad("23 DE 6C 94", &prueba, 5, NULL, 5000); //ENCENDER LUCES
  funcionalidades[19] = crear_nueva_funcionalidad("13 7C 72 94", &prueba, 5, NULL, 5000); //APAGAR LUCES
  funcionalidades[20] = crear_nueva_funcionalidad("F3 A9 89 94", &prueba, 6, NULL, 5000); //ABRIR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("93 D3 80 94", &prueba, 6, NULL, 5000); //CERRAR OJOS
  funcionalidades[22] = crear_nueva_funcionalidad("63 36 C6 94", &prueba, 6, NULL, 5000); //PESTANEAR
  funcionalidades[23] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL,5000); //GRABAR AUDIO
  funcionalidades[24] = crear_nueva_funcionalidad("53 2D 89 94", &prueba,7, NULL,5000); //REPRODUCIR AUDIO
  funcionalidades[25] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido, 7, NULL,15000); //EMITIR SONIDO
  funcionalidades[26] = crear_nueva_funcionalidad("E3 F7 A7 12", &pausar_sonido, 7, NULL,5000); //SILENCIAR

  /*-----------------------COLORES--------------------*/
  /*funcionalidades[27] = nuevaFuncionalidad("", &prueba);
  funcionalidades[28] = nuevaFuncionalidad("", &prueba);
  funcionalidades[29] = nuevaFuncionalidad("", &prueba);
  funcionalidades[30] = nuevaFuncionalidad("", &prueba);
  funcionalidades[31] = nuevaFuncionalidad("", &prueba);
  funcionalidades[32] = nuevaFuncionalidad("", &prueba);
  funcionalidades[33] = nuevaFuncionalidad("", &prueba);
  funcionalidades[34] = nuevaFuncionalidad("", &prueba);*/ 
    //void (*P)() = (funcionalidades[5])->Ptr_func;
   // P();

}
/*-------------------------------------------------------------------------------------*/

/*----------------------------------------------------------*/
void  prueba(){
  Serial.println("Entro a pruebita");
  String text = "pruebita";
  text;
}
/*----------------------------------------------------------------------------------------------*/

void grabar_audio(){

  
}

void inicializar_memoria(){

   for( int dim = 0 ; dim<5 ; dim++){
    for(int filas = 0 ; filas<8 ; filas++){
      for( int colum = 0 ; colum<5 ; colum++){
          memoria_instrucciones[dim][filas][colum] = 0;
    }   
  }  
  }
}

void imprimir_matriz(){

  for( int dim = 0 ; dim<5 ; dim++){
    Serial.print("/----DIMENSION----/: ");
    Serial.println(dim,1);
    for(int filas = 0 ; filas<8 ; filas++){
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
  MP3.pause();
}

void emitir_sonido(){
  Serial.println("Entro para play");
  //Serial.println();
  /*MP3.start(); 
  delay(10000);
  MP3.pause();*/
}

void pausar_sonido(char *UID){
  Serial.println("Entro para pausar");
  /*MP3.pause();
  delay(100);*/
}

/*--FUNCIONES QUE SE EJECUTAN UNA SOLA VEZ--*/
void setup(void) {
    Serial.begin(9600);
    while(!Serial){
      
    }
  Serial.println("NDEF Reader");
  crear_arreglo_funcionalidades();  
  crear_memoria_instrucciones();
  inicializar_memoria();
  //inicializar_sonido(); 
  nfc.begin();
}
/*------------------------------------------*/


void imprimir_funcionalidades(){
 
  String UID;
  for(int i = 0; i <= 26 ; i++){
    UID = funcionalidades[i]->UID;
    Serial.println(UID);
    
  }
}

void introducir_inst_columna_memoria(int indice_func, int colum, int dim){
bool almacenado = false;


  Serial.print("Dimension:");
  Serial.println(dim,1);
      for( int j = 0 ; j<8 ; j++){
          Serial.print("Fila:");
          Serial.println(j,1);
          if(memoria_instrucciones[dim][j][colum] == 0){
            almacenado = true;
            memoria_instrucciones[dim][j][colum] = indice_func;
            return;
          } 
    }   
  
  

  if(!almacenado){
    Serial.println("NO HAY ESPACIO PARA ALMACENAR OTRA INSTRUCCION DE ESE TIPO");

  }


}
int almacenar_instruccion(char *UID){
  char  *UID_aux = NULL;
  int tipo_inst = -1;
  bool vacio = false;

  for(int i = 0; i <= 26 ; i++){ //se identifica
    UID_aux = funcionalidades[i]->UID;
    //Serial.println(UID + "-" + UID_aux);
    
    if(strcmp(UID,UID_aux) == 0){ 
      tipo_inst = funcionalidades[i]->type;

      if(i == 0 ){ //detecto una tarjeta de sincronizacion . REVISAR QUE LA MATRIZ NO ESTE VACIA PARA HACER EL CAMBIO DE CARA
        vacio = verificar_bloque_instrucciones_vacio(0);
        if(!vacio){ 
          sincronizacion ++; //Cada sincronizacion representa un bloque de instruccciones a almacenar, lo que a su vez representa una dimension
        } 
      }

      if(tipo_inst != 0 && tipo_inst != 1 && tipo_inst != 2){ //si no son instrucciones administrativas almacenalas en la matriz 
        int dimen = sincronizacion;
        switch(tipo_inst){
          case 3: {
            introducir_inst_columna_memoria(i,0,dimen);
            break;
          }
          case 4: {
            introducir_inst_columna_memoria(i,1,dimen);
            break;
          }
          case 5: {
             introducir_inst_columna_memoria(i,2,dimen);
            break;
          }
          case 6: {
             introducir_inst_columna_memoria(i,3,dimen);
            break;
          }
          case 7:{
             introducir_inst_columna_memoria(i,4,dimen);
            break;
          }
          default: Serial.println("LA INNSTRUCCION NO ESTA DISPONIBLE");
        }    
      }
      return 1;
    } 
  }
return 0;}

bool verificar_bloque_instrucciones_vacio(int bloque){ //devuelve 1 si se encuentra vacio, 0 si esta lleno
  int vacio = 0;

  for(int i = 0 ; i<5 ; i++){
    if(memoria_instrucciones[bloque][0][i] == 0){
      vacio ++;
    }
  }
    if(vacio == 5){
      return true;
    }else{
      return false;
    } 
}

void escanear_instrucciones(){

   while(!ejecutar_programa){

    if (nfc.tagPresent()){

      NfcTag tag = nfc.read();
      tag.print();
      String TagUID = tag.getUidString();
      char *ptrUID = NULL;
      ptrUID = new char[TagUID.length() + 1];
      strcpy(ptrUID, TagUID.c_str());
      Serial.println(ptrUID);

      if((strcmp(ptrUID,"E3 FC B3 12") == 0) && comenzar_programa && finalizar_programa){ //si se ha escaneado la tarjeta de ejecutar programa
        ejecutar_programa = true; //si ya indicaronn ejecutar programa
        sincronizacion = 0;
        comenzar_programa = false;
        finalizar_programa = false;
      }

      if(comenzar_programa && !finalizar_programa){ //Si se ha iniciado la escritura del programma y no se ha finalizado la misma comienza a almacenar en memoria
        if (almacenar_instruccion(ptrUID)){
          imprimir_matriz();
        }
      }

      if((strcmp(ptrUID, "A3 CE 89 94") == 0) && !comenzar_programa){ //Si el UID es comenzar programa y esta tarjeta no ha sido escaneada previamente indica que ya comenzo la escritura de instrucciones en el programa
        comenzar_programa = true;
      }

      if((strcmp(ptrUID,"43 26 C4 12") == 0) && !finalizar_programa && comenzar_programa){ //Si el UID es finalizar programa y esta tarjeta no ha sido escaneada previamente indica que ya finalizo la escritura de instrucciones en el programa
        finalizar_programa = true;
      }

      if((strcmp(ptrUID,"E3 F3 F0 94") == 0) && comenzar_programa && finalizar_programa){ //Si el UID es volver a comenzar y ya se escaneo la tarjeta de comienzo y finalizacion del programa quiere decir que el usuario desea una iteracion adicional de este
        volver_a_comenzar++; //indica el numero de iteraciones a realizar
      }
    }
  }
}

/**---FUNCIONES QUE SE EJECUTAN N VECES----*/
void loop(void) {

  escanear_instrucciones();

  while(volver_a_comenzar >= 0){

    Serial.print("**************ITERACION NRO: ");
    Serial.print(volver_a_comenzar);
    Serial.println("**************");
    volver_a_comenzar--;

    while(true){
      //Sacar la cuenta del tiempo maximo de todos los bloques que tegan instrucciones asi sabes cuanto tiene que durar la ejecucion de instrucciones cuando se llegue al tiempo corta el bucle
    }
  }

  volver_a_comenzar = 0;
  inicializar_memoria();
  ejecutar_programa = false;

  delay(1000);
}
/*------------------------------------------*/




