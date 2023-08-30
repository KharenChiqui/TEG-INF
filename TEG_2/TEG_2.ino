#include "DFRobotDFPlayerMini.h"
#include "SoftwareSerial.h"

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
  struct Funcionalidad *next_func_conc;
  unsigned long exec_time;
}Funcionalidades;

typedef struct Memoria_Instrucciones{
  int pos;
  Memoria_Instrucciones *next_func;
}Memoria_Instrucciones;
/*----------------------*/

/*------VARIABLES GLOBALES------*/
int count = 0;
Funcionalidad **funcionalidades = NULL;
Memoria_Instrucciones *memoria = NULL;
bool sonido = false;
bool comenzar_programa = false;
bool finalizar_programa = false;
int volver_a_comenzar = 0;
bool ejecutar_programa = false;
/*-------------------------------*/



/*--------------------------------------------FUNCIONALIDADES---------------------------------------*/

void almacenar_en_memoria(int posicion){
  Memoria_Instrucciones *nuevo_elemento = NULL;

	if ((nuevo_elemento = (Memoria_Instrucciones *) malloc(sizeof (Memoria_Instrucciones))) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

  nuevo_elemento-> pos = posicion;
  nuevo_elemento->next_func = NULL;

  //PRIMER ELEMENTO
  if(memoria == NULL){
    memoria = nuevo_elemento;
  }
    else {
      Memoria_Instrucciones *Ptr_aux = memoria;

      while(Ptr_aux->next_func != NULL){
        Ptr_aux = Ptr_aux->next_func;
      }
      Ptr_aux->next_func = nuevo_elemento;
    }
  }

/*----CREA UNA NUEVA FUNCIONALIDAD PARTIENDO DE SU UID Y EL PUNTERO A LA FUNCION QUE CORRESPONDE----*/
Funcionalidad *crear_nueva_funcionalidad(String UID, void (*Ptr)(), int type, Funcionalidad *next_func, Funcionalidad *next_func_conc, unsigned long exec_time){
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
  nueva_func -> next_func_conc = NULL;
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
  nueva_func ->next_func_conc = next_func_conc;
  nueva_func -> exec_time = exec_time;

return nueva_func;}
/*-------------------------------------------------------------------------------------------------*/


/*---CREA UN ARREGLO CON LAS FUNCIONALIDADES Y LO INICIALIZA CON SUS RESPECTIVOS VALORES--*/
void crear_arreglo_funcionalidades(){
  void *p = NULL ;

  if ((funcionalidades =(Funcionalidad **) malloc(sizeof (Funcionalidad *) * 35)) == NULL){
	  Serial.println("funcionalidades: error en el malloc\n");
		exit(1);
  }

  funcionalidades[0] = crear_nueva_funcionalidad("B3 54 7A 12", &prueba, 1, NULL, NULL,0); //SINCRONIZACION
  funcionalidades[1] = crear_nueva_funcionalidad("E3 F3 F0 94", &prueba, 1, NULL, NULL,0); //VOLVER A COMENZAR
  funcionalidades[2] = crear_nueva_funcionalidad("A3 CE 89 94", &prueba, 1, NULL, NULL,0); //COMENZAR PROGRAMA
  funcionalidades[3] = crear_nueva_funcionalidad("43 26 C4 12", &prueba, 1, NULL, NULL,0); //FINALIZAR PROGRAMA
  funcionalidades[4] = crear_nueva_funcionalidad("A3 3E 72 94", &prueba, 0, NULL, NULL,0); //BORRAR
  funcionalidades[5] = crear_nueva_funcionalidad("E3 FC B3 12", &prueba, 2, NULL, NULL,0); //EJECUTAR PROGRAMA
  funcionalidades[6] = crear_nueva_funcionalidad("93 02 86 94", &prueba, 2, NULL, NULL,0); //PAUSAR
  funcionalidades[7] = crear_nueva_funcionalidad("53 12 73 94", &prueba, 3, NULL, NULL,0); //MOVER CABEZA A LA IZQUIERDA
  funcionalidades[8] = crear_nueva_funcionalidad("F3 94 8B 94", &prueba, 3, NULL, NULL,0); //MOVER CABEZA A LA DERECHA
  funcionalidades[9] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL, NULL,0); //AGITAR COLA
  funcionalidades[10] = crear_nueva_funcionalidad("83 0E AA 12", &prueba, 4, NULL, NULL,0); //AVANZAR
  funcionalidades[11] = crear_nueva_funcionalidad("13 45 8A 94", &prueba, 4, NULL, NULL,0); //GIRAR A LA DERECHA
  funcionalidades[12] = crear_nueva_funcionalidad("D3 DF 81 94", &prueba, 4, NULL, NULL,0); //GIRAR SOBRE SI MISMO
  funcionalidades[13] = crear_nueva_funcionalidad("B3 23 8F 94", &prueba, 4, NULL, NULL,0); //VOLVER POR LA DERECHA
  funcionalidades[14] = crear_nueva_funcionalidad("13 63 6B 94", &prueba, 4, NULL, NULL,0); //RETROCEDER
  funcionalidades[15] = crear_nueva_funcionalidad("83 11 6B 94", &prueba, 4, NULL, NULL,0); //GIRAR A LA IZQUIERDA
  funcionalidades[16] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL, NULL,0); //EVITAR OBSTACULOS
  funcionalidades[17] = crear_nueva_funcionalidad("93 E2 20 95", &prueba, 4, NULL, NULL,0); //VOLVER POR LA IZQUIERDA
  funcionalidades[18] = crear_nueva_funcionalidad("23 DE 6C 94", &prueba, 5, NULL, NULL,0); //ENCENDER LUCES
  funcionalidades[19] = crear_nueva_funcionalidad("13 7C 72 94", &prueba, 5, NULL, NULL,0); //APAGAR LUCES
  funcionalidades[20] = crear_nueva_funcionalidad("F3 A9 89 94", &prueba, 6, NULL, NULL,0); //ABRIR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("93 D3 80 94", &prueba, 6, NULL, NULL,0); //CERRAR OJOS
  funcionalidades[22] = crear_nueva_funcionalidad("63 36 C6 94", &prueba, 6, NULL, NULL,0); //PESTANEAR
  funcionalidades[23] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL, NULL,0); //GRABAR AUDIO
  funcionalidades[24] = crear_nueva_funcionalidad("53 2D 89 94", &prueba,7, NULL, NULL,0); //REPRODUCIR AUDIO
  funcionalidades[25] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido, 7, NULL, NULL,15000); //EMITIR SONIDO
  funcionalidades[26] = crear_nueva_funcionalidad("E3 F7 A7 12", &pausar_sonido, 7, NULL, NULL,0); //SILENCIAR

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

/*---ENCUENTRA LA FUNCIONALIDAD DE UNA TARJETA DADO SU UID---*/
void encontrar_funcionalidad_tarjeta(char* UID){
  bool flag = false;

  for(int i = 0; i < 35; i++ ){
    //Serial.println(i,1);
    if(! strcmp(UID,funcionalidades[i]-> UID)){
      /*if(funcionalidades[i]->type == 7){
        Serial.println("ES UNA FUNCIONALIDAD DE SONIDO");
        if(!sonido){
          Serial.println("Sonido desactvivado");
          sonido = true;
        }
      }*/
      almacenar_en_memoria(i);
      return;  
    }
  }
}
/*----------------------------------------------------------*/
void  prueba(){
  Serial.println("Entro a pruebita");
  String text = "pruebita";
  text;
}
/*----------------------------------------------------------------------------------------------*/

void grabar_audio(){

  
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
  MP3.start(); 
  delay(10000);
  MP3.pause();
}

void pausar_sonido(char *UID){
  Serial.println("Entro para pausar");
  MP3.pause();
  delay(100);
}

void limpiar_memoria(){

  Memoria_Instrucciones *Ptr_aux = NULL;

  for(Memoria_Instrucciones *Ptr = memoria; Ptr != NULL;  ){
    Ptr_aux = Ptr->next_func;
    free(Ptr);
    Ptr = Ptr_aux;
  }
  memoria = NULL;
}

int tamano_memoria(){
  int tam = 0;
  
  for(Memoria_Instrucciones *Ptr_aux = memoria; Ptr_aux != NULL; Ptr_aux = Ptr_aux->next_func ){
    tam++;
  }
return tam;}

/*void leer_tarjetas(){
  Serial.println("HOLA");
  
  time_t endwait = time(NULL) + 5;

  while(time(NULL) < endwait){
    println("Hey");
  }
}*/



/*--FUNCIONES QUE SE EJECUTAN UNA SOLA VEZ--*/
void setup(void) {
    Serial.begin(9600);
    while(!Serial){
      
    }
  Serial.println("NDEF Reader");
  crear_arreglo_funcionalidades();   
  inicializar_sonido(); 
  nfc.begin();
}
/*------------------------------------------*/



/**---FUNCIONES QUE SE EJECUTAN N VECES----*/
void loop(void) {
  Serial.println("\nScan a NFC tag\n");

  while(!ejecutar_programa){

    if (nfc.tagPresent()){
      NfcTag tag = nfc.read();
      tag.print();
      String TagUID = tag.getUidString();
      char *ptrUID = NULL;
      ptrUID = new char[TagUID.length() + 1];
      strcpy(ptrUID, TagUID.c_str());

      if(!strcmp(ptrUID, "A3 CE 89 94") || !strcmp(ptrUID, "43 26 C4 12") || !strcmp(ptrUID, "E3 F3 F0 94") ||  !strcmp(ptrUID, "E3 FC B3 12")){
        if(!strcmp(ptrUID, "A3 CE 89 94")){
          if(!comenzar_programa){
            comenzar_programa = true;
          } 
        }

        if(!strcmp(ptrUID, "43 26 C4 12")){
          if(comenzar_programa){
            if(!finalizar_programa){
              finalizar_programa = true;
            } 
          } 
        }

        if(!strcmp(ptrUID, "E3 F3 F0 94")){
          if(comenzar_programa && finalizar_programa){
            volver_a_comenzar++;
          }
          
        }

        if(!strcmp(ptrUID, "E3 FC B3 12")){
          if(comenzar_programa && finalizar_programa){
            ejecutar_programa = true;
          }
        }
        
      } else{

          if(comenzar_programa && !finalizar_programa){
            encontrar_funcionalidad_tarjeta(ptrUID);
          } 
        }  
      }
    }
    delay(5000); 

    int tam_mem = tamano_memoria(); 

    if(ejecutar_programa){
      volver_a_comenzar++;
  
     do{

        //imprimir_lista(); //cambiar por un llamado a cada funcionalidad en secuencia
        //Serial.println("A ejecutar funcionalidades");
        //ejecutar_funcionalidades();
        volver_a_comenzar--;

      } while(volver_a_comenzar > 0);   

        ejecutar_programa = false;
        comenzar_programa = false;
        finalizar_programa = false;

        if(memoria != NULL){
          limpiar_memoria();
      }
    }
  }
/*------------------------------------------*/




