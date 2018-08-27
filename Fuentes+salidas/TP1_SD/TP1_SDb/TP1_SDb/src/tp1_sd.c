/* 
 * TP1b - SD / Ensayo 2 tp1_sd.c 
 * R.Oliva - Materia Protocolos SE
 * CESE 2018
 * 
 * Copyright 2014, ChaN
 * Copyright 2016, Matias Marando
 * Copyright 2016, Eric Pernia
 * All rights reserved.
 *
 * This file is part of Workspace.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*==================[inlcusiones]============================================*/

#include "tp1_sd.h"   // <= own header (optional)
#include "sapi.h"     // <= sAPI header

#include "ff.h"       // <= Biblioteca FAT FS

/*==================[definiciones y macros]==================================*/

#define FILENAME "muestras.txt"

/*==================[definiciones de datos internos]=========================*/

static FATFS fs;           // <-- FatFs work area needed for each volume
static FIL fp;             // <-- File object needed for each open file
/* Buffer para UART- de ejemplo RTC */
static char uartBuff[10];
// Nuevo para BUf RTC..nuevo formato
static char RTCbuf[40];
// Nuevo para ADCbuf
static char ADCbuf[40];



/* Variables para almacenar el valores leidos del ADC */
uint16_t muestra1 = 0;
uint16_t muestra2 = 0;
uint16_t muestra3 = 0;
/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

// FUNCION que se ejecuta cada vez que ocurre un Tick
void diskTickHook( void *ptr );

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.

 */
char* itoa(int value, char* result, int base) {
   // check that the base if valid
   if (base < 2 || base > 36) { *result = '\0'; return result; }

   char* ptr = result, *ptr1 = result, tmp_char;
   int tmp_value;

   do {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
   } while ( value );

   // Apply negative sign
   if (tmp_value < 0) *ptr++ = '-';
   *ptr-- = '\0';
   while(ptr1 < ptr) {
      tmp_char = *ptr;
      *ptr--= *ptr1;
      *ptr1++ = tmp_char;
   }
   return result;
}


/* Enviar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
void showDateAndTime( rtc_t * rtc ){
   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->mday), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el dia */
   if( (rtc->mday)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->month), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el mes */
   if( (rtc->month)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->year), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el año */
   if( (rtc->year)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   uartWriteString( UART_USB, ", ");


   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->hour), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio la hora */
   if( (rtc->hour)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->min), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los minutos */
  // uartBuff[2] = 0;    /* NULL */
   if( (rtc->min)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->sec), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los segundos */
   if( (rtc->sec)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   /* Envio un 'enter' */
   uartWriteString( UART_USB, "\r\n");
}

/* Devolver fecha y hora en formato "YYYY/DD/MM_HH:MM:SS" */
void printDateAndTime( rtc_t * rtc, char *sbuf ){
	
   stdioSprintf(sbuf, "%04d/%02d/%02d_%02d:%02d:%02d", 
                 (int) (rtc->year),
				 (int) (rtc->month),
				 (int) (rtc->mday),
                 (int) (rtc->hour),
				 (int) (rtc->min),
				 (int) (rtc->sec)
   );
 
}

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void ){

   /* Definiciones internas main() */
   /* a) Estructura RTC y variables */
   rtc_t rtc;
   bool_t val = 0;
   uint8_t i = 0;   
   delay_t delay1s;   
   /* b) Contador NoStandard ELM */   
   UINT nbytes;

   // ---------- CONFIGURACIONES ------------------------------
   // Inicializar y configurar la plataforma
   boardConfig();

   // SPI configuration
   spiConfig( SPI0 );

   // Inicializar el conteo de Ticks con resolucion de 10ms,
   // con tickHook diskTickHook
   tickConfig( 10 );
   tickCallbackSet( diskTickHook, NULL );
   
   /* Inicializar UART_USB a 115200 baudios */
   uartConfig( UART_USB, 115200 );
   
   /* Inicializar ADC  */
   adcConfig( ADC_ENABLE ); /* ADC */

   uartWriteString( UART_USB, "\r\n TP1_SD Ensayo 1");
  
   // Uso de stdioSprintf()  
   //stdioSprintf(buf, " 3: %-4d left justif.\n", 3);
   //stdioPrintf(UART_USB, "%s", buf);

   rtc.year = 2018;
   rtc.month = 7;
   rtc.mday = 3;
   rtc.wday = 1;
   rtc.hour = 13;
   rtc.min = 17;
   rtc.sec= 0;

   /* Inicializar RTC */
   uartWriteString( UART_USB, "\r\n Inicializo RTC..");   
   val = rtcConfig( &rtc );

   delayConfig( &delay1s, 1000 );

   delay(2000); // El RTC tarda en setear la hora, por eso el delay

   for( i=0; i<10; i++ ){
      /* Leer fecha y hora */
      val = rtcRead( &rtc );
      /* Mostrar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
      showDateAndTime( &rtc );
      delay(1000);
   }

   rtc.year = 2018;
   rtc.month = 7;
   rtc.mday = 4;
   rtc.wday = 2;
   rtc.hour = 14;
   rtc.min = 30;
   rtc.sec= 0;

   /* Establecer fecha y hora */
   uartWriteString( UART_USB, "\r\n Cambio la hora del RTC..");    
   val = rtcWrite( &rtc );
   delay(2000); // El RTC tarda en setear la hora, por eso el delay
   uartWriteString( UART_USB, "\r\n Leo nueva hora del RTC..");       
   val = rtcRead( &rtc );
   /* Mostrar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
   showDateAndTime( &rtc );

   /* Ensayo BufferRTC form nuevo */
   uartWriteString( UART_USB, "Buffer RTC nvo:\r\n");
   
   printDateAndTime( &rtc , RTCbuf );
   stdioPrintf(UART_USB, "%s", RTCbuf);
   
   //     if ( delayRead( &delay1 ) ){
   /* Leo la Entrada Analogica AI0 - ADC0 CH1 */
   uartWriteString( UART_USB, "\n\r ADC Canales ADCBuf:\r\n");
   muestra1 = adcRead( CH1 );
   muestra2 = adcRead( CH2 );
   muestra3 = adcRead( CH3 );   
   stdioSprintf(ADCbuf, "%03d;%03d;%03d;", muestra1,muestra2,muestra3);
   stdioPrintf(UART_USB, "%s \n\r", ADCbuf);   

   
   // ------ PROGRAMA QUE ESCRIBE EN LA SD -------

   // Give a work area to the default drive
   uartWriteString( UART_USB, "\r\n SD_fmount() ..");      
   if( f_mount( &fs, "", 0 ) != FR_OK ){
      // If this fails, it means that the function could
      // not register a file system object.
	     uartWriteString( UART_USB, "\r\n Error en SD_fmount..");   
      // Check whether the SD card is correctly connected
   } else {
	   	 uartWriteString( UART_USB, "\r\n SD_fmount OK!.");   
   }

   // Infinite loop

   uartWriteString( UART_USB, "\n\r ADC Ch1; Ch2; Ch3; YYYY/DD/MM_HH:MM:SS\r\n");
   while(TRUE) {

         /* delayRead retorna TRUE cuando se cumple el tiempo de retardo */
         if ( delayRead( &delay1s ) ){

        	   val = rtcRead( &rtc );
        	   printDateAndTime( &rtc , RTCbuf );
        	   muestra1 = adcRead( CH1 );
        	   muestra2 = adcRead( CH2 );
        	   muestra3 = adcRead( CH3 );
        	   stdioSprintf(ADCbuf, "%03d;%03d;%03d;", muestra1,muestra2,muestra3);
        	   stdioPrintf(UART_USB, "\n\r %s ", ADCbuf);
        	   stdioPrintf(UART_USB, "%s", RTCbuf);

			   if( f_open( &fp, FILENAME, FA_WRITE | FA_OPEN_APPEND ) == FR_OK ){
					 f_write( &fp, ADCbuf, sizeof(ADCbuf), &nbytes );
					 f_write( &fp, RTCbuf, sizeof(RTCbuf), &nbytes );
					 f_write( &fp, "\n\r", 3, &nbytes);
					 f_close(&fp);

					 if( nbytes >= 3 ){
						// Turn ON LEDG if the write operation was successful
						uartWriteString( UART_USB, "\r\n Escritura a SD OK!");
						gpioWrite( LEDG, ON );
					     }
				      } else{
					 // Turn ON LEDR if the write operation was fail
					 gpioWrite( LEDR, ON );
					 uartWriteString( UART_USB, "\r\n Error en SD Write!");
				     }
               }  // End if fopen()
         } // End if delay...
   //}       // End while(1)

   // uartWriteString( UART_USB, "\r\n Fin ensayo");   // never here

   // ---------- REPETIR POR SIEMPRE --------------------------
   while( TRUE )
   {
      sleepUntilNextInterrupt();
   }

   // NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
   // directamenteno sobre un microcontroladore y no es llamado/ por ningun
   // Sistema Operativo, como en el caso de un programa para PC.
   return 0;
}

/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/

// FUNCION que se ejecuta cada vezque ocurre un Tick
void diskTickHook( void *ptr ){
   disk_timerproc();   // Disk timer process
}


/*==================[fin del archivo]========================================*/
