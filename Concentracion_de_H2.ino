/************************Hardware Related Macros************************************/
#define         MQ_PIN                       (0)     //definir qué pin de entrada analógica vamos a utilizar
#define         RL_VALUE                     (10)    //definir la resistencia de carga en la placa, en kilo ohmios
#define         RO_CLEAN_AIR_FACTOR          (9.21)  //RO_CLEAR_AIR_FACTOR=(Resistencia del sensor en aire limpio)/RO,
                                                     //que se deriva del gráfico de la ficha técnica
 
/***********************Software Related Macros************************************/
#define         CALIBARAION_SAMPLE_TIMES     (50)    //definir cuántas muestras se van a tomar en la fase de calibración
#define         CALIBRATION_SAMPLE_INTERVAL  (500)   //definir el intervalo de tiempo (en milisegundos) entre cada muestra en
                                                     //la fase de cablibración
#define         READ_SAMPLE_INTERVAL         (50)    //definir cuántas muestras va a tomar en funcionamiento normal
#define         READ_SAMPLE_TIMES            (5)     //definir el intervalo de tiempo (en milisegundos) entre cada muestra en 
                                                     //funcionamiento normal
 
/**********************Application Related Macros**********************************/
#define         GAS_H2                      (0)
 
/*****************************Globals***********************************************/
float           H2Curve[3]  =  {2.3, 0.93,-1.44};    //Se toman dos puntos de la curva de la hoja de datos
                                                     //Con estos dos puntos, se forma una línea que es "aproximadamente equivalente"
                                                     //a la curva original. 
                                                     // formato de datos:{ x, y, pendiente}; punto1: (lg200, lg8.5), punto2: (lg10000, lg0.03)
 
float           Ro           =  10;                  //Ro se inicializa a 10 kilo ohms
 
void setup() {
  Serial.begin(9600);                                //Configuración de la UART, velocidad en baudios = 9600bps
  Serial.print("Calibrando...\n");                
  Ro = MQCalibration(MQ_PIN);                        //Calibración del sensor. El sensor debe estár en aire limpio
                                                     //cuando se realice la calibración                  
  Serial.print("La calibración está lista...\n"); 
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm");
  Serial.print("\n");
}
 
void loop() {
   Serial.print("Concentración de H2:"); 
   Serial.print(MQGetGasPercentage(MQRead(MQ_PIN)/Ro,GAS_H2) );
   Serial.print( "ppm" );
   Serial.print("\n");
   delay(200);
}
 
/****************** MQResistanceCalculation ****************************************
Entrada:       raw_adc - valor leído del adc, que representa el voltaje
Salida:        La resistencia del sensor calculada
Observaciones: El sensor y la resistencia de carga forman un divisor de voltaje.
               Dada la tensión a través de la resistencia de carga y su resistencia, la 
               resistencia del sensor se puede deducir la resistencia del sensor.
************************************************************************************/ 
float MQResistanceCalculation(int raw_adc) {
  return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}
 
/***************************** MQCalibration ****************************************
Entrada:       mq_pin - canal analógico
Salida:        Ro del sensor
Observaciones: Esta función asume que el sensor está en aire limpio.  
               Utiliza MQResistanceCalculation para calcular la resistencia del sensor en aire limpio y 
               luego lo divide con RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR es aproximadamente 10,
               que difiere ligeramente entre los diferentes sensores.
************************************************************************************/ 
float MQCalibration(int mq_pin) {
  int i;
  float val=0;
 
  for (i=0;i<CALIBARAION_SAMPLE_TIMES;i++) {            //tomar varias muestras
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val/CALIBARAION_SAMPLE_TIMES;                   //calcular el valor promedio
 
  val = val/RO_CLEAN_AIR_FACTOR;                        //dividido por RO_CLEAN_AIR_FACTOR da como resultado el Ro
                                                        //de acuerdo con la gráfica de la hoja de datos
 
  return val; 
}
/*****************************  MQRead *********************************************
Entrada:       mq_pin - canal analógico
Salida:        Rs del sensor
Observaciones: Esta función utiliza MQResistanceCalculation para calcular la resistencia 
               del sensor (Rs). La Rs cambia cuando el sensor se encuentra en diferentes 
               concentraciones del gas objetivo. Los tiempos de muestreo y el intervalo
               de tiempo entre muestras pueden ser configurados cambiando la definición de las macros.
************************************************************************************/ 
float MQRead(int mq_pin) {
  int i;
  float rs=0;
 
  for (i=0;i<READ_SAMPLE_TIMES;i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
 
  rs = rs/READ_SAMPLE_TIMES;
 
  return rs;  
}
 
/*****************************  MQGetGasPercentage **********************************
Entrada:       rs_ro_ratio - Rs Rs dividido por Ro gas_id - tipo de gas objetivo
Salida:        ppm del gas objetivo
Observaciones: Esta función pasa diferentes curvas a la función MQGetPercentage que calcula 
               las ppm (partes por millón) del gas objetivo.
************************************************************************************/ 
int MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  if ( gas_id == GAS_H2) {
     return MQGetPercentage(rs_ro_ratio,H2Curve);
  }  
  return 0;
}
 
/*****************************  MQGetPercentage **********************************
Entrada:       rs_ro_ratio - Rs dividido por Ro
               pcurve - punto de la curva del gas objetivo
Salida:        ppm del gas objetivo
Observaciones: Utilizando la pendiente y un punto de la línea. La x(valor logarítmico de ppm) de la línea puede derivarse si se proporciona y(rs_ro_ratio). 
               Como se trata de una coordenada logarítmica, se utiliza la potencia 
               de 10 para convertir el resultado en no logarítmico 
************************************************************************************/ 
int  MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}
