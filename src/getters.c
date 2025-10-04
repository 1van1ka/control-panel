#include "../include/getters.h"
#include "../include/utils.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

int get_level_audio(void) {
  FILE *fp =
      popen("wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print $2}'", "r");
  if (!fp) {
    perror("popen failed");
    return 0;
  }

  char buffer[32];
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return 0;
  }

  pclose(fp);
  buffer[strcspn(buffer, "\n")] = '\0';

  return (int)(atof(buffer) * 100);
}

int get_state_audio_mute(void) {
  FILE *fp = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r");
  if (!fp) {
    perror("popen failed");
    return 0;
  }

  char buffer[128] = {0};
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return 0;
  }

  pclose(fp);
  buffer[strcspn(buffer, "\n")] = '\0';

  return strstr(buffer, "[MUTED]") != NULL;
}

int get_level_brightness(void) {
  const char *brightness_path =
      "/sys/class/backlight/intel_backlight/brightness";
  const char *max_brightness_path =
      "/sys/class/backlight/intel_backlight/max_brightness";

  int brightness = read_int_from_file(brightness_path);
  int max_brightness = read_int_from_file(max_brightness_path);

  if (brightness < 0 || max_brightness <= 0) {
    fprintf(stderr, "Error: invalid brightness values\n");
    return 0;
  }
  return (brightness * 100) / max_brightness;
}

int get_state_bluetooth(void) {
  FILE *fp = popen("rfkill list bluetooth | grep \"Soft blocked\" | awk '{print $3}'", "r");
  if (!fp) {
    perror("popen failed");
    return 0;
  }
  char buffer[32];
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return 0;
  }

  pclose(fp);
  buffer[strcspn(buffer, "\n")] = 0;

  return !(strcmp(buffer, "yes") == 0);
}

int get_state_dunst(void) {
  FILE *fp = popen("dunstctl is-paused", "r");
  if (!fp) {
    perror("popen failed");
    return 0;
  }
  char buffer[32];
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return 0;
  }

  pclose(fp);
  buffer[strcspn(buffer, "\n")] = 0;

  return (strcmp(buffer, "true"));
}

int get_state_wifi(void) {
  FILE *fp = popen("rfkill list wifi | grep \"Soft blocked\" | awk '{print $3}'", "r");
  if (!fp) {
    perror("popen failed");
    return 0;
  }

  char buffer[32];
  if (fgets(buffer, sizeof(buffer), fp) == NULL) {
    pclose(fp);
    return 0;
  }

  pclose(fp);
  buffer[strcspn(buffer, "\n")] = 0;

  return !(strcmp(buffer, "yes") == 0);
}
