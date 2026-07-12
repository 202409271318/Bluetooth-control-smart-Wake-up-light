/*
  Arduino IDE tab 4/12: Sleep_plan

  BLE schedule example:
    SLEEP=2300,2320,2340
      23:00-23:20: warm light kept on (max brightness)
      23:20-23:40: gradual dimming to black (gamma-corrected, second-level)
      after 23:40: off

    WAKE=0700,0730,0800
      07:00-07:30: gradual sunrise brightening (gamma-corrected, second-level)
      07:30-08:00: max warm light kept on
      after 08:00: off
*/

static bool planWasActive = false;
static const float PLAN_GAMMA = 2.2f;   // 经典伽马值，越大渐变越"慢热"

void initSleepPlan() {
  planWasActive = false;
}

bool timeInInterval(int startMinute, int endMinute, int nowMinute) {   //跨午夜判断
  startMinute = normalizeMinute(startMinute);
  endMinute = normalizeMinute(endMinute);
  nowMinute = normalizeMinute(nowMinute);

  if (startMinute == endMinute) {
    return false;
  }

  if (startMinute < endMinute) {
    return nowMinute >= startMinute && nowMinute < endMinute;
  }

  // Crosses midnight.
  return nowMinute >= startMinute || nowMinute < endMinute;
}

int intervalLengthMinutes(int startMinute, int endMinute) {
  startMinute = normalizeMinute(startMinute);
  endMinute = normalizeMinute(endMinute);

  if (endMinute >= startMinute) {
    return endMinute - startMinute;
  }
  return (24 * 60 - startMinute) + endMinute;
}

int elapsedInIntervalMinutes(int startMinute, int nowMinute) {
  startMinute = normalizeMinute(startMinute);
  nowMinute = normalizeMinute(nowMinute);

  if (nowMinute >= startMinute) {
    return nowMinute - startMinute;
  }
  return (24 * 60 - startMinute) + nowMinute;
}

// ===================== 秒级渐变辅助函数 =====================
long intervalLengthSeconds(int startMinute, int endMinute) {
  return (long)intervalLengthMinutes(startMinute, endMinute) * 60L;
}

long elapsedInIntervalSeconds(int startMinute, int nowMinute, int nowSecond) {
  startMinute = normalizeMinute(startMinute);
  nowMinute = normalizeMinute(nowMinute);

  long startSec = (long)startMinute * 60L;
  long nowSec   = (long)nowMinute * 60L + (long)nowSecond;

  if (nowSec >= startSec) {
    return nowSec - startSec;
  }
  return (24L * 60L * 60L - startSec) + nowSec;
}

// ===================== Gamma 校正 =====================
// 把"线性进度 0.0 → 1.0"映射到"视觉进度"，曲线 = pow(progress, 2.2)
// 让人眼感知到的渐变趋于线性
uint8_t gammaCorrect(float progress, uint8_t maxBri) {
  progress = constrain(progress, 0.0f, 1.0f);
  float corrected = powf(progress, PLAN_GAMMA);
  return (uint8_t)(corrected * (float)maxBri);
}
// =====================================================

bool isPlanActiveNow() {
  int nowMin = getMinuteOfDay();

  return timeInInterval(planSettings.sleepLightTime, planSettings.sleepTurnTime, nowMin) ||
         timeInInterval(planSettings.sleepTurnTime, planSettings.sleepDarkTime, nowMin) ||
         timeInInterval(planSettings.wakeStartTime, planSettings.wakeMaxTime, nowMin) ||
         timeInInterval(planSettings.wakeMaxTime, planSettings.wakeOffTime, nowMin);
}

void updateSleepPlan() {   //在这里更新亮度，通过时间判断进度，再进行伽马矫正，算出当前亮度，调用showSolidColor设置颜色
  if (!clockState.timeValid) {
    return;
  }

  int nowMin = getMinuteOfDay();
  int nowSec = clockState.second;
  bool active = false;
  uint8_t planBrightness = 0;
  uint8_t r = PLAN_WARM_R;
  uint8_t g = PLAN_WARM_G;
  uint8_t b = PLAN_WARM_B;

  if (timeInInterval(planSettings.sleepLightTime, planSettings.sleepTurnTime, nowMin)) {
    // ① 暖光保持
    active = true;
    planBrightness = PLAN_MAX_BRIGHTNESS;

  } else if (timeInInterval(planSettings.sleepTurnTime, planSettings.sleepDarkTime, nowMin)) {
    // ② 渐暗（秒级 + gamma 校正）
    active = true;
    long elapsedSec = elapsedInIntervalSeconds(planSettings.sleepTurnTime, nowMin, nowSec);
    long lengthSec  = max(1L, intervalLengthSeconds(planSettings.sleepTurnTime,
                                                    planSettings.sleepDarkTime));
    float progress = 1.0f - (float)elapsedSec / (float)lengthSec;   // 1.0 → 0.0
    planBrightness = gammaCorrect(progress, PLAN_MAX_BRIGHTNESS);

  } else if (timeInInterval(planSettings.wakeStartTime, planSettings.wakeMaxTime, nowMin)) {
    // ③ 渐亮（秒级 + gamma 校正）
    active = true;
    long elapsedSec = elapsedInIntervalSeconds(planSettings.wakeStartTime, nowMin, nowSec);
    long lengthSec  = max(1L, intervalLengthSeconds(planSettings.wakeStartTime,
                                                    planSettings.wakeMaxTime));
    float progress = (float)elapsedSec / (float)lengthSec;          // 0.0 → 1.0
    planBrightness = gammaCorrect(progress, PLAN_MAX_BRIGHTNESS);

  } else if (timeInInterval(planSettings.wakeMaxTime, planSettings.wakeOffTime, nowMin)) {
    // ④ 最亮保持
    active = true;
    planBrightness = PLAN_MAX_BRIGHTNESS;
  }

  if (active) {
    if (!planWasActive) {
      previousManualMode = currentMode;
      planWasActive = true;
      Serial.println("Sleep/Wake plan started.");
    }

    currentMode = SLEEP_PLAN_MODE;
    showSolidColor(r, g, b, planBrightness);
    return;
  }

  if (planWasActive) {
    planWasActive = false;
    currentMode = previousManualMode;
    brightnessLevel = 0;
    showOff();
    Serial.println("Sleep/Wake plan ended.");
  }
}
