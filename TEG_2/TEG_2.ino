#include "DFRobotDFPlayerMini.h"                                                                                                          /*-REALIZADO POR KHAREN URDANETA NIÑO-*/
#include "SoftwareSerial.h"
#include <Adafruit_NeoPixel.h>
#include <SdFat.h>
#include <UTFT.h>
#include <UTFT_SdRaw.h>
#include "LedControl.h"
#include <MemoryFree.h>
#include <avr/wdt.h>


#define SD_CHIP_SELECT  53 
SdFat sd;
const int addrL = 0;  // first LED matrix - Left robot eye
const int addrR = 1;  // second LED matrix - Right robot eye
UTFT myGLCD(CTE40,38,39,40,41);
UTFT_SdRaw myFiles(&myGLCD);
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


#define MAX_FUNCIONALIDADES 46
#define MAX_BLOQUES 5
#define MAX_COLUMNAS 5
#define MAX_FILAS 6
#define MAX_MOV_OJOS 6
#define COMENZAR_PROGRAMA 0  // Bit 0
#define FINALIZAR_PROGRAMA 1  // Bit 1
#define EJECUTAR_PROGRAMA 2   // Bit 2
#define VOZ_COMANDO 3         // Bit 3
#define LUCES_SEMAFORO 4      // Bit 4
#define LUZ_VERDE 5           // Bit 5
#define LUZ_ROJA 6            // Bit 6
#define LUZ_AMARILLA 7        // Bit 7

LedControl lc = LedControl(9, 11, 10, 2); //din , clk , cs

/*------------TIPOS DE DATOS-----------*/
typedef struct Funcionalidad {
	char UID[12];
	void *Ptr_func;
  uint8_t type;
  struct Funcionalidad *next_func; 
  unsigned long exec_time;
}Funcionalidades;

/*---------------------------VARIABLES GLOBALES-------------------*/
typedef struct {
    uint8_t memoria_instrucciones[MAX_BLOQUES][MAX_COLUMNAS][MAX_FILAS];
    uint8_t sincronizacion;
    uint8_t volver_a_comenzar;
    uint8_t num_bloques;
    uint8_t grabacion;
    uint8_t reproduccion;
    uint8_t flags;
    int selec_col[MAX_COLUMNAS];
    bool turnos_ojos[MAX_MOV_OJOS];
    unsigned long tiempo_inicio_programa;
    unsigned long tiempo_fin_programa;
    unsigned long tiempos_previos_ojos[MAX_MOV_OJOS];
    unsigned long tiempos_eje_bloques[MAX_BLOQUES];
} Sistema;

typedef struct {
  uint32_t tiempo_inicio;
  uint32_t tiempo_fin;
  uint32_t tiempo_ejecucion;
  uint32_t tiempo_inicio_inst;
  uint32_t tiempo_eje_inst;
  bool tiempo_excedido;
  bool ejecutada;
  bool vacia;
  uint8_t fila;
} Columna;

 const char *imagenes_tarjetas[]  = {
    "tag_sincro.RAW", "tag_volver_comenzar.RAW", "tag_comenzar_programa.RAW", "tag_finalizar_programa.RAW", 
    "tag_borrar_inst.RAW", "tag_eje_pro.RAW", "tag_reset_pro.RAW", "tag_grabar_inst.RAW",
    "tag_mover_cab_izq.RAW", "tag_mover_cab_der.RAW", "tag_agitar_cola.RAW", "tag_avanzar.RAW",
    "tag_girar_der.RAW", "tag_girar_sobre_si.RAW", "tag_volver_der.RAW", "tag_retroceder.RAW",
    "tag_girar_izq.RAW", "tag_evitar_obs.RAW", "tag_volver_izq.RAW", "tag_encender_leds.RAW",
    "tag_cerrar_ojos.RAW", nullptr, nullptr, nullptr, nullptr, nullptr,
    "tag_pestanar.RAW", "tag_grabar_audio.RAW", "tag_repro_audio.RAW", "tag_emitir_sonido.RAW",
    nullptr, nullptr, nullptr, nullptr, nullptr, "tag_ir_siguiente_bloque.RAW",
    "tag_ir_bloque_anterior.RAW", nullptr, nullptr, nullptr
  };
/*-----------------------------------------------------------------*/

Funcionalidad funcionalidades[MAX_FUNCIONALIDADES];
Sistema sistema;
Columna columnas[MAX_COLUMNAS];

/*********************************************************************FUNCIONES DE LOGICA DE INSTRUCCIONES Y ALMACENAMIENTO*********************************************************************/

/*----CREA UNA NUEVA FUNCIONALIDAD PARTIENDO DE SU UID,  EL PUNTERO A LA FUNCION QUE LE CORRESPONDE Y SU TIEMPO DE EJECUCION----*/
Funcionalidad crear_nueva_funcionalidad(char *UID, void (*ptr_func)(), uint8_t type, Funcionalidad *next_func, unsigned long exec_time) {
  Funcionalidad f;
  strcpy(f.UID, UID); // Copia el UID
  f.Ptr_func = ptr_func;
  f.type = type;
  f.next_func = next_func;
  f.exec_time = exec_time;
  return f;
}
/*-------------------------------------------------------------------------------------------------*/

/*---CREA UN ARREGLO CON LAS FUNCIONALIDADES ESTABLECIDAS Y LO INICIALIZA CON SUS RESPECTIVOS VALORES---*/
void crear_arreglo_funcionalidades(){

  funcionalidades[0] = crear_nueva_funcionalidad("B3 54 7A 12", &prueba, 0, NULL, 0); // SINCRONIZACION
  funcionalidades[1] = crear_nueva_funcionalidad("E3 F7 A7 12", &prueba, 0, NULL, 0); // VOLVER A COMENZAR
  funcionalidades[2] = crear_nueva_funcionalidad("A3 CE 89 94", &prueba, 0, NULL, 0); // COMENZAR PROGRAMA
  funcionalidades[3] = crear_nueva_funcionalidad("43 26 C4 12", &prueba, 0, NULL, 0); // FINALIZAR PROGRAMA
  funcionalidades[4] = crear_nueva_funcionalidad("A3 3E 72 94", &prueba, 0, NULL, 0); // BORRAR INSTRUCCION
  funcionalidades[5] = crear_nueva_funcionalidad("E3 FC B3 12", &prueba, 0, NULL, 0); // EJECUTAR PROGRAMA
  funcionalidades[6] = crear_nueva_funcionalidad("93 02 86 94", &prueba, 0, NULL, 0); // RESETEAR PROGRAMA
  funcionalidades[7] = crear_nueva_funcionalidad("13 7C 72 94", &prueba, 0, NULL, 0); // INSTRUCCIONES POR COMANDO DE VOZ
  funcionalidades[8] = crear_nueva_funcionalidad("53 12 73 94", &prueba, 3, NULL, 3000); // MOVER CABEZA A LA IZQUIERDA
  funcionalidades[9] = crear_nueva_funcionalidad("F3 94 8B 94", &prueba, 3, NULL, 3000); // MOVER CABEZA A LA DERECHA
  funcionalidades[10] = crear_nueva_funcionalidad("33 22 B7 94", &prueba, 3, NULL, 5000); // AGITAR COLA
  funcionalidades[11] = crear_nueva_funcionalidad("83 0E AA 12", &prueba, 4, NULL, 5000); // AVANZAR
  funcionalidades[12] = crear_nueva_funcionalidad("13 45 8A 94", &prueba, 4, NULL, 1350); // GIRAR A LA DERECHA
  funcionalidades[13] = crear_nueva_funcionalidad("D3 DF 81 94", &prueba, 4, NULL, 5400); // GIRAR SOBRE SI MISMO
  funcionalidades[14] = crear_nueva_funcionalidad("B3 23 8F 94", &prueba, 4, NULL, 2700); // VOLVER POR LA DERECHA
  funcionalidades[15] = crear_nueva_funcionalidad("13 63 6B 94", &prueba, 4, NULL, 5000); // RETROCEDER
  funcionalidades[16] = crear_nueva_funcionalidad("83 11 6B 94", &prueba, 4, NULL, 1350); // GIRAR A LA IZQUIERDA
  funcionalidades[17] = crear_nueva_funcionalidad("33 09 BB 94", &prueba, 4, NULL, 15000); // EVITAR OBSTACULOS
  funcionalidades[18] = crear_nueva_funcionalidad("93 E2 20 95", &prueba, 4, NULL, 2700); // VOLVER POR LA IZQUIERDA
  funcionalidades[19] = crear_nueva_funcionalidad("23 DE 6C 94", &prueba, 5, NULL, 5000); // ENCENDER LUCES SEMAFORO
  funcionalidades[20] = crear_nueva_funcionalidad("93 D3 80 94", &prueba, 6, NULL, 6000); // CERRAR OJOS
  funcionalidades[21] = crear_nueva_funcionalidad("C3 7B 0A 95", &prueba, 6, NULL, 5000); // MIRAR SORPRENDIDO
  funcionalidades[22] = crear_nueva_funcionalidad("53 5A 84 94", &prueba, 6, NULL, 6000); // MIRAR FELIZ
  funcionalidades[23] = crear_nueva_funcionalidad("53 CF 25 95", &prueba, 6, NULL, 10000); // MIRAR TRISTE
  funcionalidades[24] = crear_nueva_funcionalidad("63 84 6F 94", &prueba, 6, NULL, 10000); // MIRAR ENOJADO
  funcionalidades[25] = crear_nueva_funcionalidad("03 F2 D3 A8", &prueba, 6, NULL, 6000); // MIRAR ENAMORADO
  funcionalidades[26] = crear_nueva_funcionalidad("63 36 C6 94", &prueba, 6, NULL, 10000); // PESTANEAR
  funcionalidades[27] = crear_nueva_funcionalidad("D3 12 74 94", &prueba, 7, NULL, 10000); // GRABAR AUDIO
  funcionalidades[28] = crear_nueva_funcionalidad("53 2D 89 94", &reproducir_grabacion, 7, NULL, 10000); // REPRODUCIR GRABACION
  funcionalidades[29] = crear_nueva_funcionalidad("A3 7A CB 94", &emitir_sonido, 7, NULL, 10000); // EMITIR SONIDO
  funcionalidades[30] = crear_nueva_funcionalidad("E3 F3 F0 94", &prueba, 5, NULL, 5000); // BLANCO
  funcionalidades[31] = crear_nueva_funcionalidad("73 4B AA 94", &prueba, 5, NULL, 5000); // NARANJA
  funcionalidades[32] = crear_nueva_funcionalidad("83 9D 6D 94", &prueba, 5, NULL, 5000); // AMARILLO
  funcionalidades[33] = crear_nueva_funcionalidad("E3 D3 6B 12", &prueba, 5, NULL, 5000); // ROJO
  funcionalidades[34] = crear_nueva_funcionalidad("B3 05 6E 12", &prueba, 5, NULL, 5000); // AZUL
  funcionalidades[35] = crear_nueva_funcionalidad("F3 43 C6 12", &prueba, 5, NULL, 5000); // MORADO
  funcionalidades[36] = crear_nueva_funcionalidad("93 F1 C7 12", &prueba, 5, NULL, 5000); // VERDE
  funcionalidades[37] = crear_nueva_funcionalidad("83 75 A9 94", &prueba, 5, NULL, 5000); // ROSA
  funcionalidades[38] = crear_nueva_funcionalidad("A3 D2 B7 12", &prueba, 2, NULL, 0); // SIGUIENTE BLOQUE
  funcionalidades[39] = crear_nueva_funcionalidad("E3 00 69 12", &prueba, 2, NULL, 0); // ANTERIOR BLOQUE
  funcionalidades[40] = crear_nueva_funcionalidad("43 FB 27 A7", &prueba, 2, NULL, 0); // SELECCIONAR COLUMNA 0
  funcionalidades[41] = crear_nueva_funcionalidad("D3 60 39 A7", &prueba, 2, NULL, 0); // SELECCIONAR COLUMNA 1
  funcionalidades[42] = crear_nueva_funcionalidad("33 23 98 A7", &prueba, 2, NULL, 0); // SELECCIONAR COLUMNA 2
  funcionalidades[43] = crear_nueva_funcionalidad("53 3F 92 A7", &prueba, 2, NULL, 0); // SELECCIONAR COLUMNA 3
  funcionalidades[44] = crear_nueva_funcionalidad("E3 16 67 A7", &prueba, 2, NULL, 0); // SELECCIONAR COLUMNA 4
  funcionalidades[45] = crear_nueva_funcionalidad("E3 DC 14 A7", &prueba, 0, NULL, 0); // BORRAR BLOQUE INSTRUCCIONES

}
/*-------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------*/

/*------INICIALIZA EL ARREGLO(MEMORIA) QUE SE ENCARGA DE ALMACENAR LAS FUNCIONALIDADES A EJECUTAR EN EL PROGRAMA----*/
void inicializar_memoria() {
  for (uint8_t dim = 0; dim < MAX_BLOQUES; dim++) {
    for (uint8_t colum = 0; colum < MAX_COLUMNAS; colum++) {
      for (uint8_t fila = 0; fila < MAX_FILAS; fila++) {
        sistema.memoria_instrucciones[dim][colum][fila] = 0;
      }
    }
  }
}
/*----------------------------------------------------------------------------------------------------------*/

/*---VERIFICA SI UN BLOQUE TIENE INSTRUCCIONES ALMACENADAS. DEVUELVE TRUE EN CASO DE QUE SE ENCUENTRE VACIO---*/
bool verificar_bloque_instrucciones_vacio(uint8_t bloque){ //devuelve 1 si se encuentra vacio, 0 si esta lleno
  uint8_t vacio = 0;

  for(uint8_t colum = 0 ; colum < MAX_COLUMNAS ; colum++){
    if(sistema.memoria_instrucciones[bloque][colum][0] == 0){ //SI LO QUE ESTA EN LA PRIMERA POSICION DE LA COLUMNA ES CERO
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
int almacenar_instruccion(const char *UID){
  int tipo_inst = -1;
  bool cursor = false;
  char UID_aux[15];
  Serial.print("Tamano UID-->");
  Serial.println(strlen(UID) + 1);
  Serial.println("ENTRA EN ALMACENAR INSTRUCCION");
  for(uint8_t i = 0; i < MAX_FUNCIONALIDADES ; i++){ //REVISA TODAS LAS FUNCIONALIDADES
    strncpy(UID_aux, funcionalidades[i].UID, sizeof(UID_aux) - 1);
    UID_aux[sizeof(UID_aux) - 1] = '\0';  // Asegurar el carácter nulo al final
    //Serial.print("Memoria libre: ");
    //Serial.println(freeMemory());
    if(strcmp(UID,UID_aux) == 0){  //SI ENCUENTRA LA INSTRUCCION
      imprimir_imagen_tarjeta(i);
      tipo_inst = funcionalidades[i].type;
      Serial.println("ENCONTRO TAG EN ALMACENAMIENTO FUNCTION");
      if((strcmp(UID,"B3 54 7A 12") == 0) && !verificar_bloque_instrucciones_vacio(0) && sistema.num_bloques < 5 ){ //SI SE HA ESCANEADO TAG SINCRONIZACION , EL BLOQUE NO ESTA VACIO Y EL NUMERO DE BLOQUES ES MENOR QUE 5
     //SE PASA AL SIGUIENTE BLOQUE (SINCRONIZACION == BLOQUE)
        sistema.num_bloques++;
        sistema.sincronizacion = sistema.num_bloques;
        Serial1.print("SINCRONIZACION_");
      }
      if(strcmp(UID,"23 DE 6C 94") == 0){
        activarBandera(LUCES_SEMAFORO);
      }
      
      if(strcmp(UID,"D3 12 74 94") == 0){ //SI SE HA ESCANEADO TAG GRABAR AUDIO
        Serial.println("ENCONTRO TAG GRABAR");
        if((sistema.num_bloques == 0 && sistema.grabacion <= 6) || (sistema.num_bloques == 1 && sistema.grabacion <= 12) || (sistema.num_bloques == 2 && sistema.grabacion <= 18) || (sistema.num_bloques == 3 && sistema.grabacion <= 24) || (sistema.num_bloques == 4 && sistema.grabacion <= 30)){
          tipo_inst = 0;
          sistema.grabacion++;
          Serial.println("SE ENCIENDEN LUCES AZULES");
          Serial1.print("GRABAR-AUDIO_");
          delay(10000); 
        }
      }

      if((strcmp(UID,"53 2D 89 94") == 0) && sistema.grabacion > sistema.reproduccion){ //SI SE HA ESCANEADO TAG REPRODUCIR AUDIO Y  HAY GRABACION PREVIA
        tipo_inst = 0;
        sistema.reproduccion++;
        Serial.println("HAY GRABACION PARA SER REPRODUCIDA");
        delay(10000);
      }

      if(verificar_instruccion_seleccionada(UID, "43 FB 27 A7", 0, &cursor) == 1){
       Serial.println("SE SELECCIOONO INST EN COLUM 0");
        sistema.selec_col[1] = -1;
        sistema.selec_col[2] = -1;
        sistema.selec_col[3] = -1;
        sistema.selec_col[4] = -1;
      }
      if(verificar_instruccion_seleccionada(UID, "D3 60 39 A7",1, &cursor) == 1){
        
        Serial.println("SE SELECCIOONO INST EN COLUM 1");
        sistema.selec_col[0] = -1;
        sistema.selec_col[2] = -1;
        sistema.selec_col[3] = -1;
        sistema.selec_col[4] = -1;
      }
      
      if(verificar_instruccion_seleccionada(UID, "33 23 98 A7",  2, &cursor) == 1){
         Serial.println("SE SELECCIOONO INST EN COLUM 2");
        sistema.selec_col[0] = -1;
        sistema.selec_col[1] = -1;
        sistema.selec_col[3] = -1;
        sistema.selec_col[4] = -1;
      }

      if(verificar_instruccion_seleccionada(UID, "53 3F 92 A7",  3, &cursor) == 1){
         Serial.println("SE SELECCIOONO INST EN COLUM 3");
        sistema.selec_col[0] = -1;
        sistema.selec_col[1] = -1;
        sistema.selec_col[2] = -1;
        sistema.selec_col[4] = -1;
      }
      if(verificar_instruccion_seleccionada(UID, "E3 16 67 A7", 4, &cursor) == 1){
         Serial.println("SE SELECCIOONO INST EN COLUM 4");
        sistema.selec_col[0] = -1;
        sistema.selec_col[1] = -1;
        sistema.selec_col[2] = -1;
        sistema.selec_col[3] = -1;      
      }


      if((strcmp(UID,"E3 00 69 12") == 0) && sistema.sincronizacion > 0 ){ //si verificaste que hay un bloque antes
        sistema.sincronizacion--;
        Serial1.print("BLOQUE-ANTERIOR_");
      }

      if((strcmp(UID,"A3 D2 B7 12") == 0) && sistema.sincronizacion < sistema.num_bloques ){ //si verificaste que hay un bloque despues
        sistema.sincronizacion++;
        Serial1.print("SIGUIENTE-BLOQUE_");
      }

      if((strcmp(UID,"A3 3E 72 94") == 0)){ //SI SE HA ESCANEADO BORRAR INSTRUCCION
        Serial.println("SE ESCANEO TAG BORRAR INSTRUCCCION");
        Serial.print("SELEC_COL_0-->");
          Serial.println(sistema.selec_col[0]);
        if(sistema.selec_col[0] != -1){
         // Serial.print("SELEC_COL_0-->");
          //Serial.println(*selec_col_0);
          eliminar_instruccion(0, sistema.selec_col[0], sistema.sincronizacion);
          sistema.selec_col[0] = -1;
        }

        if(sistema.selec_col[1] != -1){
          eliminar_instruccion(1, sistema.selec_col[1], sistema.sincronizacion);
          sistema.selec_col[1] = -1;
        }

        if(sistema.selec_col[2] != -1){
          eliminar_instruccion(2, sistema.selec_col[2], sistema.sincronizacion);
          sistema.selec_col[2] = -1;
        }
        if(sistema.selec_col[3] != -1){
          eliminar_instruccion(3, sistema.selec_col[3], sistema.sincronizacion);
          sistema.selec_col[3] = -1;
        }
        if(sistema.selec_col[4] != -1){
          eliminar_instruccion(4, sistema.selec_col[4], sistema.sincronizacion);
          sistema.selec_col[4] = -1;
        } 
      } 

      if(strcmp(UID,"E3 DC 14 A7") == 0 ){ 
        if(eliminar_bloque_instrucciones(sistema.sincronizacion) != 0){//si esta vacio no hagas nada
          Serial.println("BLOQUE ELIMINADO");
          Serial1.print("BLOQUE-ELIMINADO_");
        }
      }

      if(strcmp(UID,"93 02 86 94") == 0){
        resetear_programa();
       Serial.println("RESETEADO EL PROGRAMA");
       Serial1.print("PROGRAMA-RESETEADO_");
      }

      switch(tipo_inst){
        case 3: {
          introducir_inst_columna_memoria(i,0,sistema.sincronizacion);
          break;
        }
        case 4: {
          introducir_inst_columna_memoria(i,1,sistema.sincronizacion);
          break;
        }
        case 5: {
          introducir_inst_columna_memoria(i,2,sistema.sincronizacion);
          break;
        }
        case 6: {
          introducir_inst_columna_memoria(i,3,sistema.sincronizacion);
          break;
        }
        case 7:{
          introducir_inst_columna_memoria(i,4,sistema.sincronizacion);
          break;
        }
      }
      
      if(!cursor){
        imprimir_pantalla_matriz_funcionalidades(sistema.sincronizacion);
      }
      
      return 1;
    } 
  }
return 0;
}
/*----------------------------------------------------------------------------------------------*/


/*---INTRODUCE EL INDICE DE UNA FUNCIONALIDAD EN LA MEMORIA DE INSTRUCCIONES DADO SU INDICE, LA COLUMNA Y EL BLOQUE DONDE SERA INTRODUCIDA---*/
void introducir_inst_columna_memoria(uint8_t indice_func, uint8_t colum, uint8_t dim){
  bool almacenado = false;

  for( uint8_t fila = 0 ; fila < MAX_FILAS ; fila++){
    if(sistema.memoria_instrucciones[dim][colum][fila] == 0){
      almacenado = true;
      sistema.memoria_instrucciones[dim][colum][fila] = indice_func;
      return;
    } 
  }   
}
/*-------------------------------------------------------------------------------------------------------------------------------------------*/

/*-----------ESCANEA LAS TARJETAS RECIBIENDO COMO PARAMETROS LOS TIEMPOS Y LOS TURNOS DE OJOS PARA CONTROLAR SU ANIMACION-------------*/
void escanear_instrucciones(const char *ptrUID){
  Serial.print("MEMORIA LIBREEEEEEEEEEEEEEEE: ");
  Serial.println(freeMemory());
  //delay(1000); // Espera 1 segundo
  Serial.print("MENSAJE 333333333333333333-->");
  Serial.println(ptrUID);
  

  myFiles.load(5, 0, 310, 480, "tarjeta_escaneada.RAW", 1 , 0);
  Serial.print("Memoria libre222: ");
  Serial.println(freeMemory());
  delay(1000); // Espera 1 segundo
  Serial.print("MENSAJE 444444444444444444444-->");
  Serial.println(ptrUID);
  
  if((strcmp(ptrUID,"13 7C 72 94" ) == 0) && !esBanderaActiva(FINALIZAR_PROGRAMA) && esBanderaActiva(COMENZAR_PROGRAMA) ){ //SI SE HA ESCANEADO TAG COMANDO VOZ Y ESTA AUN NO HA SIDO ESCANEADA PERO YA SE COMENZO LA ESCRITURA DE INSTRUCCIONES FINALIZALA
    Serial.println("ENTRO AQUI 0");
    myFiles.load(5, 0, 310, 480, "comando_voz.RAW", 1 , 0);
    activarBandera(VOZ_COMANDO);
    Serial1.print("FINALIZO-ESCANEAR-TAG_");
    return;
  }

  if((strcmp(ptrUID, "A3 CE 89 94") == 0) && !esBanderaActiva(COMENZAR_PROGRAMA)){ //SI SE HA ESCANEADO TAG COMENZAR PROGRAMA Y ESTA NO HA SIDO ESCANEADA SE COMIENZA LA ESCRITURA DEL MISMO 
    Serial.println("ENTRO AQUI 2");
    activarBandera(COMENZAR_PROGRAMA);
    imprimir_imagen_tarjeta(2);
    Serial1.print("FINALIZO-ESCANEAR-TAG_");
    return;
  }

  if((strcmp(ptrUID,"43 26 C4 12") == 0) && !esBanderaActiva(FINALIZAR_PROGRAMA) && esBanderaActiva(COMENZAR_PROGRAMA)){ //SI SE HA ESCANEADO TAG FINALIZAR PROGRAMA Y ESTA AUN NO HA SIDO ESCANEADA PERO YA SE COMENZO LA ESCRITURA DE INSTRUCCIONES FINALIZALA
    Serial.println("ENTRO AQUI 3");
    activarBandera(FINALIZAR_PROGRAMA);
    Serial1.print("FINALIZO-ESCANEAR-TAG_");
    return;
  } 

  if((strcmp(ptrUID,"E3 F7 A7 12") == 0) && esBanderaActiva(COMENZAR_PROGRAMA) && esBanderaActiva(FINALIZAR_PROGRAMA)){ //SI SE HA ESCANEADO LA TAG VOLVER A COMENZAR Y YA SE REALIZO LA ESCRITURA CORRESPONDIENTE DE INSTRUCCIONES INDICA NUEVA ITERACION
    Serial.println("ENTRO AQUI 4");
    sistema.volver_a_comenzar++; //indica el numero de iteraciones a realizar
    imprimir_imagen_tarjeta(1);
    Serial1.print("FINALIZO-ESCANEAR-TAG_");
    return;
  }

  if(esBanderaActiva(COMENZAR_PROGRAMA) && !esBanderaActiva(FINALIZAR_PROGRAMA)){ //SI SE HA INICIADO LA ESCRITURA DEL PROGRAMA Y NO SE HA FINALIZADO COMIENZA A ALMACENAR LAS PROXIMAS TAG ESCANEADAS EN MEMORIA
    almacenar_instruccion(ptrUID);
    Serial.println("ENTRO AQUI TAMBIEN");
    imprimir_matriz(); 
    Serial1.print("FINALIZO-ESCANEAR-TAG_");
    return;
  }
      
  if((strcmp(ptrUID,"E3 FC B3 12") == 0) && esBanderaActiva(COMENZAR_PROGRAMA) && esBanderaActiva(FINALIZAR_PROGRAMA)){ //SI SE HA ESCANEADO TAG EJECUTAR PROGRAMA Y YA SE REALIZO LA ESCRITURA CORRESPONDIENTE DE INSTRUCCIONES
    Serial.println("ENTRO AQUI 5");
    activarBandera(EJECUTAR_PROGRAMA);
    desactivarBandera(COMENZAR_PROGRAMA);
    desactivarBandera(FINALIZAR_PROGRAMA);
    Serial1.print("TAG-EJECUTAR-PROGRAMA_");
    return;
    
  }
  //Serial1.print("FINALIZO-ESCANEAR-TAG_");
}
/*-------------------------------------------------------------------------------------------------------------------------------*/

/*---VERIFICA SI SE HA SELECCIONADO UNA INSTRUCCION EN UNA COLUMNA DADO EL UID DE LA TARJETA JUNTO CON EL UID A COMPARAR, LA POSICION DE LA FUNCIONALIDAD EN LA MATRIZ Y EL ESTADO DEL CURSOR ------*/
int verificar_instruccion_seleccionada(const char *UID, char *UID_aux, int colum, bool *cursor){

  Serial.println("ENTRA EN VERIFICAR INSTR SELECCIONADA");
  if((strcmp(UID,UID_aux) == 0)){
    Serial.println("HOLA MUNDO 0");
    *cursor = true;
    if(sistema.memoria_instrucciones[sistema.sincronizacion][colum][0] == 0){
      Serial.println("HOLA MUNDO 1");
      imprimir_pantalla_matriz_funcionalidades(sistema.sincronizacion);
      return 1;
    }
   
    Serial.print("COLUM SELECTOR-->");
    Serial.print(sistema.selec_col[colum]);
    //(*selec_col)++;
    (sistema.selec_col[colum])++;

    if(sistema.memoria_instrucciones[sistema.sincronizacion][colum][sistema.selec_col[colum]] != 0){
      imprimir_pantalla_matriz_funcionalidades(sistema.sincronizacion);
      imprimir_funcion_selec_columna_matriz_funcionalidades(sistema.sincronizacion, colum , sistema.selec_col[colum], true); //SELECCIONA EL ELEMENTO INDICADO
      Serial.println("HOLA MUNDO 2");
    Serial.print("COLUM SELECTOR-->");
    Serial.print(sistema.selec_col[colum]);
      return 1;
    }else{
      sistema.selec_col[colum] = -1;
      imprimir_pantalla_matriz_funcionalidades(sistema.sincronizacion);
      Serial.println("HOLA MUNDO 3");
      return 1;
    }
  }
  Serial.println("HOLA MUNDO 4");
return 0;
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*---IMPRIME LA MEMORIA DE INSTRUCCIONES A EJECUTAR EN EL PROGRAMA EN EL SERIAL MONITOR---*/
void imprimir_matriz(){

  for( uint8_t dim = 0 ; dim < MAX_BLOQUES ; dim++){
    Serial.print("/----DIMENSION----/: ");
    Serial.println(dim,1);
    for(uint8_t filas = 0 ; filas < MAX_FILAS ; filas++){
      Serial.print("\n");
      for(uint8_t colum = 0 ; colum < MAX_COLUMNAS ; colum++){
        Serial.print("\t");
        Serial.print( sistema.memoria_instrucciones[dim][colum][filas],1);
        Serial.print("\t");
      }
    }
    Serial.println();
  }
}
/*------------------------------------------------------------------------------------*/


/*******************************FUNCIONALIDADES ASOCIADAS A LA EJECUCION DEL PROGRAMA***********************************/
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

void ejecutar_columna_instrucciones(uint8_t *bloque, uint8_t columna, unsigned long *tiempo_inicio_bloque, unsigned long *tiempo_fin_bloque, unsigned long *tiempo_eje_bloque){

  void (*Funcionalidad)(bool) = NULL;
  char ptrUID[15];
  
  Serial.print("COLUMNA: ");
  Serial.println(columna);

  if((*tiempo_fin_bloque) - (*tiempo_inicio_bloque) >= 0){//VERIFICA SI EL TIEMPO TRANSCURRIDO ES MAYOR A 0 MICROSEGUNDOS
    if(sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila] != 0){ //VERIFICA SI EXISTE UNA INSTRUCCION PARA EJECUTAR
      if(!(columnas[columna].ejecutada) ){ //VERIFICA SI NO SE HA EJECUTADO LA INSTRUCCION
        Funcionalidad = (funcionalidades[sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila]]).Ptr_func;
        strcpy(ptrUID,(funcionalidades[sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila]]).UID);

        if(columna != 3 && columna != 2){
          Funcionalidad(false);
        }else{
          if(columna == 3){
            inicializar_turnos_ojos();
            inicializar_tiempos_ojos();
            realizar_movimiento_ojos(ptrUID, 1);   
          }
          if(columna == 2) {
            if(strcmp(ptrUID, "23 DE 6C 94") == 0){
              Serial.println("DETECTAMOS EN EJECUCION TAG ENCENDER LUCES SEMAFORO");
              activarBandera(LUCES_SEMAFORO);
              //encender_luces_semaforo(false);
            }else{
              desactivarBandera(LUCES_SEMAFORO);
             // encender_luces(false, ptrUID);
            }
          }
        }
        columnas[columna].ejecutada = true; 

        if((columnas[columna].fila) == 0){ //VERIFICA SI ES LA PRIMERA INSTRUCCION DE LA COLUMNA DADA
          columnas[columna].tiempo_inicio_inst = *tiempo_inicio_bloque;
        }else{
          columnas[columna].tiempo_inicio_inst = millis(); 
        }

      }else{ //SI LA INSTRUCCION YA FUE EJECUTADA
        columnas[columna].tiempo_fin = millis();
        
        if(columna == 2 && esBanderaActiva(LUCES_SEMAFORO)){
          //encender_luces_semaforo(false);
          Serial.println("ENCENDER LUCES SEMAFORO");
        }

        if(columna == 3){
          strcpy(ptrUID,(funcionalidades[sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila]]).UID);
          realizar_movimiento_ojos(ptrUID, 2);
        }

        if(!(columnas[columna].tiempo_excedido)){ //VERIFICA SI AUN NO HAY TIEMPO EXCENDENTE EN ESTA COLUMNA 
          columnas[columna].tiempo_eje_inst = funcionalidades[sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila]].exec_time;
      
          //VERIFICA SI AUN NO SE HA ESTABLECIDO EL TIEMPO DE ESTA COLUMNA Y LO ESTABLECE
          if((columnas[columna].tiempo_ejecucion) == 0){
            columnas[columna].tiempo_ejecucion = tiempo_eje_columna_instrucciones(*bloque, columna);
          }

          //VERIFICA SI YA SE CUMPLIO EL TIEMPO DE LA INSTRUCCION, SI A ESE BLOQUE AUN LE QUEDAN INSTRUCCIONES POR EJECUTAR Y SI EL TIEMPO DE EJECUCION DE LA COLUMNA LLEGO A SU FIN
          if(((columnas[columna].tiempo_fin) - (columnas[columna].tiempo_inicio)  >= (columnas[columna].tiempo_ejecucion)) && ((columnas[columna].tiempo_fin) - (columnas[columna].tiempo_inicio)  < *tiempo_eje_bloque) ){
            columnas[columna].tiempo_eje_inst += *tiempo_eje_bloque - (columnas[columna].tiempo_fin - columnas[columna].tiempo_inicio);
            columnas[columna].tiempo_excedido = true;
          }
        } 
        
        //VERIFICA SI EL TIEMPO TRANSCURRIDO DESDE QUE COMENZO A EJECUTARSE LA INSTRUCCION ES IGUAL AL ESTABLECIDO PARA SU EJECUCION O SI EL TIEMPO ESTABLECIDO PARA LA EJECUCION DE LA COLUMNA YA SE CUMPLIO.
        if(((columnas[columna].tiempo_fin) - (columnas[columna].tiempo_inicio_inst)  >= columnas[columna].tiempo_eje_inst) || ((columnas[columna].tiempo_fin) - (columnas[columna].tiempo_inicio)  >= *tiempo_eje_bloque) ){ 
          Funcionalidad = (funcionalidades[sistema.memoria_instrucciones[*bloque][columna][columnas[columna].fila]]).Ptr_func;
          Serial.println("FINALIZO LA INSTRUCCION");
          if(columna != 3 && columna != 2){
            Funcionalidad(true); //DESACTIVA LA INSTRUCCION
          }

          if(columna == 2){
            if(esBanderaActiva(LUCES_SEMAFORO)){
              //Serial.println("ENCENDER LUCES SEMAFORO");
              //encender_luces_semaforo(true);
              desactivarBandera(LUCES_SEMAFORO);
            }else{
              //encender_luces(true, ptrUID);
              //Serial.println("APAGAR LUCES SEMAFORO");
            }
          } 

          if(columnas[ columna].fila < MAX_FILAS){
            (columnas[columna].fila)++;// INCREMENTA LA FILA A RECORRER
          }
          
          columnas[columna].ejecutada = false; 
          columnas[columna].tiempo_excedido = false;
        }
      }   
    }
  }
}
/*------------------------------------------------------------------------------------*/

/*---DETERMINA EL TIEMPO DE EJECUCION DE UNA COLUMNA DADO EL BLOQUE DE INSTRUCCIONES AL QUE PERTENECE---*/
unsigned long tiempo_eje_columna_instrucciones(uint8_t bloque, uint8_t colum){
  unsigned long duracion = 0, duracion_max = 0;
    
    for(uint8_t fila = 0; fila < MAX_FILAS; fila++){
      if(sistema.memoria_instrucciones[bloque][colum][fila] != 0){
        duracion += funcionalidades[sistema.memoria_instrucciones[bloque][colum][fila]].exec_time;
      }else{
        break;
      }
    }
return duracion;
}
/*--------------------------------------------------------------------------------------------------------*/

/*---DETERMINA EL TIEMPO MAXIMO QUE DURA UN BLOQUE PARTIENDO DE LA COLUMNA QUE TARDA MAS EN EJECUTARSE---*/
unsigned long tiempo_duracion_bloque_instrucciones(uint8_t bloque){ 
  unsigned long duracion = 0, duracion_max = 0;

  for(uint8_t colum = 0 ; colum < MAX_COLUMNAS; colum++){
    duracion = 0;
    for(uint8_t fila = 0; fila < MAX_FILAS; fila++){
      if(sistema.memoria_instrucciones[bloque][colum][fila] != 0){
        duracion += funcionalidades[sistema.memoria_instrucciones[bloque][colum][fila]].exec_time;
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

  for(uint8_t i = 0; i< MAX_BLOQUES ; i++){
    tiempo_eje_programa += sistema.tiempos_eje_bloques[i];
    Serial.println(sistema.tiempos_eje_bloques[i]);
  }

return tiempo_eje_programa;
}
/*------------------------------------------------------------------------------------------------*/

/*---INICIALIZA EL ARREGLO QUE ALMACENA LOS TIEMPOS DE EJECUCION DE CADA BLOQUE---*/
void inicializar_arreglo_tiempos_ejecucion_bloques(){ 

  for(uint8_t i = 0; i < MAX_BLOQUES; i++){
    sistema.tiempos_eje_bloques[i] = 0;
  }
}
/*--------------------------------------------------------------------------------*/

/*--ESTABLECE LOS TIEMPOS DE EJECUCION DE CADA BLOQUE DE INSTRUCCIONES EN EL ARREGLO QUE LOS ALMACENA--*/
void establecer_tiempos_eje_arreglo_tiempos_bloques(unsigned long tiempo_bloque_0,unsigned long tiempo_bloque_1,unsigned long tiempo_bloque_2, unsigned long tiempo_bloque_3, unsigned long tiempo_bloque_4){

  sistema.tiempos_eje_bloques[0] = tiempo_bloque_0;
  sistema.tiempos_eje_bloques[1] = tiempo_bloque_1;
  sistema.tiempos_eje_bloques[2] = tiempo_bloque_2;
  sistema.tiempos_eje_bloques[3] = tiempo_bloque_3;
  sistema.tiempos_eje_bloques[4] = tiempo_bloque_4;
}
/*----------------------------------------------------------------------------------------------------*/

/***********************************************FUNCIONALIDADES ADMINISTRATIVAS*****************************************/

/*------ELIMINA UNA INSTRUCCION DE LA MEMORIA DADO EL BLOQUE, LA COLUMNA Y LA FILA DONDE ESTA SE UBICA-----*/
void eliminar_instruccion(uint8_t colum, uint8_t fila, uint8_t bloque){
 uint8_t columna_auxiliar[MAX_FILAS];

  Serial.println("ENTRA EN ELIMINAR INSTRUCCIOON");
  for(uint8_t i = 0; i < MAX_FILAS; i++){
    columna_auxiliar[i] = 0;
  }
  
  for(uint8_t i = 0, j = 0; i < MAX_FILAS; i++){
    if(i != fila){
      columna_auxiliar[j] = sistema.memoria_instrucciones[bloque][colum][i];
      j++;
    }
  }

  for(uint8_t i = 0; i < MAX_FILAS; i++){
    Serial.print(columna_auxiliar[i]);
    Serial.print(" ");
  }
  Serial.println();

  for(uint8_t i = 0; i < MAX_FILAS; i++){
    sistema.memoria_instrucciones[bloque][colum][i] = columna_auxiliar[i];
  }

}
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------ELIMINA UN BLOQUE DADO EL INDICE DEL MISMO------------------------------------*/
int eliminar_bloque_instrucciones(uint8_t bloque) {
 uint8_t memoria_auxiliar[MAX_BLOQUES][MAX_COLUMNAS][MAX_FILAS];

  if (verificar_bloque_instrucciones_vacio(bloque)) {
    Serial.println("ESTE BLOQUE ESTA VACIO DEBE SER LLENADO");
    return 0;
  }

  // Inicializar memoria_auxiliar
  for (uint8_t dim = 0; dim < MAX_BLOQUES; dim++) {
    for (uint8_t colum = 0; colum < MAX_COLUMNAS; colum++) {
      for (uint8_t filas = 0; filas < MAX_FILAS; filas++) {
        memoria_auxiliar[dim][colum][filas] = 0;
      }
    }
  }
  // Copiar datos, excluyendo el bloque a eliminar
  for (uint8_t dim = 0, dim_aux = 0; dim < MAX_BLOQUES; dim++){
    if (dim != bloque) {
      for (uint8_t colum = 0; colum < MAX_COLUMNAS; colum++) {
        for (uint8_t filas = 0; filas < MAX_FILAS; filas++) {
          memoria_auxiliar[dim_aux][colum][filas] = sistema.memoria_instrucciones[dim][colum][filas];
        }
      }
      dim_aux++;
    }
  }
  // Copiar datos de vuelta a memoria_instrucciones
  for (uint8_t dim = 0; dim < MAX_BLOQUES; dim++) {
    for (uint8_t colum = 0; colum < MAX_COLUMNAS; colum++) {
      for (uint8_t filas = 0; filas < MAX_FILAS; filas++) {
        sistema.memoria_instrucciones[dim][colum][filas] = memoria_auxiliar[dim][colum][filas];
      }
    }
  }

  Serial.println("/*----IMPRIMIENDO MATRIZ LUEGO DE LA ELIMINACION-----*/");
  imprimir_matriz();
  Serial.println("/*----FIN IMPRIMIR MATRIZ DE LA ELIMINACION-----*/");

  if (sistema.num_bloques != 0) {
    sistema.num_bloques--;
  }
  sistema.sincronizacion = 0;

  return 1;
}

/*---------------------------------------------------------------------------------------------------------------------*/

/*----------------RESETEA TODAS LAS VARIABLES DEL PROGRAMA PARA DAR INICIO A UNA NUEVA PROGRAMACION--------------------*/
void resetear_programa(){
  myFiles.load(5, 0, 310, 480, "escanear_tarjeta.RAW", 1 , 0);
  sistema.grabacion = 0;
  sistema.reproduccion = 0;
  sistema.volver_a_comenzar = 0;
  sistema.sincronizacion = 0;
  inicializar_memoria();
  sistema.flags = 0;
  //INICIALIZAR MEMORIA GRABACIONES
  sistema.num_bloques = 0;
  inicializar_selectores_col();
  Serial.println("MEMORIA RESETEADA");
  imprimir_matriz();
}
/*---------------------------------------------------------------------------------------------------------------------*/

/*--FUNCION VACIA PARA LAS FUNCIONALIDADES ADMINISTRATIVAS--*/
void prueba(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("ACTIVANDO INSTRUCCION");
  }else{
    Serial.println("DESACTIVANDO INSTRUCCION"); 
  } 
}
/*----------------------------------------------------------*/

/*****************************************FUNCIONALIDADES ASOCIADAS A LOS COLORES***************************************/

/*--------CREA UN NUEVO COLOR DADO SUS VALORES EN RGB------*/
/*void crear_nuevo_color(uint8_t pos, uint8_t R, uint8_t G, uint8_t B ){

  if (pos < 0 || pos >= MAX_COLORES) {
    Serial.println("Error: posición fuera de rango");
    return;
  }
  colores[pos].R = R;
  colores[pos].G = G;
  colores[pos].B = B;
}*/

/*----------------------------------------------------------*/

/*-----ESTABLECE EL ARREGLO DE LOS COLORES DISPONIBLES EN LA APLICACION----*/
/*void inicializar_colores_led(){

  crear_nuevo_color(0,255,255,255); //blanco
  crear_nuevo_color(1,255,45,0); //naranja
  crear_nuevo_color(2,255,125,0); //amarillo
  crear_nuevo_color(3,255,0,0); //rojo
  crear_nuevo_color(4,0,0,255); //azul 
  crear_nuevo_color(5,160,25,150); //morado
  crear_nuevo_color(6,0,255,0); //verde
  crear_nuevo_color(7,166,9,28); //rosa
 
}*/
/*------------------------------------------------------------------------*/

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

/*-----------INICIALIZA EL ARREGLO DONDE SE ALMACENA LOS TIEMPO TRANSCURRIDOS QUE CONTROLAN LA ANIMACION DE LOS OJOS-------*/
void inicializar_tiempos_ojos(){
  
  for(uint8_t i = 0; i< MAX_MOV_OJOS ; i++){
    sistema.tiempos_previos_ojos[i] = 0;
  }
}
/*-------------------------------------------------------------------------------------------------------------------------*/

/*----INICIALIZA  EL ARREGLO DONDE SE ALMACENAN LOS TURNOS CORRESPONDIENTES QUE CONTROLAN LA ANIMACION DE LOS OJOS---------*/
void inicializar_turnos_ojos(){
  sistema.turnos_ojos[0] = true;

  for(uint8_t i = 1; i< MAX_MOV_OJOS ; i++){
    sistema.turnos_ojos[i] = false;
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
void mover_ojos_neutros(){

  if((millis() - sistema.tiempos_previos_ojos[0] >= 3000) && sistema.turnos_ojos[0] ){
    ojos_neutros_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 3000) && sistema.turnos_ojos[1]){
    ojos_neutros_pos_1();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[2] = true;
    sistema.tiempos_previos_ojos[2] = millis();
  }

  if((millis() - sistema.tiempos_previos_ojos[2] >= 3000) && sistema.turnos_ojos[2]){
    ojos_neutros_pos_2();
    sistema.turnos_ojos[2] = false;
    sistema.turnos_ojos[3] = true;
    sistema.tiempos_previos_ojos[3] = millis();
  }

  if((millis() - sistema.tiempos_previos_ojos[3] >= 3000) && sistema.turnos_ojos[3]){
    ojos_neutros_pos_3();
    sistema.turnos_ojos[3] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
  }
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

/*---REALIZA EL MOVIMIENTO CORRESPONDIENTE DE OJOS DE ACUERDO A EL UID PROPORCIONADO------*/
//TIPO 1: INDICA QUE NO REQUIERE ANIMACION 
//TIPO 2: REQUIERE ANIMACION POR LO TANTO USA EL ARREGLO DE TIEMPOS Y TURNOS

void realizar_movimiento_ojos(char *ptrUID, uint8_t tipo){

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
      mover_ojos_felices();
    }
    if(strcmp(ptrUID, "53 CF 25 95") == 0){
      Serial.println("MIRAR TRISTE");            
      mover_ojos_tristes();
    }
    if(strcmp(ptrUID, "63 84 6F 94") == 0){
      mover_ojos_enojados();
    }
    if(strcmp(ptrUID, "03 F2 D3 A8") == 0){
      Serial.println("MIRAR ENAMORADO");
      mover_ojos_enamorados();
    }
    if(strcmp(ptrUID, "63 36 C6 94") == 0){
      Serial.println("PESTANEAR");
      pestanar();
    }
  }
}


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
void mover_ojos_enamorados(){
  
  if((millis() - sistema.tiempos_previos_ojos[0] >= 1000) && sistema.turnos_ojos[0]){
    ojos_enamorados_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 500) && sistema.turnos_ojos[1]){
    ojos_enamorados_pos_1();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
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
void mover_ojos_enojados(){
  
  if((millis() - sistema.tiempos_previos_ojos[0] >= 2500) && sistema.turnos_ojos[0] ){
    ojos_enojados_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 2500) && sistema.turnos_ojos[1]){
    ojos_enojados_pos_1();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[2] = true;
    sistema.tiempos_previos_ojos[2] = millis();
  }

  if((millis() - sistema.tiempos_previos_ojos[2] >= 2500) && sistema.turnos_ojos[2]){
    ojos_enojados_pos_2();
    sistema.turnos_ojos[2] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
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
void mover_ojos_tristes(){
  
  if((millis() - sistema.tiempos_previos_ojos[0] >= 2500) && sistema.turnos_ojos[0] ){
    ojos_tristes_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 2500) && sistema.turnos_ojos[1]){
    ojos_tristes_pos_1();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
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
void mover_ojos_felices(){
  
  if((millis() - sistema.tiempos_previos_ojos[0] >= 750) && sistema.turnos_ojos[0] ){
    ojos_felices_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 750) && sistema.turnos_ojos[1]){
    ojos_felices_pos_1();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
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
void pestanar(){
  
  if((millis() - sistema.tiempos_previos_ojos[0] >= 2000) && sistema.turnos_ojos[0]){
    ojos_neutros_pos_0();
    sistema.turnos_ojos[0] = false;
    sistema.turnos_ojos[1] = true;
    sistema.tiempos_previos_ojos[1] = millis();
  }
  
  if((millis() - sistema.tiempos_previos_ojos[1] >= 500) && sistema.turnos_ojos[1]){
    cerrar_ojos();
    sistema.turnos_ojos[1] = false;
    sistema.turnos_ojos[2] = true;
    sistema.tiempos_previos_ojos[2] = millis();
  }

  if((millis() - sistema.tiempos_previos_ojos[2] >= 2000) && sistema.turnos_ojos[2]){
    ojos_neutros_pos_3();
    sistema.turnos_ojos[2] = false;
    sistema.turnos_ojos[3] = true;
    sistema.tiempos_previos_ojos[3] = millis();
  }

  if((millis() - sistema.tiempos_previos_ojos[3] >= 500) && sistema.turnos_ojos[3]){
    cerrar_ojos();
    sistema.turnos_ojos[3] = false;
    sistema.turnos_ojos[0] = true;
    sistema.tiempos_previos_ojos[0] = millis();
  }
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


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
void imprimir_funcion_selec_columna_matriz_funcionalidades(uint8_t bloque, uint8_t columna, uint8_t indice_fila_selec, bool seleccion){
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

  for(uint8_t fila = 0; fila < MAX_FILAS ; fila++){
    cord_y = cord_y + 65;
    if(fila == indice_fila_selec){
      imprimir_icono_funcionalidad(1, sistema.memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila < indice_fila_selec && !seleccion){
      imprimir_icono_funcionalidad(2, sistema.memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila < indice_fila_selec && seleccion){
      imprimir_icono_funcionalidad(0, sistema.memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
    if(fila > indice_fila_selec){
      imprimir_icono_funcionalidad(0, sistema.memoria_instrucciones[bloque][columna][fila], cord_y); 
    }      
  }
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*-----IMPRIME EL ICONO DE LA FUNCIONALIDAD EN LA PANTALLA DADO EL ESTADO DE LA MISMA, SU INDICE Y LA POSICION EN EL EJE Y----*/
void imprimir_icono_funcionalidad(uint8_t estado, uint8_t indice, int pos_y){
  
  switch(indice){
    case 8 :{
      if(estado == 0){
        myFiles.load(0, pos_y, 60, 60, "mover_cab_izq.RAW", 1 , 0);
      }
      if(estado == 1){
        myFiles.load(0, pos_y, 60, 60, "selec_mover_cab_izq.RAW", 1 , 0);
      }
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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
      if(estado == 2){
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

/*------IMPRIME LA MATRIZ DE FUNCIONALIDADES A EJECUTAR DADO EL BLOQUE CORRESPONDIENTE-----*/
void imprimir_pantalla_matriz_funcionalidades(uint8_t bloque){
 
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
  
  for(uint8_t columna = 0; columna < MAX_COLUMNAS ; columna++){
    int cord_y = 0;
    for(uint8_t fila = 0; fila < MAX_FILAS; fila++){
      cord_y = cord_y + 65;
      imprimir_icono_funcionalidad(0, sistema.memoria_instrucciones[bloque][columna][fila], cord_y); 
    }
  }

  if(sistema.num_bloques > bloque ){ //si verificaste que hay un bloque despues
    myFiles.load(218, 455, 97, 25, "f_siguiente.RAW", 1 , 0);
  }

  if(sistema.num_bloques > 0 && bloque != 0 ){ //si verificaste que hay un bloque antes
    myFiles.load(5, 455, 99, 25, "f_anterior.RAW", 1 , 0);
  }
 
}

/*-----------------MUESTRA LA IMAGEN EN GRANDE DE TARJETA EN LA PANTALLA----------------*/
void imprimir_imagen_tarjeta(uint8_t indice){

  if (indice >= 0 && indice < MAX_FUNCIONALIDADES) {
    if (imagenes_tarjetas[indice] != nullptr) {
      // Cargar la imagen si está disponible
     myFiles.load(5, 0, 310, 480, imagenes_tarjetas[indice], 1, 0);
    } else {
      // Imprimir mensaje si no hay imagen disponible
      Serial.println("SIN IMAGEN POR LOS MOMENTOS");
    }
  } else {
    Serial.println("Índice fuera de rango");
  }

  unsigned long tiempo_ahora = 0;
  
  tiempo_ahora = millis();   //RETRASO DE 1250MS
  while(millis() < tiempo_ahora + 500 );
  
}
/*--------------------------------------------------------------------------------*/

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
/*void encender_luces(bool fin_eje_inst, char *UID){
  Serial.println("ENTRA A ENCENDER LUCES");
  if(!fin_eje_inst){
    uint8_t R = 0, G = 0, B = 0;
    if(strcmp(UID, "E3 F3 F0 94") == 0){ //BLANCO
      Serial.println("ENTRA A ENCENDER LUCES BLANCAS");
      R = colores[0].R;
      G = colores[0].G;
      B = colores[0].B;
    }
    if(strcmp(UID, "73 4B AA 94") == 0){ //NARANJA
      R = colores[1].R;
      G = colores[1].G;
      B = colores[1].B;
      Serial.println("ENTRA A ENCENDER NARANJAS");
    }
    if(strcmp(UID, "83 9D 6D 94") == 0){ //AMARILLO
      R = colores[2].R;
      G = colores[2].G;
      B = colores[2].B;
      Serial.println("ENTRA A ENCENDER LUCES AMARILLAS");
    }
    if(strcmp(UID, "E3 D3 6B 12") == 0){ //ROJO
      R = colores[3].R;
      G = colores[3].G;
      B = colores[3].B;
      Serial.println("ENTRA A ENCENDER ROJAS");
    }
    if(strcmp(UID, "B3 05 6E 12") == 0){ //AZUL
      R = colores[4].R;
      G = colores[4].G;
      B = colores[4].B;
      Serial.println("ENTRA A ENCENDER LUCES AZULES");
    }
    if(strcmp(UID, "F3 43 C6 12") == 0){ //MORADO
      R = colores[5].R;
      G = colores[5].G;
      B = colores[5].B;
      Serial.println("ENTRA A ENCENDER LUCES  MORADAS");
    }
    if(strcmp(UID, "93 F1 C7 12") == 0){ //VERDE
      R = colores[6].R;
      G = colores[6].G;
      B = colores[6].B;
      Serial.println("ENTRA A ENCENDER LUCES VERDES");
    }
    if(strcmp(UID, "83 75 A9 94") == 0){ //ROSA
      R = colores[7].R;
      G = colores[7].G;
      B = colores[7].B;
      Serial.println("ENTRA A ENCENDER LUCES ROSAS");
    }

    for(uint8_t i = 0; i < NUM_LEDS ; i++){
      color_led.setPixelColor(i, R,G, B);
    }
    color_led.show();
  }else{
    for(uint8_t i = 0; i < NUM_LEDS ; i++){
      color_led.setPixelColor(i, color_led.Color(0,0,0));
    }
    color_led.show();
    Serial.println("ENTRA A APAGAR LUCES");
  }  
  
}

/******************FUNCIONALIDADES ASOCIADAS AL MOVIMIENTO*******************************/
void avanzar(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("AVANZANDO");
  //  sistema.luz_verde = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN AVANZANDO"); 
  } 

}

void retroceder(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("RETROCEDIENDO");
    //sistema.luz_amarilla = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN RETROCEDIENDO"); 
  } 
}

void girar_izq(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("GIRANDO IZQUIERDA");
    //sistema.luz_verde = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN GIRANDO IZQUIERDA"); 
  } 
}

void girar_der(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("GIRANDO DERECHA");
    //sistema.luz_verde = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN GIRANDO DERECHA"); 
  } 
}

void volver_izq(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("VOLVIENDO IZQUIERDA");
    //sistema.luz_amarilla = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN VOLVIENDO IZQUIERDA"); 
  } 
}

void volver_der(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("VOLVIENDO DERECHA");
    //sistema.luz_amarilla = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN VOLVIENDO DERECHA"); 
  } 
}


void girar_sobre_si_mismo(bool fin_eje_inst){
  if(!fin_eje_inst){
    Serial.println("GIRANDO SOBRE SI MISMO");
    //sistema.luz_verde = true;
    //sistema.luz_roja = false;
  }else{
    Serial.println("FIN GIRANDO SOBRE SI MISMO"); 
  } 
}

/**********************FUNCIONALIDADES ASOCIADAS AL SONIDO******************************/
/*----EMITE SONIDO DADA LA VARIABLE QUE INDICA SI SE HA LLEGADO AL FIN DE LA EJECUCION DE LA INSTRUCCION---*/
void emitir_sonido(bool fin_eje_inst){

  if(!fin_eje_inst){
    Serial.println("COMIENZA A EMITIR SONIDO");
    Serial1.print("EMITIR-SONIDO_");
  }else{
    Serial.println("COMIENZA A DETENER SONIDO");
    Serial1.print("DETENER-SONIDO_");
  }
}
/*--------------------------------------------------------------------------------------------------------*/

void reproducir_grabacion(bool fin_eje_inst){

  if(!fin_eje_inst){
    Serial.println("REPRODUCIENDO GRABACION");
    Serial1.print("REPRODUCIR-GRABACION_");   
  }else{
    Serial1.print("DETENER-REPRODUCCION-GRABACION_");  
  }
}

void pruebita (){
  Serial.println("PRUEBITAAAA");
 
}

void inicializar_columnas_ejecucion_programa(){

  for(uint8_t i = 0; i< MAX_COLUMNAS; i++){
    columnas[i].tiempo_inicio = 0;
    columnas[i].tiempo_fin = 0;
    columnas[i].tiempo_ejecucion = 0;
    columnas[i].tiempo_inicio_inst = 0;
    columnas[i].tiempo_eje_inst = 0;
    columnas[i].tiempo_excedido = false;
    columnas[i].ejecutada = false;
    columnas[i].vacia = false;
    columnas[i].fila = 0;
  }
}


void leerCaracteresHastaDelimitador(char delimitador, unsigned long timeout, char* mensaje, int tamanoBuffer) {
    uint8_t index = 0;
    unsigned long startTime = millis();
    Serial.println("ENTRO A LEER CARACTERES");

    while (millis() - startTime < timeout) {
        if (Serial1.available()) {
            Serial.println("HAY DATOS DISPONIBLES EN EL SERIAL MONITOR");
            char c = Serial1.read();
            Serial.print("Caracter leído: ");
            Serial.println(c);
            
            if (c == delimitador) {
                Serial.println("ENCONTRO EL DELIMITADOR");
                break;  // Salir si se encuentra el delimitador
            }

            // Verificar que no se exceda el tamaño del arreglo
            if (index < tamanoBuffer - 1) {
                mensaje[index] = c;
                index++;
                mensaje[index] = '\0';  // Añadir el terminador nulo
                Serial.print("Mensaje acumulado: ");
                Serial.println(mensaje);
            } else {
                Serial.println("Error: tamaño de mensaje excedido");
                break;  // Salir si se excede el tamaño
            }
        }
    }

    // Limpiar cualquier dato residual en el buffer
    while (Serial1.available()) {
        Serial1.read();  // Descartar los caracteres restantes
    }

    Serial.println("LISTIN");
}


void activarBandera(uint8_t bandera) {
    sistema.flags |= (1 << bandera); // Activa la bandera especificada
}

// Función para desactivar una bandera
void desactivarBandera(uint8_t bandera) {
    sistema.flags &= ~(1 << bandera); // Desactiva la bandera especificada
}

// Función para verificar el estado de una bandera
uint8_t esBanderaActiva(uint8_t bandera) {
    return (sistema.flags & (1 << bandera)) != 0; // Devuelve 1 si la bandera está activa, 0 si no
}


void inicializar_selectores_col(){
    for(uint8_t i = 0; i < MAX_COLUMNAS; i++){
      sistema.selec_col[i] = -1;
    }
}

/*---------FUNCIONES QUE SE EJECUTAN UNA SOLA VEZ-----------*/
void setup(void) {
  Serial.begin(9600);
  Serial1.begin(9600);
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif
  // END of Trinket-specific code.

  while(!Serial){
      
  }
  //wdt_enable(WDTO_5S); 
  Serial1.setTimeout(5000);
  sistema.sincronizacion = 0;
  sistema.volver_a_comenzar = 1;
  sistema.num_bloques = 0;
  sistema.grabacion = 0;
  sistema.reproduccion = 0;
  sistema.flags = 0;
  sistema.tiempo_inicio_programa = 0;
  sistema.tiempo_fin_programa = 0;
  crear_arreglo_funcionalidades();  
  inicializar_memoria();
  inicializar_selectores_col();
  //inicializar_colores_led();
  //color_led.begin();
  inicializar_pantalla_tft();
  myFiles.load(5, 0, 310, 480, "escanear_tarjeta.RAW", 1 , 0);
  inicializar_ojos();
  inicializar_tiempos_ojos();
  inicializar_turnos_ojos();

}
/*---------------------------------------------------------*/



/**-------------------------------------------------------------------------------------PROGRAMA PRINCIPAL---------------------------------------------------------------------------------------------------*/
void loop(void) {

  mover_ojos_neutros();

  if (Serial1.available() > 0) { // Verifica si hay datos disponibles
    Serial.println("HAY DATOS DISPONIBLES PLACA B");
    //String mensaje = Serial1.readStringUntil('_');
    char mensaje[50];
    leerCaracteresHastaDelimitador('_', 5000, mensaje, 50);
    Serial.println("DESPUES DE LEER");
    //Serial.println(mensaje);
 //   String mensaje = leerHastaCaracter('_'); // Lee el mensaje
    Serial.print("RESPUESTA B-->");
    Serial.println(mensaje);
   
    if (strcmp(mensaje, "EJECUTAR-PROGRAMA") != 0) {
      Serial.print("PTRUID ENVIADO");
      Serial.println(mensaje);
      Serial.println("ALMACENANDO UID RECIBIDO");
      escanear_instrucciones(mensaje);
      Serial.println("FIN ESCANEOITO");
    } 

    if (strcmp(mensaje, "EJECUTAR-PROGRAMA") == 0) {
      
      unsigned long tiempo_eje_programa = 0, tiempo_inicio_bloque = 0, tiempo_fin_bloque = 0, tiempo_eje_bloque = 0;
      bool inicio_bloque = false, fin_programa = false, imprimir_pantalla = false;
      uint8_t bloque = 0;  // Bloque actual
    
      Serial.println("HOLA MUNDO EJECUTAR PROGRAMA");
 
      inicializar_arreglo_tiempos_ejecucion_bloques();
  
      establecer_tiempos_eje_arreglo_tiempos_bloques(tiempo_duracion_bloque_instrucciones(0),tiempo_duracion_bloque_instrucciones(1),tiempo_duracion_bloque_instrucciones(2),tiempo_duracion_bloque_instrucciones(3),tiempo_duracion_bloque_instrucciones(4));
  
      tiempo_eje_programa = determinar_duracion_programa();

      Serial.print("*********************TIEMPO EJECUCION PROGRAMA********************");
      Serial.println(tiempo_eje_programa);
      Serial.println();
      Serial.println();
      while(sistema.volver_a_comenzar > 0){ //REPITE HASTA QUE HAYAS COMPLETADO TODAS LAS ITERACIONES INDICADAS
        myFiles.load(5, 0, 312, 480, "programa_comenzar.RAW", 1 , 0);
        delay(1500); //ver si se reemplaza por millis
        myFiles.load(5, 0, 310, 480, "memoria_instrucciones.RAW", 1 , 0);
        delay(1000); //ver si se reemplza por millis
        Serial.print("**************ITERACION NRO: ");
        Serial.print(sistema.volver_a_comenzar);
        Serial.println("**************");
        sistema.tiempo_inicio_programa = millis();
        sistema.tiempo_fin_programa = millis();
        inicializar_turnos_ojos();
        inicializar_tiempos_ojos();
        inicializar_columnas_ejecucion_programa();

        while(!fin_programa){ //REPITE MIENTRAS EL TIEMPO TRANSCURRIDO SEA MENOR AL TIEMPO DE EJECUCION DEL PROGRAMA
      
          if (!inicio_bloque){ //SI ESTE BLOQUE AUN NO SE EJECUTA INICIALIZA TODAS LAS VARIABLES CORRESPONDIENTES
            Serial1.print("ACTUALIZAR-BLOQUE_");
            myGLCD.clrScr();
            inicio_bloque = true;
            tiempo_fin_bloque = millis();
            tiempo_inicio_bloque = millis();
            columnas[0].tiempo_inicio = tiempo_inicio_bloque;
            columnas[1].tiempo_inicio = tiempo_inicio_bloque;
            columnas[2].tiempo_inicio = tiempo_inicio_bloque;
            columnas[3].tiempo_inicio = tiempo_inicio_bloque;
            columnas[4].tiempo_inicio = tiempo_inicio_bloque;
            columnas[0].fila = 0;
            columnas[1].fila = 0;
            columnas[2].fila = 0;
            columnas[3].fila = 0;
            columnas[4].fila = 0;
            columnas[0].tiempo_ejecucion = 0;
            columnas[1].tiempo_ejecucion = 0;
            columnas[2].tiempo_ejecucion = 0;
            columnas[3].tiempo_ejecucion = 0;
            columnas[4].tiempo_ejecucion = 0;

            if(sistema.memoria_instrucciones[bloque][0][0] == 0){
              columnas[0].vacia = true;
            }
            if(sistema.memoria_instrucciones[bloque][1][0] == 0){
              columnas[1].vacia = true;
            }
            if(sistema.memoria_instrucciones[bloque][2][0] == 0){
              columnas[2].vacia = true;
            }
            if(sistema.memoria_instrucciones[bloque][3][0] == 0){
              columnas[3].vacia = true;
            }
            if(sistema.memoria_instrucciones[bloque][4][0] == 0){
              columnas[4].vacia = true;
            }

            //ESTABLECE EL TIEMPO DE EJECUCION DEL BLOQUE ACTUAL
            if(bloque == 0){
              tiempo_eje_bloque = sistema.tiempos_eje_bloques[0];
            }
            if(bloque == 1){
              tiempo_eje_bloque = sistema.tiempos_eje_bloques[1];
            }
            if(bloque == 2){
              tiempo_eje_bloque = sistema.tiempos_eje_bloques[2];
            }
            if(bloque == 3){
              tiempo_eje_bloque = sistema.tiempos_eje_bloques[3];
            }
            if(bloque == 4){
              tiempo_eje_bloque = sistema.tiempos_eje_bloques[4];
            }
          }


      //--------------------------------------------
          //VERIFICA SI LA COLUMNA 0 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV EXTREMIDADES
          if(!columnas[0].vacia){ 
            ejecutar_columna_instrucciones(&bloque, 0, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque);
            imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 0, columnas[0].fila, false);
          }
          //VERIFICA SI LA COLUMNA 1 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV CUERPO
          if(!columnas[1].vacia){
            ejecutar_columna_instrucciones(&bloque,1, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque);
            imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 1, columnas[1].fila, false);
          }
          //VERIFICA SI LA COLUMNA 2 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //LUCES LEDS
          if(!columnas[2].vacia){///AQUI SE BLOQUEA EL PROGRAMA
            ejecutar_columna_instrucciones(&bloque,2, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque);
            imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 2, columnas[2].fila, false);
          }
          //VERIFICA SI LA COLUMNA 3 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //MOV OJOS
          if(!columnas[3].vacia){
            ejecutar_columna_instrucciones(&bloque,3, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque);
            imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 3, columnas[3].fila, false);
          }else{
            mover_ojos_neutros();
          }
      
          //VERIFICA SI LA COLUMNA 4 DE LA MATRIZ MEMORIA NO ESTA VACIA Y PROCEDE A EJECUTAR LAS INSTRUCCIONES QUE ESTA CONTENGA //SONIDO
          if(!columnas[4].vacia){
            ejecutar_columna_instrucciones(&bloque,4, &tiempo_inicio_bloque, &tiempo_fin_bloque, &tiempo_eje_bloque);
            imprimir_funcion_selec_columna_matriz_funcionalidades(bloque, 4, columnas[4].fila, false);
          }
      
          tiempo_fin_bloque = millis();
          sistema.tiempo_fin_programa = millis();

          //VERIFICA SI HA LLEGADO EL TIEMPO DE FINALIZAR UN BLOQUE DE INSTRUCCIONES
          if(tiempo_fin_bloque - tiempo_inicio_bloque >= tiempo_eje_bloque){ 
            if(!columnas[0].ejecutada && !columnas[1].ejecutada && !columnas[2].ejecutada && !columnas[3].ejecutada && !columnas[4].ejecutada){ //SI YA TODAS LAS INSTRUCCIONES HAN SIDO DESACTIVADAS PROCEDE A CULMINAR EL BLOQUE
              inicio_bloque = false; 
              if(bloque < sistema.sincronizacion){
                bloque++;
              }
          
              unsigned long tiempo_ahora = 0;
  
              tiempo_ahora = millis();   //RETRASO DE 1250MS
              while(millis() < tiempo_ahora + 500 );

              myGLCD.clrScr();
            }
          }
 
          //VERIFICA SI HA LLEGADO EL TIEMPO DE CULMINAR EL PROGRAMA
          if((sistema.tiempo_fin_programa - sistema.tiempo_inicio_programa >= tiempo_eje_programa) && !inicio_bloque){
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
        sistema.volver_a_comenzar--;
      }

      Serial.println("/*-------NUEVO ESCANEO-----*/");

      //SE INICIALIZAN LAS VARIABLES PERTINENTES PARA REALIZAR UNA NUEVA ESCRITURA DE PROGRAMA
      bloque = 0; //verifica si lo tengo que quitar
      sistema.grabacion = 0;
      sistema.volver_a_comenzar = 1;
      desactivarBandera(COMENZAR_PROGRAMA);
      desactivarBandera(FINALIZAR_PROGRAMA);
      sistema.sincronizacion = 0;
      inicializar_memoria();
      desactivarBandera(EJECUTAR_PROGRAMA);
      inicializar_tiempos_ojos(); //INICIALIZO NUEVAMENTE LOS ARREGLOS ENCARGADOS DEL CONTROL DE LOS OJOS
      inicializar_turnos_ojos();
      Serial.println("FIN DE LA EJECUCION BLOQUE DE INSTRUCCIONES");
      Serial1.print("FINALIZO-EJECUCION-PROGRAMA_");
    }
  }
}
/*------------------------------------------*/



