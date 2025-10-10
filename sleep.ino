#include <hardware/rtc.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/sleep.h>
#include <pico/stdlib.h>

#define BUTTON_PIN 15  // GPIO Pin für den Taster (kannst du ändern)

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("Raspberry Pi Pico 2W Sleep Example mit Taster und RTC-Aufwachen");
  
  // Taster-Pin als Input mit Pull-up konfigurieren
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);
  
  // RTC initialisieren
  rtc_init();
  datetime_t t = {
    .year  = 2025,
    .month = 1,
    .day   = 1,
    .dotw  = 3,
    .hour  = 0,
    .min   = 0,
    .sec   = 0
  };
  rtc_set_datetime(&t);
}

void loop() {
  Serial.println("Wach! Führe Aufgaben aus...");
  delay(1000);
  
  Serial.println("Gehe in Sleep-Modus für 5 Sekunden (oder Taster drücken)...");
  delay(100);
  
  // RTC Alarm für Aufwachen nach 5 Sekunden setzen
  datetime_t alarm_time;
  rtc_get_datetime(&alarm_time);
  alarm_time.sec += 5;
  if (alarm_time.sec >= 60) {
    alarm_time.sec -= 60;
    alarm_time.min += 1;
  }
  
  // GPIO für Dormant Wake konfigurieren
  gpio_set_dormant_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
  
  // In Sleep mit RTC Alarm gehen (GPIO bleibt als Weckquelle aktiv)
  sleep_goto_sleep_until(&alarm_time);
  
  // Nach dem Aufwachen hier weitermachen
  Serial.println("Aufgewacht!");
  delay(500); // Entprellung
}