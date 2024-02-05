#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define MAX_PATH_LENGTH 2056
#define D_BUFFER 2056
#define NOTE_NAME_MAX 256
#define DATE_TIME_FORMAT "%Y%m%d_%H%M%S"

typedef struct {
  char save_path[MAX_PATH_LENGTH];
  int add_dates;
} AppConfig;

char* get_documents_folder() {
  #ifdef _WIN32
    PWSTR documentsPath;
    if (SHGetKnownFolderPath(&FOLDERID_Documents, 0, NULL, &documentsPath) == S_OK) {
        char *path = _strdup("");
        wcstombs(path, documentsPath, MAX_PATH_LENGTH);
        CoTaskMemFree(documentsPath);
        return path;
    } else {
        fprintf(stderr, "Error getting Documents folder path on Windows.\n");
        return NULL;
    }
  #else
    const char *homeDir = getenv("HOME");
    if (homeDir != NULL) {
        char *path = malloc(MAX_PATH_LENGTH);
        snprintf(path, MAX_PATH_LENGTH, "%s/Documents", homeDir);
        return path;
    } else {
        fprintf(stderr, "Error getting HOME directory on Linux.\n");
        return NULL;
    }
  #endif
}

void read_config(const char *filename, AppConfig *config) {
  FILE *file = fopen(filename, "r");

  // Set defaults first to account for unset config options.
  char *documentsPath = get_documents_folder();   
  if (documentsPath != NULL) {
    snprintf(config->save_path, sizeof(config->save_path), "%s/", documentsPath);
    free(documentsPath);
  } else {
    perror("Cannot get default save path.");
    exit(EXIT_FAILURE);
  }
  config->add_dates = 0;  // Default: Do not add dates

  char line[MAX_PATH_LENGTH];
  while(fgets(line, sizeof(line), file) != NULL) {
    // skip comments etc
    if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
      continue;
    }

    //parse config options
    char key[MAX_PATH_LENGTH];
    char value[MAX_PATH_LENGTH];

    // account for stupid spaces in paths probably unsafe
    if (sscanf(line, "%s = %[^\n]", key, value) == 2) {
      if(strcasecmp(key, "save_path") == 0) {
        strncpy(config->save_path, value, sizeof(config->save_path) - 1);
        config->save_path[sizeof(config->save_path) - 1] = '\0';

      } else if(strcasecmp(key, "date_files") == 0) {
 
        if(strcasecmp(value,"false") == 0) {
          config->add_dates = 0;
        }

        if(strcasecmp(value,"true") == 0) {
          config->add_dates = 1;
        }

      }
    } else {
      fprintf(stderr, "Malformed line: %s", line);      
    }
  }  

  fclose(file);
}

char *get_dynamic_string(size_t buffer_size) {
  char *dyn_str = (char *)malloc(buffer_size * sizeof(char));

  if(dyn_str == NULL) {
    perror("Initial memory allocation failed.");
    exit(EXIT_FAILURE);
  }

  dyn_str[0] = '\0';

  printf("Enter text. Type 'save' on a new line to end:\n");

  char input[D_BUFFER];

  int ch;

  while(1) {
    fgets(input, sizeof(input), stdin);

    // Check if 'save' command is given on a new line
    if (strcmp(input, "save\n") == 0) {
      break;
    }

    size_t input_length = strlen(input);

    if(input_length > buffer_size) {
      char *temp = (char *)realloc(dyn_str, input_length + 1);

      if (temp == NULL) {
        perror("Memory reallocation failed.");
        free(dyn_str);
        exit(EXIT_FAILURE);
      } else {
        dyn_str = temp;
      }
    }

    strcat(dyn_str, input);
  }

  return dyn_str;
}

char *add_date_to_save_path(const char *save_path) {
  char date_time[20];
  time_t current_time = time(NULL);
  struct tm *time_info = localtime(&current_time);

  strftime(date_time, sizeof(date_time), DATE_TIME_FORMAT, time_info);

  char *updated_save_path = (char *)malloc(MAX_PATH_LENGTH);
  if(updated_save_path == NULL) {
    perror("Memory allocation failed.");
    exit(EXIT_FAILURE);
  }

  snprintf(updated_save_path, MAX_PATH_LENGTH, "%s_%s", save_path, date_time);

  return updated_save_path;
}

char *get_save_name(AppConfig *config) {
  char note_name[NOTE_NAME_MAX ];
  printf("Enter note name: ");
  printf("\n");
  fgets(note_name, NOTE_NAME_MAX, stdin);

  note_name[strcspn(note_name, "\n")] = 0;

  if(strlen(config->save_path) + strlen(note_name) < sizeof(config->save_path)) {
    size_t remaining_space = sizeof(config->save_path) - strlen(config->save_path) - 1;

    char temp_path[MAX_PATH_LENGTH];

    // Ensure temp_path is empty
    temp_path[0] = '\0';

    // Concatenate "/" and note_name into temp_path
    strncat(temp_path, "/", sizeof(temp_path) - 1);
    strncat(temp_path, note_name, sizeof(temp_path) - 1);

    // make save path
    strncat(config->save_path, note_name, remaining_space);

    // Duplicate the string for the caller
    char *result = strdup(config->save_path);

    if(config->add_dates) {
      char *dated_save_path = add_date_to_save_path(result);

      // Clean up the original result
      free(result);

      // Return the save path with the date
      return dated_save_path;
      
    } else {
      // Return the save path
      return result;
    }
  } else {
    fprintf(stderr, "Error, file name too long.");
    exit(EXIT_FAILURE);
  }
}



void write_to_file(const char *file_path, const char *content) {
  FILE *file_check = fopen(file_path, "r");

  if (file_check) {
    char response[10];
    printf("The file already exists. Do you want to overwrite it? (y/n): ");
    scanf("%s", response);

    if (strcasecmp(response, "y") == 0 || strcasecmp(response, "yes") == 0) {
      printf("writing file...\n");
      FILE *file = fopen(file_path, "w");

      if (file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
      }

      fprintf(file, "%s", content);

      fclose(file_check);
      fclose(file);

      printf("%s Saved!\n", file_path);
    } else {
      printf("Operation aborted. Exiting.\n");
      exit(EXIT_SUCCESS);
    }
  } else {
    printf("writing file...\n");
    FILE *file = fopen(file_path, "w");

    if (file == NULL) {
      perror("Error creating file");
      exit(EXIT_FAILURE);
    }

    fprintf(file, "%s", content);
    fclose(file);
    printf("%s Saved!\n", file_path);
  }
}

int main() {
  AppConfig config;

  const char *config_file = "nmrc";

  // --------- Set config ---------
  read_config(config_file, &config);

  // --------- Get note name ---------
  char *save_name = get_save_name(&config);

  // -------- Get note content ---------
  char *note_content = get_dynamic_string(D_BUFFER);

  // -------- Save file -----------
  write_to_file(save_name, note_content);

  // Clean up
  free(save_name);
  free(note_content);

  return 0;
}
