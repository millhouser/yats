
char* dateTimeStr(time_t t) {
  struct tm timeinfo;
  static char dts[15];

  //gmtime_r(&t, &timeinfo);
  localtime_r(&t, &timeinfo);

  sprintf(dts, "%.2u.%.2u.%.4u %.2u:%.2u", timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min);

  return dts;
}

int measure() {
  int ret = 0;

  time_t now = time(nullptr);

  float temp = dht.readTemperature();
  float humi = dht.readHumidity();

  if (isnan(temp) || isnan(humi)) {
    Serial.println("Lesefehler DHT22!");
    return -1;
  } else {
    current_measurement.time = now;
    current_measurement.temp = temp;
    current_measurement.humi = humi;
  }

  return 0;
}

void resetMinMaxTemp() {
  min_temp = current_measurement;
  max_temp = current_measurement;
}

void resetMinMaxHumi() {
  min_humi = current_measurement;
  max_humi = current_measurement;
}

void setMinMax() {
  // set min_temp
  if ((min_temp.time == 0) || (current_measurement.temp < min_temp.temp)) {
    min_temp = current_measurement;
  }

  // set min_humi
  if ((min_humi.time == 0) || (current_measurement.humi < min_humi.humi)) {
    min_humi = current_measurement;
  }

  // set max_temp
  if ((max_temp.time == 0) || (current_measurement.temp > max_temp.temp)) {
    max_temp = current_measurement;
  }

  // set max_humi
  if ((max_humi.time == 0) || (current_measurement.humi > max_humi.humi)) {
    max_humi = current_measurement;
  }
}