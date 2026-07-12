/*
  Arduino IDE tab 7/12: Count_Time
  Fixed:
    跨午夜自动翻日 / 跨月 / 跨年看，
*/

static bool isLeapYear(int year) {  //是不是闰年
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int daysInMonth(int year, int month) {   //所有月份设置
  static const uint8_t table[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 30;
  if (month == 2 && isLeapYear(year)) return 29;
  return table[month - 1];
}

void initCountTime() {
  clockState.baseMillis = millis();
  clockState.timeValid  = false;
}

void setCurrentTimeFromBle(int year, int month, int day, int hour, int minute, int second) {
  clockState.year   = constrain(year, 2024, 2099);
  clockState.month  = constrain(month, 1, 12);
  clockState.day    = constrain(day, 1, daysInMonth(clockState.year, clockState.month));
  clockState.hour   = constrain(hour, 0, 23);
  clockState.minute = constrain(minute, 0, 59);
  clockState.second = constrain(second, 0, 59);
  clockState.baseMillis = millis();
  clockState.timeValid  = true;

  Serial.print("Time set: ");
  Serial.println(getDateTimeString());
}

void updateCountTime() {
  if (!clockState.timeValid) return;

  unsigned long now = millis();
  unsigned long elapsedMs = now - clockState.baseMillis;
  unsigned long elapsedSeconds = elapsedMs / 1000UL;

  if (elapsedSeconds == 0) return;          // 不足 1 秒，直接退出

  // 关键：基准只前进"已消费的整秒"，剩余的毫秒小数留给下次
  clockState.baseMillis += elapsedSeconds * 1000UL;

  // 推进秒/分/时
  long total = (long)clockState.second + (long)elapsedSeconds;
  clockState.second = total % 60;
  long minuteCarry = total / 60;

  total = (long)clockState.minute + minuteCarry;
  clockState.minute = total % 60;
  long hourCarry = total / 60;

  total = (long)clockState.hour + hourCarry;
  clockState.hour = total % 24;
  long dayCarry = total / 24;

  // 推进日/月/年
  while (dayCarry-- > 0) {
    clockState.day++;
    int dim = daysInMonth(clockState.year, clockState.month);
    if (clockState.day > dim) {
      clockState.day = 1;
      clockState.month++;
      if (clockState.month > 12) {
        clockState.month = 1;
        clockState.year++;
      }
    }
  }
}

int getMinuteOfDay() {   //把时间统一转成当天第几分钟
  return normalizeMinute(clockState.hour * 60 + clockState.minute);
}

String twoDigits(int v) {
  return (v < 10 ? "0" : "") + String(v);
}

String getTimeString() {
  if (!clockState.timeValid) return "--:--:--";
  return twoDigits(clockState.hour) + ":" +
         twoDigits(clockState.minute) + ":" +
         twoDigits(clockState.second);
}

String getDateString() {
  if (!clockState.timeValid) return "----/--/--";
  return String(clockState.year) + "-" +
         twoDigits(clockState.month) + "-" +
         twoDigits(clockState.day);
}

String getDateTimeString() {
  return getDateString() + " " + getTimeString();
}

String minuteToTimeString(int minuteOfDay) {
  minuteOfDay = normalizeMinute(minuteOfDay);
  return twoDigits(minuteOfDay / 60) + ":" + twoDigits(minuteOfDay % 60);
}
