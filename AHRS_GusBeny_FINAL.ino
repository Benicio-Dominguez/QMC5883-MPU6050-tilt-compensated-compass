#include <EEPROM.h>
#include <MechaQMC5883.h>
#include <MPU6050_tockn.h>
#include <Wire.h>

MPU6050 mpu6050(Wire);
MechaQMC5883 qmc;
//Variables de Calibracion


int mgx = 0, mgy = 0, mgz = 0, az;
float mxOffset = 0, myOffset = 0, mzOffset = 0;
float mxScale , myScale, mzScale ;
float mxCal, myCal, mzCal;
float mX = 0; float mY = 0 ; float mZ = 0;
int contadorCiclos = 0;
void CalibrateQmc();


void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu6050.begin();

  mpu6050.calcGyroOffsets(true);//Calibracion giroscopo
  delay (200);

  qmc.init();
  qmc.setMode(Mode_Continuous, ODR_200Hz, RNG_2G, OSR_256);

  //CalibrateQmc();
  //guardarQMC();

  obtenerQMC();
}

void loop() {
  //Leo el QMC y calculo el rumbo magnético.
  int mx, my, mz, rumboQMC;
  qmc.read(& mx, & my, & mz, & rumboQMC);

  // Aplicar correccion por Calibracion
  mxCal = ( mx - mxOffset) / mxScale ;
  myCal = ( my - myOffset) / myScale ;
  mzCal = ( mz - mzOffset) / mzScale ;

  //Promedio con las diez lecturas anteriores.
  mX = mX * 0.9 + mxCal * 0.1;
  mY = mY * 0.9 + myCal * 0.1;
  mZ = mZ * 0.9 + mzCal * 0.1;

  contadorCiclos ++;

  //Calculo rumboQMC

  if (contadorCiclos == 10) {
    mpu6050.update();

    float anglex = mpu6050.getAngleX();
    float angley = mpu6050.getAngleY();

    float rollRad = anglex * PI / 180;
    float pitchRad = angley * PI / 180;

    //Transformacion de angulos para armonizar los ejes
    float roll QMC = -pitchRad;
    float pitchQMC = rollRad;

    //Compenso por inclinacion del magnetometro
    float mx_corr = mX * cos(pitchQMC) + mZ * sin(pitchQMC);
    float my_corr = mX * sin(rollQMC) * sin(pitchQMC) + mY * cos(rollQMC) - mZ * sin(rollQMC) * cos(pitchQMC);

    //Calculo el rumboQMC al Horizonte
    float rumboCorr = atan2(my_corr, mx_corr) * 180 / PI ;

    //Proceso el rumbo para que crezca en sentido horario y tenga rango 0 a 360.
    if (rumboCorr < 0)
      rumboCorr += 360;

    //Muestro Resultados
    Serial.print("P: ");
    Serial.print(pitchQMC / PI * 180);
    Serial.print("° R: ");
    Serial.print(rollQMC / PI * 180);
    Serial.print("°\n");
    Serial.print ("R.C.: ");
    Serial.print(rumboCorr);
    Serial.println("°");
    Serial.print ("R.N.: ");
    Serial.print(rumboQMC);
    Serial.println("°");
    Serial.println("\n\n");

    contadorCiclos = 0;
  }
}


void CalibrateQmc() {
  int mgx_min = 32767;                                         // Raw data extremes
  int mgy_min = 32767;
  int mgz_min = 32767;
  int mgx_max = -32768;
  int mgy_max = -32768;
  int mgz_max = -32768;

  float chord_x, chord_y, chord_z;                              // Used for calculating scale factors
  float chord_average;

  //Comienza Calibracion

  Serial.println("Gire el magnetometro...");
  for (int i = 0; i < 3000; i++)
  {
    qmc.read(&mgx, &mgy, &mgz, &az);

    // Busco valores máximos y mínimos de xyz.
    mgx_min = min(mgx, mgx_min);
    mgx_max = max(mgx, mgx_max);
    mgy_min = min(mgy, mgy_min);
    mgy_max = max(mgy, mgy_max);
    mgz_min = min(mgz, mgz_min);
    mgz_max = max(mgz, mgz_max);

    delay(10);

  }
  Serial.print(F("Mx :")); Serial.print(mgx_max); Serial.print(F("   "));
  Serial.print(F("mx :")); Serial.print(mgx_min); Serial.println("   ");
  Serial.print(F("My :")); Serial.print(mgy_max); Serial.print(F("   "));
  Serial.print(F("my :")); Serial.print(mgy_min); Serial.println("   ");
  Serial.print(F("Mz :")); Serial.print(mgz_max); Serial.print(F("   "));
  Serial.print(F("mz :")); Serial.print(mgz_min); Serial.println("   ");
  Serial.println("Listo!");

  delay(2000);

  // ----- Calculate hard-iron offsets
  mxOffset = (mgx_max + mgx_min) / 2;                     // Get average magnetic bias in counts
  myOffset = (mgy_max + mgy_min) / 2;
  mzOffset = (mgz_max + mgz_min) / 2;

  // ----- Calculate soft-iron scale factors
  chord_x = (mgx_max - mgx_min) / 2;                 // Get average max chord length in counts
  chord_y = (mgy_max - mgy_min) / 2;
  chord_z = (mgz_max - mgz_min) / 2;

  chord_average = (chord_x + chord_y + chord_z) / 3;              // Calculate average chord length

  mxScale = chord_average / chord_x;                          // Calculate X scale factor
  myScale = chord_average / chord_y;                          // Calculate Y scale factor
  mzScale = chord_average / chord_z;                          // Calculate Z scale factor
}

void guardarQMC() {
  EEPROM.put(1, mxOffset);
  EEPROM.put(5, myOffset);
  EEPROM.put(9, mzOffset);

  EEPROM.put(13, mxScale);
  EEPROM.put(17, myScale);
  EEPROM.put(21, mzScale);
}

void obtenerQMC() {
  EEPROM.get(1, mxOffset);
  EEPROM.get(5, myOffset);
  EEPROM.get(9, mzOffset);

  EEPROM.get(13, mxScale);
  EEPROM.get(17, myScale);
  EEPROM.get(21, mzScale);
}
