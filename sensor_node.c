#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/leds.h"
#include "powertrace.h"
#include <stdio.h>
#include <string.h>

#define CHANNEL 129
#define INTERVAL (CLOCK_SECOND * 10)

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  printf("Broadcast received from %d.%d: %s\n", 
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer et;
  static float moisture, ph, temp;
  static char recommendation[100];
  static int packet_counter = 0;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();

  powertrace_start(CLOCK_SECOND * 60); // Track energy consumption
  broadcast_open(&broadcast, CHANNEL, &broadcast_call);
  etimer_set(&et, INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // Simulate sensor data
    moisture = 20.0 + (float)(random_rand() % 8000) / 100.0; // 20-100%
    ph = 4.0 + (float)(random_rand() % 50) / 10.0; // 4.0-9.0
    temp = 5.0 + (float)(random_rand() % 3500) / 100.0; // 5-40°C

    // Threshold-based recommendations
    strcpy(recommendation, "Soil conditions optimal");
    if (moisture < 30.0) {
      strcpy(recommendation, "Irrigation needed");
      leds_on(LEDS_RED);
    } else if (moisture > 75.0) {
      strcpy(recommendation, "Reduce irrigation");
      leds_on(LEDS_BLUE);
    } else {
      leds_off(LEDS_RED | LEDS_BLUE);
    }

    if (ph > 7.5) {
      strcpy(recommendation, "Add sulfur to reduce alkalinity");
      leds_on(LEDS_GREEN);
    } else if (ph < 5.5) {
      strcpy(recommendation, "Add lime to increase pH");
      leds_on(LEDS_GREEN);
    }

    if (temp > 35.0) {
      strcpy(recommendation, "Provide shade");
      leds_on(LEDS_YELLOW);
    } else if (temp < 10.0) {
      strcpy(recommendation, "Use crop covers");
      leds_on(LEDS_YELLOW);
    }

    // Prepare and send data packet
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "ID:%d.%d Moisture:%.1f pH:%.1f Temp:%.1f Rec:%s",
             linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
             moisture, ph, temp, recommendation);
    packetbuf_copyfrom(buffer, strlen(buffer) + 1);
    broadcast_send(&broadcast);
    packet_counter++;

    printf("Packet %d sent: %s\n", packet_counter, buffer);

    // Reset timer
    etimer_reset(&et);
  }

  PROCESS_END();
}