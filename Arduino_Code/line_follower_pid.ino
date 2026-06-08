// ===================== Motor Driver =====================
#define ENA 5
#define IN1 9
#define IN2 10
#define ENB 6
#define IN3 11
#define IN4 12

// ===================== Sensors ==========================
#define S1 2
#define S2 3
#define S3 13
#define S4 4
#define S5 8

const int sensorPins[5] = {S1, S2, S3, S4, S5};

// إذا الحساسات بتدي LOW على الأسود
const int BLACK = LOW;
const int WHITE = HIGH;

// ===================== Control ==========================
// سرعة عالية ومستقرة لموتور JGA25-370 (780 RPM)
int baseSpeed = 190;      // السرعة الأساسية
int maxSpeed  = 255;      // أقصى PWM (آمن للموتور والدرايفر)
int minSpeed  = 0;

// ===================== PID Tuned ========================
// قيم مضبوطة لتقليل الاهتزاز يمين/شمال
float Kp = 4.2;
float Kd = 1.8;
float Ki = 0.015;

// ===================== PID Variables ====================
float lastError = 0;
float integral = 0;

// ===================== Timing ===========================
unsigned long lastTime = 0;
const int loopTime = 2;   // تحديث سريع لتحكم أنعم

// ===================== Motor Calibration ================
float motorTrimLeft  = 1.00;
float motorTrimRight = 1.00;

// ===================== Setup ============================
void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  Serial.begin(9600);

  stopMotors();
  delay(2000); // انتظار قبل البدء
}

// ===================== Main Loop ========================
void loop() {
  if (millis() - lastTime < loopTime) return;
  lastTime = millis();

  // قراءة الحساسات
  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);

  // حساب الخطأ
  int error = getError(s1, s2, s3, s4, s5);

  // Debug على السيريال
  Serial.print(s1); Serial.print(" ");
  Serial.print(s2); Serial.print(" ");
  Serial.print(s3); Serial.print(" ");
  Serial.print(s4); Serial.print(" ");
  Serial.print(s5); Serial.print("  E=");
  Serial.println(error);

  // لو الخط ضاع
  if (error == 100) {
    if (lastError < 0)
      turnLeftHard();
    else
      turnRightHard();
    return;
  }

  // ================= PID =================
  // Integral
  integral += error;
  integral = constrain(integral, -20, 20);

  // Derivative (مخفف لتقليل الاهتزاز)
  float derivative = (error - lastError) * 0.7;

  // Correction
  float correction = (Kp * error) +
                     (Kd * derivative) +
                     (Ki * integral);

  // منع التصحيح الزائد
  correction = constrain(correction, -70, 70);

  // تحديث الخطأ السابق
  lastError = error;

  // حساب السرعات
  int leftSpeed  = baseSpeed - correction;
  int rightSpeed = baseSpeed + correction;

  // ضبط الحدود
  leftSpeed  = constrain((int)(leftSpeed  * motorTrimLeft),  minSpeed, maxSpeed);
  rightSpeed = constrain((int)(rightSpeed * motorTrimRight), minSpeed, maxSpeed);

  // Deadband لتقليل الاهتزاز
  if (error == 0) {
    // يمشي مستقيم تمامًا
    forward(baseSpeed, baseSpeed);
  }
  else if (abs(error) == 1) {
    // تصحيح بسيط جدًا
    int smallCorrection = correction * 0.45;

    leftSpeed  = baseSpeed - smallCorrection;
    rightSpeed = baseSpeed + smallCorrection;

    leftSpeed  = constrain(leftSpeed, minSpeed, maxSpeed);
    rightSpeed = constrain(rightSpeed, minSpeed, maxSpeed);

    forward(leftSpeed, rightSpeed);
  }
  else {
    // تصحيح كامل للانحراف الكبير
    forward(leftSpeed, rightSpeed);
  }
}

// ===================== Error Function ===================
int getError(int s1, int s2, int s3, int s4, int s5) {

  // المنتصف
  if (s3 == BLACK && s2 == WHITE && s4 == WHITE)
    return 0;

  // انحراف بسيط
  if (s2 == BLACK && s3 == WHITE)
    return -1;

  if (s4 == BLACK && s3 == WHITE)
    return 1;

  // انحراف كبير
  if (s1 == BLACK)
    return -2;

  if (s5 == BLACK)
    return 2;

  // حالات مزدوجة
  if (s2 == BLACK && s3 == BLACK)
    return -1;

  if (s3 == BLACK && s4 == BLACK)
    return 1;

  if (s1 == BLACK && s2 == BLACK)
    return -2;

  if (s4 == BLACK && s5 == BLACK)
    return 2;

  // الخط مفقود
  return 100;
}

// ===================== Motor Control ====================
void setLeftMotor(int speed) {
  speed = constrain(speed, -255, 255);

  if (speed >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, speed);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, -speed);
  }
}

void setRightMotor(int speed) {
  speed = constrain(speed, -255, 255);

  if (speed >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, speed);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, -speed);
  }
}

void forward(int leftSpeed, int rightSpeed) {
  setLeftMotor(leftSpeed);
  setRightMotor(rightSpeed);
}

// دوران قوي عند فقدان الخط
void turnLeftHard() {
  setLeftMotor(-100);
  setRightMotor(160);
}

void turnRightHard() {
  setLeftMotor(160);
  setRightMotor(-100);
}

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
}
