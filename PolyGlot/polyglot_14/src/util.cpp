
// util.cpp

// includes

extern void polyglot_quit();

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "main.h"
#include "posix.h"
#include "util.h"

// variables

static bool Error;

static FILE * LogFile;

// functions

// util_init()

void util_init() {

   Error = false;

   // init log file

   LogFile = NULL;

   // switch file buffering off

   setbuf(stdin,NULL);
   setbuf(stdout,NULL);
}

// my_random_init()

void my_random_init() {

   srand(time(NULL));
}

// my_random_int()

int my_random_int(int n) {

   int r;

   ASSERT(n>0);

   r = int(floor(my_random_double()*double(n)));
   ASSERT(r>=0&&r<n);

   return r;
}

// my_random_double()

double my_random_double() {

   double r;

   r = double(rand()) / (double(RAND_MAX) + 1.0);
   ASSERT(r>=0.0&&r<1.0);

   return r;
}

// my_atoll()

sint64 my_atoll(const char string[]) {

   sint64 n;

   sscanf(string,S64_FORMAT,&n);

   return n;
}

// my_round()

int my_round(double x) {

   return int(floor(x+0.5));
}

// my_malloc()

void * my_malloc(int size) {

   void * address;

   ASSERT(size>0);

   address = malloc(size);
   if (address == NULL) my_fatal("my_malloc(): malloc(): %s\n",strerror(errno));

   return address;
}

// my_realloc()

void * my_realloc(void * address, int size) {

   ASSERT(address!=NULL);
   ASSERT(size>0);

   address = realloc(address,size);
   if (address == NULL) my_fatal("my_realloc(): realloc(): %s\n",strerror(errno));

   return address;
}

// my_free()

void my_free(void * address) {

   ASSERT(address!=NULL);

   free(address);
}

// my_log_open()

void my_log_open(const char file_name[]) {

   ASSERT(file_name!=NULL);

   LogFile = fopen(file_name,"a");

   if (LogFile != NULL) setvbuf(LogFile,NULL,_IOLBF,0); // line buffering
}

// my_log_close()

void my_log_close() {

   if (LogFile != NULL) fclose(LogFile);
}

// my_log()

void my_log(const char format[], ...) {

   va_list ap;

   ASSERT(format!=NULL);

   if (LogFile != NULL) {
      va_start(ap,format);
      vfprintf(LogFile,format,ap);
      va_end(ap);
   }
}

// my_fatal()

void my_fatal(const char format[], ...) {

   va_list ap;

   ASSERT(format!=NULL);

   va_start(ap,format);

   vfprintf(stderr,format,ap);
   if (LogFile != NULL) vfprintf(LogFile,format,ap);

   va_end(ap);

   if (Error) { // recursive error
      my_log("POLYGLOT *** RECURSIVE ERROR ***\n");
      exit(EXIT_FAILURE);
      // abort();
   } else {
      Error = true;
      polyglot_quit();
   }
}

// my_file_read_line()

bool my_file_read_line(FILE * file, char string[], int size) {

   int src, dst;
   int c;

   ASSERT(file!=NULL);
   ASSERT(string!=NULL);
   ASSERT(size>0);

   if (fgets(string,size,file) == NULL) {
      if (feof(file)) {
         return false;
      } else { // error
         my_fatal("my_file_read_line(): fgets(): %s\n",strerror(errno));
      }
   }

   // remove CRs and LFs

   src = 0;
   dst = 0;
   
   while ((c=string[src++]) != '\0') {
      if (c != '\r' && c != '\n') string[dst++] = c;
   }

   string[dst] = '\0';

   return true;
}

// my_string_empty()

bool my_string_empty(const char string[]) {

   return string == NULL || string[0] == '\0';
}

// my_string_equal()

bool my_string_equal(const char string_1[], const char string_2[]) {

   ASSERT(string_1!=NULL);
   ASSERT(string_2!=NULL);

   return strcmp(string_1,string_2) == 0;
}

// my_string_case_equal()

bool my_string_case_equal(const char string_1[], const char string_2[]) {

   int c1, c2;

   ASSERT(string_1!=NULL);
   ASSERT(string_2!=NULL);

   while (true) {

      c1 = *string_1++;
      c2 = *string_2++;

      if (tolower(c1) != tolower(c2)) return false;
      if (c1 == '\0') return true;
   }

   return false;
}

// my_strdup()

char * my_strdup(const char string[]) {

   char * address;

   ASSERT(string!=NULL);

   // strdup() is not ANSI C

   address = (char *) my_malloc(strlen(string)+1);
   strcpy(address,string);

   return address;
}

// my_string_clear()

void my_string_clear(const char * * variable) {

   ASSERT(variable!=NULL);

   if (*variable != NULL) {
      my_free((void*)(*variable));
      *variable = NULL;
   }
}

// my_string_set()

void my_string_set(const char * * variable, const char string[]) {

   ASSERT(variable!=NULL);
   ASSERT(string!=NULL);

   if (*variable != NULL) my_free((void*)(*variable));
   *variable = my_strdup(string);
}

// my_timer_reset()

void my_timer_reset(my_timer_t * timer) {

   ASSERT(timer!=NULL);

   timer->start_real = 0.0;
   timer->start_cpu = 0.0;
   timer->elapsed_real = 0.0;
   timer->elapsed_cpu = 0.0;
   timer->running = false;
}

// my_timer_start()

void my_timer_start(my_timer_t * timer) {

   ASSERT(timer!=NULL);

   ASSERT(timer->start_real==0.0);
   ASSERT(timer->start_cpu==0.0);
   ASSERT(!timer->running);

   timer->running = true;
   timer->start_real = now_real();
   timer->start_cpu = now_cpu();
}

// my_timer_stop()

void my_timer_stop(my_timer_t * timer) {

   ASSERT(timer!=NULL);

   ASSERT(timer->running);

   timer->elapsed_real += now_real() - timer->start_real;
   timer->elapsed_cpu += now_cpu() - timer->start_cpu;
   timer->start_real = 0.0;
   timer->start_cpu = 0.0;
   timer->running = false;
}

// my_timer_elapsed_real()

double my_timer_elapsed_real(const my_timer_t * timer) {

   double elapsed;

   ASSERT(timer!=NULL);

   elapsed = timer->elapsed_real;
   if (timer->running) elapsed += now_real() - timer->start_real;

   if (elapsed < 0.0) elapsed = 0.0;

   return elapsed;
}

// my_timer_elapsed_cpu()

double my_timer_elapsed_cpu(const my_timer_t * timer) {

   double elapsed;

   ASSERT(timer!=NULL);

   elapsed = timer->elapsed_cpu;
   if (timer->running) elapsed += now_cpu() - timer->start_cpu;

   if (elapsed < 0.0) elapsed = 0.0;

   return elapsed;
}

// my_timer_cpu_usage()

double my_timer_cpu_usage(const my_timer_t * timer) {

   double real, cpu;
   double usage;

   ASSERT(timer!=NULL);

   real = my_timer_elapsed_real(timer);
   cpu = my_timer_elapsed_cpu(timer);

   if (real <= 0.0 || cpu <= 0.0) return 0.0;

   usage = cpu / real;
   if (usage >= 1.0) usage = 1.0;

   return usage;
}

// end of util.cpp

