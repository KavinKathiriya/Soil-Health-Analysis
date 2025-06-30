#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>

#define CHANNEL 129

PROCESS(base_station_process, "Base Station Process");
AUTOSTART_PROCESSES(&base_station_process);

static int packets_received = 0;

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  packets_received++;
  printf("Packet %d received from %d.%d: %s\n", 
         packets_received, from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
  leds_toggle(LEDS_GREEN); // Visual feedback for received packet
  
  // Log data to file for analysis
  FILE *log_file = fopen("soil_data.log", "a");
  if (log_file) {
    fprintf(log_file, "%ld, %d.%d, %s\n", clock_time(), 
            from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
    fclose(log_file);
  }
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

PROCESS_THREAD(base_station_process, ev, data) {
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();

  broadcast_open(&broadcast, CHANNEL, &broadcast_call);
  
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}