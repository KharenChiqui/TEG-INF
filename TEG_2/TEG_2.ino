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
unsigned long *tiempos_eje_bloques = NULL;

/*-------------------------------*/



/*--------------------------------------------FUNCIONALIDADES---------------------------------------*/

unsigned long *crear_arreglo_tiempos_ejecucion_bloques(){
  unsigned long *nuevos_tiempos_eje_bloques = NULL;

  if ((nuevos_tiempos_eje_bloques = (unsigned long *) malloc(sizeof (unsigned long) * 5)) == NULL){
		Serial.println("nuevaFunc: error en el malloc\n");
		exit(1);
  }

return nuevos_tiempos_eje_bloques;}

void inicializar_arreglo_tiempos_ejecucion_bloques(){ //inicializa el arreglo que contiene los tiempos de ejecuion de cada bloque

  for(int i = 0; i < 5; i++){
    tiempos_eje_bloques[i] = 0;
  }
}

void establecer_tiempos_eje_arreglo_tiempos_bloques(unsigned long tiempo_bloque_0,unsigned long tiempo_bloque_1,unsigned long tiempo_bloque_2, unsigned long tiempo_bloque_3, unsigned long tiempo_bloque_4){

  tiempos_eje_bloques[0] = tiempo_bloque_0;
  tiempos_eje_bloques[1] = tiempo_bloque_1;
  tiempos_eje_bloques[2] = tiempo_bloque_2;
  tiempos_eje_bloques[3] = tiempo_bloque_3;
  tiempos_eje_bloques[4] = tiempo_bloque_4;

}
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
  funcionalidades[7] = crear_nueva_funcionalidad("53 12 73 94", &funcion_3, 3, NULL,5000); //MOVER CABEZA A LA IZQUIERDA
  funcionalidades[8] = crear_nueva_funcionalidad("F3 94 8B 94", &funcion_4, 3, NULL,5000); //MOVER CABEZA A LA DERECHA
  funcionalidades[9] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL,5000); //AGITAR COLA
  funcionalidades[10] = crear_nueva_funcionalidad("83 0E AA 12", &prueba, 4, NULL,5000); //AVANZAR
  funcionalidades[11] = crear_nueva_funcionalidad("13 45 8A 94", &prueba, 4, NULL, 5000); //GIRAR A LA DERECHA
  funcionalidades[12] = crear_nueva_funcionalidad("D3 DF 81 94", &prueba, 4, NULL,5000); //GIRAR SOBRE SI MISMO
  funcionalidades[13] = crear_nueva_funcionalidad("B3 23 8F 94", &prueba, 4, NULL,5000); //VOLVER POR LA DERECHA
  funcionalidades[14] = crear_nueva_funcionalidad("13 63 6B 94", &prueba, 4, NULL,5000); //RETROCEDER
  funcionalidades[15] = crear_nueva_funcionalidad("83 11 6B 94", &prueba, 4, NULL, 5000); //GIRAR A LA IZQUIERDA
  funcionalidades[16] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL,5000); //EVITAR OBSTACULOS
  funcionalidades[17] = crear_nueva_funcionalidad("93 E2 20 95", &prueba, 4, NULL, 5000); //VOLVER POR LA IZQUIERDA
  funcionalidades[18] = crear_nueva_funcionalidad("23 DE 6C 94", &funcion_1, 5, NULL, 5000); //ENCENDER LUCES
  funcionalidades[19] = crear_nueva_funcionalidad("13 7C 72 94", &funcion_2, 5, NULL, 3000); //APAGAR LUCES
  funcionalidades[20] = crear_nueva_funcionalidad("F3 A9 89 94", &funcion_2, 6, NULL, 5000); //ABRIR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("93 D3 80 94", &prueba, 6, NULL, 5000); //CERRAR OJOS
  funcionalidades[22] = crear_nueva_funcionalidad("63 36 C6 94", &prueba, 6, NULL, 8000); //PESTANEAR
  funcionalidades[23] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL,5000); //GRABAR AUDIO
  funcionalidades[24] = crear_nueva_funcionalidad("53 2D 89 94", &prueba,7, NULL,5000); //REPRODUCIR AUDIO
  funcionalidades[25] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido, 7, NULL,15000); //EMITIR SONIDO
  funcionalidades[26] = crear_nueva_funcionalidad("E3 F7 A7 12", &pausar_sonido, 7, NULL,15000); //SILENCIAR

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
      pinMode(12, OUTPUT);
   pinMode(10, OUTPUT);
   pinMode(8, OUTPUT);
   pinMode(5, OUTPUT);
   pinMode(3, OUTPUT);
   pinMode(2, OUTPUT);
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

      for( int j = 0 ; j<8 ; j++){
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

unsigned long tiempo_duracion_bloque_instrucciones(int bloque){ //busca la columna que que dure mas tiempo en ejecutarse. Ese es el tiempo que debe demorar el bloque de instrucciones completo
  
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
return duracion_max;}

unsigned long duracion_programa(){
  unsigned long tiempo_eje_programa = 0;

  for(int i = 0; i<5 ; i++){
      tiempo_eje_programa += tiempos_eje_bloques[i];
  }
return tiempo_eje_programa;}

void escanear_instrucciones(){

   while(!ejecutar_programa){

    if (nfc.tagPresent()){

      NfcTag tag = nfc.read();
      //tag.print();
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
 bool fin_eje = false;

void funcion_1(bool fin_eje_inst){

  if(!fin_eje_inst){
    digitalWrite(10, HIGH);
    digitalWrite(8, HIGH);
  }else{
      digitalWrite(10, LOW);
      digitalWrite(8, LOW);
  }

}
void funcion_2(bool fin_eje_inst){

  if(!fin_eje_inst){
    digitalWrite(5, HIGH);
  }else{
     digitalWrite(5, LOW);
  }

}

void funcion_3(bool fin_eje_inst){

  if(!fin_eje_inst){
    digitalWrite(3, HIGH);
  }else{
     digitalWrite(3, LOW);
  }

}

void funcion_4(bool fin_eje_inst){

  if(!fin_eje_inst){
    digitalWrite(2, HIGH);
  }else{
     digitalWrite(2, LOW);
  }

}



void ejecutar_columna_instrucciones(int *bloque, int columna, int *fila, unsigned long *tiempo_inicio_bloque, unsigned long *tiempo_fin_bloque, unsigned long *tiempo_eje_bloque, bool *ejecutada, unsigned long *tiempo_inicio_col,unsigned long *tiempo_fin_col, bool *tiempo_exed_col, unsigned long *tiempo_inicio_inst_col, unsigned long *tiempo_inst_col){

  void (*Funcionalidad)(bool) = NULL;
  Serial.print("/*-----COLUMNA ");
  Serial.print(columna);
  Serial.println("-----------*/");
  Serial.print("Tiempo inicio: ");
  Serial.println(*tiempo_inicio_bloque);

  if((*tiempo_fin_bloque) - (*tiempo_inicio_bloque) >= 0){//Si el tiempo transcurrido es mayor a 0 segundos

    if(!(*ejecutada)){ //Verica si no se ha ejecutado la instruccion de esta fila y la comienza a ejecutar
      Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][*fila][columna]])->Ptr_func;
      Funcionalidad(false);
      *ejecutada = true; //Indica que ya esta fila comenzo a ejecutarse
      *tiempo_inicio_inst_col = millis(); //almacena el tiempo en el que comenzo a ejecutarse la instruccion actual
      Serial.print("Tiempo en el que empezo a ejecutarse la instrucccion por primera vez->");
      Serial.println(*tiempo_inicio_inst_col);
    }else{
      *tiempo_fin_col = millis();
       Serial.print("Tiempo en el que empezo a ejecutarse la instrucccion->");
      Serial.println(*tiempo_inicio_inst_col);
        Serial.print("Tiempo actual transucrrio->");
      Serial.println(*tiempo_fin_col);

    


      if(!(*tiempo_exed_col)){
          Serial.print("Fila->");
      Serial.println(*fila);
        *tiempo_inst_col = funcionalidades[memoria_instrucciones[*bloque][*fila][columna]]->exec_time;
        Serial.print("Tiempo que dura la instruccion->");
        Serial.println(*tiempo_inst_col);
        Serial.print("Tiempo ejecutado de la columna");
        Serial.println((*tiempo_fin_col) - (*tiempo_inicio_col));
        if( ((*tiempo_fin_col) - (*tiempo_inicio_inst_col) + 500 >= *tiempo_inst_col)  && ((*tiempo_fin_col) - (*tiempo_inicio_col) + 500 <= *tiempo_eje_bloque) ){
          Serial.println("ENCCONTRO UN EXCEDENTE");
          *(tiempo_inst_col) += *tiempo_eje_bloque - (*tiempo_fin_col - *tiempo_inicio_col);
          //Serial.prin
          *tiempo_exed_col = true;
        }
      }
      //Si ya transcurrio el tiempo de ejecucion de la instruccion y es la ultima de la columna pero aun queda tiempo del bloque por ejecutarse
      
      //*tiempo_fin_col = millis();
      // Serial.print("Tiempo actual transucrrido 222->");
      
      if((*tiempo_fin_col) - (*tiempo_inicio_inst_col)+ 500 >= *tiempo_inst_col){ //Verifica si el tiempo transcurrido desde que comenzo a ejecutarse la instruccion es igual al establecido para su ejecucion
        Serial.println("SE CUMPLIO EL TIEMPO DE LA INSTRUCCION DESACTIVALA");
        Funcionalidad = (funcionalidades[memoria_instrucciones[*bloque][*fila][columna]])->Ptr_func;
        Funcionalidad(true);
        (*fila)++;// Incrementa la fila que esta recorriendo
        *ejecutada = false; //Indica que  esta fila termino de ejecutarse
        *tiempo_exed_col = false;
      }
    }
  }
}


unsigned long tiempo_inicio_programa = 0, tiempo_fin_programa = 0;

/**---FUNCIONES QUE SE EJECUTAN N VECES----*/
void loop(void) {

  unsigned long tiempo_eje_programa = 0, tiempo_eje_bloque_0 = 0, tiempo_eje_bloque_1 = 0, tiempo_eje_bloque_2 = 0,
    tiempo_eje_bloque_3 = 0, tiempo_eje_bloque_4 = 0, tiempo_inicio_bloque = 0, tiempo_fin_bloque = 0, tiempo_eje_bloque = 0; 
  
  unsigned long tiempo_inicio_col_0 = 0, tiempo_inicio_col_1 = 0, tiempo_inicio_col_2 = 0,tiempo_inicio_col_3 = 0,tiempo_inicio_col_4 = 0; 
  unsigned long tiempo_fin_col_0 = 0, tiempo_fin_col_1 = 0, tiempo_fin_col_2 = 0,tiempo_fin_col_3 = 0,tiempo_fin_col_4 = 0; 
  unsigned long tiempo_inicio_inst_col_0 = 0, tiempo_inicio_inst_col_1 = 0, tiempo_inicio_inst_col_2 = 0,tiempo_inicio_inst_col_3 = 0,tiempo_inicio_inst_col_4 = 0; 
  bool tiempo_exed_col_0 = false,  tiempo_exed_col_1 = false, tiempo_exed_col_2 = false ,tiempo_exed_col_3 = false ,tiempo_exed_col_4 = false;
  unsigned long tiempo_eje_inst = 0;

  int bloque = 0, fila_col_0 = 0, fila_col_1 = 0, fila_col_2 = 0, fila_col_3 = 0, fila_col_4 = 0;
  bool inicio_bloque = false, ejecutada_0 = false, ejecutada_1 = false, ejecutada_2 = false, ejecutada_3 = false, ejecutada_4 = false ;

  escanear_instrucciones();

  if(tiempos_eje_bloques == NULL){
    tiempos_eje_bloques = crear_arreglo_tiempos_ejecucion_bloques();
  }

  inicializar_arreglo_tiempos_ejecucion_bloques();
  tiempo_eje_bloque_0 = tiempo_duracion_bloque_instrucciones(0);
  tiempo_eje_bloque_1 = tiempo_duracion_bloque_instrucciones(1);
  tiempo_eje_bloque_2 = tiempo_duracion_bloque_instrucciones(2);
  tiempo_eje_bloque_3 = tiempo_duracion_bloque_instrucciones(3);
  tiempo_eje_bloque_4 = tiempo_duracion_bloque_instrucciones(4);

  establecer_tiempos_eje_arreglo_tiempos_bloques(tiempo_eje_bloque_0,tiempo_eje_bloque_1,tiempo_eje_bloque_2,tiempo_eje_bloque_3,tiempo_eje_bloque_4);
  
  tiempo_eje_programa = duracion_programa();
  Serial.print("----------Tiempo eje programa->");
  Serial.print(tiempo_eje_programa);

  while(volver_a_comenzar >= 0){

    Serial.print("**************ITERACION NRO: ");
    Serial.print(volver_a_comenzar);
    Serial.println("**************");
    volver_a_comenzar--;
    
    tiempo_inicio_programa = millis();
    tiempo_fin_programa = millis();

   while(tiempo_fin_programa - tiempo_inicio_programa <= tiempo_eje_programa){ //REPITE MIENTRAS EL TIEMPO TRANSCURRIDO SEA MENOR AL TIEMPO DE EJECUCION DEL PROGRAMA
    
      if (!inicio_bloque){
        inicio_bloque = true;
        tiempo_fin_bloque = millis();
        tiempo_inicio_bloque = millis();
        tiempo_inicio_col_0 = tiempo_inicio_bloque;
        tiempo_inicio_col_2 = tiempo_inicio_bloque;
        fila_col_0 = 0;
        fila_col_1 = 0;
        fila_col_2 = 0;
        fila_col_3 = 0;
        fila_col_4 = 0;
      }

      if(bloque == 0){
        tiempo_eje_bloque = tiempo_eje_bloque_0;
      }
      if(bloque == 1){
        tiempo_eje_bloque = tiempo_eje_bloque_1;
      }
      if(bloque == 2){
        tiempo_eje_bloque = tiempo_eje_bloque_2;
      }
      if(bloque == 3){
        tiempo_eje_bloque = tiempo_eje_bloque_3;
      }
      if(bloque == 4){
        tiempo_eje_bloque = tiempo_eje_bloque_4;
      }

      Serial.print("BLoque a Ejecutar->");
      Serial.println(bloque);
  
      if(memoria_instrucciones[bloque][0][0] != 0){
          ejecutar_columna_instrucciones(&bloque,0, &fila_col_0, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_0, &tiempo_inicio_col_0, &tiempo_fin_col_0, &tiempo_exed_col_0, &tiempo_inicio_inst_col_0, &tiempo_eje_inst);
      }   
      
     // ejecutar_columna_instrucciones(&bloque,1, &fila, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_1, &tiempo_inicio_col_1);
      if(memoria_instrucciones[bloque][0][2] != 0){
         ejecutar_columna_instrucciones(&bloque,2, &fila_col_2, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_2, &tiempo_inicio_col_2, &tiempo_fin_col_2, &tiempo_exed_col_2,&tiempo_inicio_inst_col_2, &tiempo_eje_inst);
      }
      /*ejecutar_columna_instrucciones(&bloque,3, &fila, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_3, &tiempo_inicio_col_3);
      ejecutar_columna_instrucciones(&bloque,4, &fila, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque, &ejecutada_4, &tiempo_inicio_col_4);*/

      tiempo_fin_bloque = millis();
      tiempo_fin_programa = millis();

      if(tiempo_fin_bloque - tiempo_inicio_bloque >= tiempo_eje_bloque){
        Serial.print("LLEGO EL FIN DE BLOQUE TIEMPO-->");
        Serial.println(tiempo_eje_bloque);
        inicio_bloque = false;
        bloque++;
  
      }
    } 
  }

  volver_a_comenzar = 0;
  inicializar_memoria();
  ejecutar_programa = false;

  delay(1000);
}
/*------------------------------------------*/




