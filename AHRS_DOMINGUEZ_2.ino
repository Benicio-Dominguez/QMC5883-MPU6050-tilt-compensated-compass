#include <Wire.h>
#include <MechaQMC5883.h>
#include <MPU9250_WE.h>
#define MPU9250_ADDR 0x68

#define MPU6050_ADDRESS_AD0_LOW     0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU6050_ADDRESS_AD0_HIGH    0x69 // address pin high (VCC)
#define MPU6050_DEFAULT_ADDRESS     MPU6050_ADDRESS_AD0_LOW

MPU9250_WE myMPU9250 = MPU9250_WE(MPU9250_ADDR);
MechaQMC5883 qmc;

int magXYZA_RAW[4]; //x, y, z, azimuth crudos del magnetometro.
int magXYZ[3];     //x, y, z, con correcciones aplicadas.
int magXYZ_RED[3]; //x, y, z del mag, reducido al horizonte.
int magOffsets[3];
int contadorCiclos = 0;
float magScales[3];
int magRumbo;	//rumbo magnético.
int magRumbo_RED; //rumbo magnético reducido al horizonte.

void   CalibrarQmc();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  myMPU9250.init();

  Serial.println("Position you MPU9250 flat and don't move it - calibrating...");
  delay(1000);
  myMPU9250.autoOffsets();
  Serial.println("Done!");

  myMPU9250.setSampleRateDivider(10);
  myMPU9250.setAccRange(MPU9250_ACC_RANGE_2G);
  myMPU9250.enableAccDLPF(true);
  myMPU9250.setAccDLPF(MPU9250_DLPF_6);

  qmc.init();
  qmc.setMode(Mode_Continuous, ODR_200Hz, RNG_2G, OSR_256);
  CalibrarQmc();

  //Valores Iniciales del magnetometro
  magXYZ[0] = 0;
  magXYZ[1] = 0;
  magXYZ[2] = 0;
}

void loop() {
  //Leo el QMC y calculo el rumbo magnético.
  qmc.read(&magXYZA_RAW[0], &magXYZA_RAW[1], &magXYZA_RAW[2], &magXYZA_RAW[3]);

  float magXRAW = ((float)magXYZA_RAW[0] - magOffsets[0]) * magScales[0];
  float magYRAW = ((float)magXYZA_RAW[1] - magOffsets[1]) * magScales[1];
  float magZRAW = ((float)magXYZA_RAW[2] - magOffsets[2]) * magScales[2];

  //Corrección de ejes: GYRO_X = MAG_Y | GYRO_Y = MAG_X | GYRO_Z = -MAG_Z. Intercambio los ejes. X e Y del mag.
  int aux = magXRAW;
  magXRAW = -magYRAW;
  magYRAW = aux;
  magZRAW = magZRAW;
  
  //Promedio las lecturas del magnetómetro.
  magXYZ[0] = magXYZ[0] * 0.9 + magXRAW * 0.1;
  magXYZ[1] = magXYZ[1] * 0.9 + magYRAW * 0.1;
  magXYZ[2] = magXYZ[2] * 0.9 + magZRAW * 0.1;

  //Cada 10 ciclos de programa hago las cuentas y muestro el heading.
  contadorCiclos++;
  if (contadorCiclos == 10) {
    xyzFloat angles = myMPU9250.getAngles();

    /*
    float pitch = -myMPU9250.getPitch();
    float roll  = myMPU9250.getRoll();
    */
    
    float pitch = angles.x;
    float roll  = angles.y;

    float pitchRad = pitch * PI / (float)180.0;
    float rollRad = roll * PI / (float)180.0;

    Serial.print(F("mX ")); Serial.print(magXYZ[0]);  Serial.print(F(",   "));
    Serial.print(F("mY ")); Serial.print(magXYZ[1]);  Serial.print(F(",   "));
    Serial.print(F("mZ ")); Serial.print(magXYZ[2]);  Serial.print(F(",   "));
    Serial.println();
    Serial.print(F("P ")); Serial.print(pitch);   Serial.print(F(",   "));
    Serial.print(F("R "));  Serial.print(roll);   Serial.print(F(",   "));
    Serial.println();

    //Álgebra que reduce los valores del magnetómetro al horizonte, usando el giroscopio:
    
    magXYZ_RED[0] = magXYZ[0] * cos(pitchRad) + magXYZ[1] * sin(pitchRad) * sin(rollRad) - magXYZ[2] * cos(rollRad) * sin(pitchRad);
    magXYZ_RED[1] = magXYZ[1] * cos(rollRad) - magXYZ[2] * sin(rollRad);

    magRumbo = atan2(magXYZ[1], magXYZ[0]) * 180 / PI ;
    magRumbo_RED = atan2(magXYZ_RED[1], magXYZ_RED[0]) * 180 / PI ;

    //Proceso el rumbo para que crezca en sentido horario y tenga rango 0 a 360.
    if (magRumbo < 0)
      magRumbo += 360;
    if (magRumbo_RED < 0)
      magRumbo_RED += 360;

    Serial.print("R: ");
    Serial.print(magRumbo);
    Serial.print(" R.C: ");
    Serial.print(magRumbo_RED);
    Serial.println();
    Serial.println();

    contadorCiclos = 0;
  }
  delay(10);
}

void CalibrarQmc() {
  int mag_x_min = 32767;                                         // Raw data extremes
  int mag_y_min = 32767;
  int mag_z_min = 32767;
  int mag_x_max = -32768;
  int mag_y_max = -32768;
  int mag_z_max = -32768;

  float chord_x, chord_y, chord_z;                              // Used for calculating scale factors
  float chord_average;

  Serial.println("Gire el magnetometro...");
  for (int i = 0; i < 3000; i++)
  {
    qmc.read(&magXYZA_RAW[0], &magXYZA_RAW[1], &magXYZA_RAW[2], &magXYZA_RAW[3]);

    // Busco valores máximos y mínimos de xyz.
    mag_x_min = min(magXYZA_RAW[0], mag_x_min);
    mag_x_max = max(magXYZA_RAW[0], mag_x_max);
    mag_y_min = min(magXYZA_RAW[1], mag_y_min);
    mag_y_max = max(magXYZA_RAW[1], mag_y_max);
    mag_z_min = min(magXYZA_RAW[2], mag_z_min);
    mag_z_max = max(magXYZA_RAW[2], mag_z_max);

    delay(10);
  }

  Serial.println("Listo!");

  // ----- Calculate hard-iron offsets
  magOffsets[0] = (mag_x_max + mag_x_min) / 2;                     // Get average magnetic bias in counts
  magOffsets[1] = (mag_y_max + mag_y_min) / 2;
  magOffsets[2] = (mag_z_max + mag_z_min) / 2;

  // ----- Calculate soft-iron scale factors
  chord_x = ((float)(mag_x_max - mag_x_min)) / 2;                 // Get average max chord length in counts
  chord_y = ((float)(mag_y_max - mag_y_min)) / 2;
  chord_z = ((float)(mag_z_max - mag_z_min)) / 2;

  chord_average = (chord_x + chord_y + chord_z) / 3;              // Calculate average chord length

  magScales[0] = chord_average / chord_x;                          // Calculate X scale factor
  magScales[1] = chord_average / chord_y;                          // Calculate Y scale factor
  magScales[2] = chord_average / chord_z;                          // Calculate Z scale factor
}
