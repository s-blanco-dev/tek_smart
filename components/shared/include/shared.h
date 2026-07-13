#ifndef SHARED
#define SHARED

typedef struct command_s {
  char* id;
  void (*callback)(void);
} command_t;

#endif // !SHARED
