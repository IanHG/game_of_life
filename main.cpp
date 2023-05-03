#include <iostream>

#include <time.h>
#include <getopt.h>
#include <signal.h>

typedef enum draw_type_e 
{  ASCII
,  BRAILLE
,  BIGPIXEL
,  SIXEL 
} draw_type_t;

// Global variables
int rows = 10;
int cols = 10;
int verbose = 0;
draw_type_t draw_type = ASCII;
int random_fill = 0;
float fps   = 10;
float random_fraction = 0.2;
int max_steps = 0;
int keep_running = 1;

void (*draw_init) () = NULL;
void (*draw_final)() = NULL;
void (*draw_frame)(const int* board, int rows, int cols) = NULL;

/**
 * Setup signal handlers
 **/
void
sigint_handler
   (  int         signal
   ,  siginfo_t*  si
   ,  void*       ptr
   )
{
   keep_running = 0;
}

int 
signal_setup_handlers
   (
   )
{
   struct sigaction sa_sigint{ 0, };
   sa_sigint.sa_sigaction = sigint_handler;
   sa_sigint.sa_flags     = SA_SIGINFO;
   sigaction(SIGINT, &sa_sigint, NULL);
}

/**
 * Print
 **/
void print_matrix(int* mat, int rows, int cols)
{
   for(int i = 0; i < rows; ++i)
   {
      for(int j = 0; j < cols; ++j)
      {
         std::cout << mat[i * cols + j] << " ";
      }
      std::cout << std::endl;
   }
}

/**
 * Fill board
 **/
void init_board_random
   (  int*  board
   ,  int   rows
   ,  int   cols
   )
{
   int size = rows * cols;

   for(int i = 0; i < size; ++i)
   {
      float r = float(rand()) / float(RAND_MAX);
      board[i] = r <= random_fraction;
   }
}

/**
 *
 **/
void init_board_file
   (  int*  board
   ,  int   rows
   ,  int   cols
   )
{
   FILE* file = fopen("games/se-glider.txt", "r");
   int frows;
   int fcols;

   if(fscanf(file, "%i %i\n", &frows, &fcols) == 2)
   {
      int  fsize = frows * fcols;
      int  buffer[fsize];
      char cbuffer[fsize];
      
      fgets(cbuffer, fsize, file);
      
      for(int i = 0; i < fsize; ++i)
      {
         std::cout << cbuffer[i] << std::endl;
      }
   }

   fclose(file);
}

/**
 * Update board
 **/
void update_board
   (  int*  board
   ,  int*  sum
   ,  int   rows
   ,  int   cols
   )
{
   int size = rows * cols;

   int* curr = board;
   int* next = board + cols;
   int* prev = nullptr;

   int* curr_sum = sum;

   for(int i = 1; i < rows - 1; ++i)
   {
      prev = curr;
      curr = next;
      next = next + cols;

      curr_sum = curr_sum + cols;

      for (int j = 1; j < cols - 1; ++j)
      {
         curr_sum[j] += prev[j - 1] + prev[j] + prev[j + 1]
                     +  next[j - 1] + next[j] + next[j + 1]
                     +  curr[j - 1] + curr[j + 1];
      }
   }

   for(int i = 0; i < size; ++i)
   {
      board[i] = ((sum[i] == 2 && board[i]) || sum[i] == 3) ? 1 : 0;
      sum  [i] = 0;
   }
}

char output_buffer[2048 * 1024];

/**
 * Draw ASCII
 **/
void 
draw_frame_ascii
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	/* fill output buffer */
	char *buf            = output_buffer;
   const int *board_buf = board;
   
   // Set buffer to write from 1,1
   *buf++ = '\033'; *buf++ = '[';
   *buf++ = '1';    *buf++ = ';';
   *buf++ = '1';    *buf++ = 'H';

	for (int row = 0; row < rows; ++row) 
   {
		for (int col = 0; col < cols; ++col) 
      {
         *buf++ = *board_buf * '0' + !*board_buf * ' ';
         
         /* Increment pointers */
         ++board_buf;
		}

		*buf++ = '\n';
	}
	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	fputs(output_buffer, stdout);
}

/**
 * Draw Braille
 **/
void 
draw_frame_braille
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	/* fill output buffer */
	char *buf            = output_buffer;
   const int *board_buf0 = board;
   const int *board_buf1 = board_buf0 + cols;
   const int *board_buf2 = board_buf1 + cols;
   const int *board_buf3 = board_buf2 + cols;

   
   // Set buffer to write from 1,1
   *buf++ = '\033'; *buf++ = '[';
   *buf++ = '1';    *buf++ = ';';
   *buf++ = '1';    *buf++ = 'H';

	for (int row = 0; row < rows; row += 4) 
   {
		for (int col = 0; col < cols; col += 2) 
      {
         /** 
          * Dot positions in a cell
          *
          *    1 4
          *    2 5
          *    3 6
          *    7 8
          *
          * Described by two hex characters:
          *
          *         {8 7 6 5} {4 3 2 1}
          * Binary:  b b b b   b b b b
          * Hex   :     Y         X          
          *
          * U+2800 + YX
          *
          * See `man utf-8`. U+28YX is in the "0x00000800 - 0x0000FFFF" range, which means it is represented by:
          *
          *    1110xxxx 10xxxxxx 10xxxxxx
          *
          *
          * See also: 
          *    https://en.wikipedia.org/wiki/Braille_Patterns
          *    https://stackoverflow.com/questions/6240055/manually-converting-unicode-codepoints-into-utf-8-and-utf-16
          **/

         unsigned char c0 = 0b10100000;
         unsigned char c1 = 0b10000000;

         const unsigned char d1 = 0b00000001;
         const unsigned char d2 = 0b00000010;
         const unsigned char d3 = 0b00000100;
         const unsigned char d4 = 0b00001000;
         const unsigned char d5 = 0b00010000;
         const unsigned char d6 = 0b00100000;

         const unsigned char d7 = 0b00000001;
         const unsigned char d8 = 0b00000010;

         c0 |= board_buf3[0] * d7 | board_buf3[1] * d8;
         c1 |= board_buf0[0] * d1 | board_buf0[1] * d4
            |  board_buf1[0] * d2 | board_buf1[1] * d5
            |  board_buf2[0] * d3 | board_buf2[1] * d6;
         
         const unsigned char braille_char[] = { 0xe2, c0, c1 };
			*buf++ = braille_char[0]; 
			*buf++ = braille_char[1]; 
			*buf++ = braille_char[2]; 
         
         /* Increment pointers */
         board_buf0 += 2;
         board_buf1 += 2;
         board_buf2 += 2;
         board_buf3 += 2;
		}

      board_buf0 += 3 * cols;
      board_buf1 += 3 * cols;
      board_buf2 += 3 * cols;
      board_buf3 += 3 * cols;

		*buf++ = '\n';
	}
	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	fputs(output_buffer, stdout);
}

/**
 * Draw bigpixel
 **/
#define BYTE_TO_TEXT(buf_, byte_) do {\
	*(buf_)++ = '0' + (byte_) / 100u;\
	*(buf_)++ = '0' + (byte_) / 10u % 10u;\
	*(buf_)++ = '0' + (byte_) % 10u;\
} while (0)

void 
draw_frame_bigpixel
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	/* fill output buffer */
	char *buf            = output_buffer;

	uint32_t color_fg = 0xFFFFFF00;
	uint32_t color_bg = 0xFFFFFF00;

   const int *board_buf0 = board;
   const int *board_buf1 = board_buf0 + cols;
   
   // Set buffer to write from 1,1
   *buf++ = '\033'; *buf++ = '[';
   *buf++ = '1';    *buf++ = ';';
   *buf++ = '1';    *buf++ = 'H';

	for (int row = 0; row < rows; row+=2) 
   {
		for (int col = 0; col < rows; col++) 
      {
         /* Handle foreground */
         uint32_t pixel_fg = (*board_buf1) * 0x000000FF;
			if ((color_fg ^ pixel_fg) & 0x00FFFFFF) 
         {
				*buf++ = '\033'; *buf++ = '[';
				*buf++ = '3'; *buf++ = '8'; /* Set foreground color */
				*buf++ = ';'; *buf++ = '2';
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_fg);
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_fg);
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_fg);
				*buf++ = 'm';
				color_fg = pixel_fg;
			}
         /* Handle background */
         uint32_t pixel_bg = (*board_buf0) * 0x000000FF;
			if ((color_bg ^ pixel_bg) & 0x00FFFFFF) 
         {
				*buf++ = '\033'; *buf++ = '[';
				*buf++ = '4'; *buf++ = '8'; /* Set background color */
				*buf++ = ';'; *buf++ = '2';
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_bg);
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_bg);
				*buf++ = ';'; BYTE_TO_TEXT(buf, (uint8_t)pixel_bg);
				*buf++ = 'm';
				color_bg = pixel_bg;
			}
         /* Write U+2584 (solid block in lower half of cell) */
         char two_pixel_pr_char[] = { (char)0xe2, (char)0x96, (char)0x84 };
			*buf++ = two_pixel_pr_char[0]; 
			*buf++ = two_pixel_pr_char[1]; 
			*buf++ = two_pixel_pr_char[2]; 
         
         /* Increment pointers */
			board_buf0++;
			board_buf1++;
		}

		*buf++ = '\n';

	   board_buf0 += cols;
	   board_buf1 += cols;
	}
   
   /* Reset char (not really needed, but also doesn't cost that much) and NULL terminate */
	*buf++ = '\033'; *buf++ = '[';
	*buf++ = '0';
	*buf++ = 'm';

	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	fputs(output_buffer, stdout);
}

void
draw_final_bigpixel
   (
   )
{
	/* fill output buffer */
	char *buf            = output_buffer;

   /* Reset char (not really needed, but also doesn't cost that much) and NULL terminate */
	*buf++ = '\033'; *buf++ = '[';
	*buf++ = '0';
	*buf++ = 'm';

	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	fputs(output_buffer, stdout);
}

#undef BYTE_TO_TEXT

/**
 * Sixel
 **/
void 
draw_init_sixel
   (
   )
{  
   //fputs("\033Pq", stdout); /* start sixel output */
   //fputs("#0;2;100;100;100", stdout); /* define white in color register 0 */
}

void 
draw_final_sixel
   (
   )
{
   //fputs("\033\\", stdout); /* return to normal output */
}

void draw_frame_sixel
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	char *buf            = output_buffer;
   
   /* sixel is 6 pixels stacked on top of each-other */
   const int *board_buf0 = board;
   const int *board_buf1 = board_buf0 + cols;
   const int *board_buf2 = board_buf1 + cols;
   const int *board_buf3 = board_buf2 + cols;
   const int *board_buf4 = board_buf3 + cols;
   const int *board_buf5 = board_buf4 + cols;

   /* Set buffer to write from 1,1 */
   *buf++ = '\033'; *buf++ = '[';
   *buf++ = '1';    *buf++ = ';';
   *buf++ = '1';    *buf++ = 'H';
   
   /* Enter sixel mode */
   /* \033P is "Device Control String", and q is sixel */
   /* Format is \033P<p1>;<p2>;<p3>q, where <p1>, <p2>, and <p3> are optional parameters */
   *buf++ = '\033'; *buf++ = 'P'; 
   ///**buf++ = '';*/ *buf++ = ';'; 
   //*buf++ = '0'; *buf++ = ';'; 
   ///* *buf++ = ''; */
   *buf++ = 'q';

   /* Define white in color register 0 */
   *buf++ = '#'; 
   *buf++ = '0'; *buf++ = ';';
   *buf++ = '2'; *buf++ = ';';
   *buf++ = '1'; *buf++ = '0'; *buf++ = '0'; *buf++ = ';';
   *buf++ = '1'; *buf++ = '0'; *buf++ = '0'; *buf++ = ';';
   *buf++ = '1'; *buf++ = '0'; *buf++ = '0'; 
   
   /* Define black in color register 1 */
   *buf++ = '#'; 
   *buf++ = '1'; *buf++ = ';';
   *buf++ = '2'; *buf++ = ';';
   *buf++ = '0'; *buf++ = '0'; *buf++ = '0'; *buf++ = ';';
   *buf++ = '0'; *buf++ = '0'; *buf++ = '0'; *buf++ = ';';
   *buf++ = '0'; *buf++ = '0'; *buf++ = '0'; 

   const unsigned char b0 = 0b00000001;
   const unsigned char b1 = 0b00000010;
   const unsigned char b2 = 0b00000100;
   const unsigned char b3 = 0b00001000;
   const unsigned char b4 = 0b00010000;
   const unsigned char b5 = 0b00100000;

	for (int row = 0; row < rows; row += 6) 
   {
      /* Print white pixels */
      *buf++ = '#'; *buf++ = '0';

		for (int col = 0; col < cols; col += 1) 
      {
         unsigned char c0 = 0b00000000;
         
         c0 |= board_buf0[0] * b0 | board_buf1[0] * b1
            |  board_buf2[0] * b2 | board_buf3[0] * b3
            |  board_buf4[0] * b4 | board_buf5[0] * b5;

         c0 += 0b00111111; /* Add 63 (defined by sixel spec) */

         *buf++ = c0;

         /* Increment pointers */
         board_buf0 += 1;
         board_buf1 += 1;
         board_buf2 += 1;
         board_buf3 += 1;
         board_buf4 += 1;
         board_buf5 += 1;
      }
		
      *buf++ = '$'; /* $ means go to start of line (and overwrite with more sixels) */

      board_buf0 -= cols;
      board_buf1 -= cols;
      board_buf2 -= cols;
      board_buf3 -= cols;
      board_buf4 -= cols;
      board_buf5 -= cols;
      
      /* Print black pixels */
      *buf++ = '#'; *buf++ = '1';

		for (int col = 0; col < cols; col += 1) 
      {
         unsigned char c0 = 0b00000000;

         c0 |= (!board_buf0[0]) * b0 | (!board_buf1[0]) * b1
            |  (!board_buf2[0]) * b2 | (!board_buf3[0]) * b3
            |  (!board_buf4[0]) * b4 | (!board_buf5[0]) * b5;

         c0 += 0b00111111; /* Add 63 (defined by sixel spec) */

         *buf++ = c0;

         /* Increment pointers */
         board_buf0 += 1;
         board_buf1 += 1;
         board_buf2 += 1;
         board_buf3 += 1;
         board_buf4 += 1;
         board_buf5 += 1;
      }

      board_buf0 += 5 * cols;
      board_buf1 += 5 * cols;
      board_buf2 += 5 * cols;
      board_buf3 += 5 * cols;
      board_buf4 += 5 * cols;
      board_buf5 += 5 * cols;

		*buf++ = '-'; /* - means go to next line */
   }
   
   *buf++ = '\033'; *buf++ = '\\'; /* End sixel mode */

	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	fputs(output_buffer, stdout);
}

/**
 * Print usage help message.
 * Will exit the program.
 **/
void print_usage (FILE* stream, int exit_code)
{
  fprintf (stream,
           "  -h  --help         Display this usage information.\n"
           "  -v  --verbose      Print verbose messages.\n"
           "  -r  --rows <rows>  Set number of rows (default: 10).\n"
           "  -c  --cols <cols>  Set number of cols (default: 10).\n"
           "  --fps <fps>        The number of frames to simulate per second.\n"
           "  --braille          Display using Braille characters.\n"
           "  --random           Randomly fill the board.\n"
           "  --fraction <fraction>\n"
           "                     The fraction of cells that are populated when using --random.\n"
           );
  exit (exit_code);
}

/**
 * Parse command line options.
 * Called on master thread.
 *
 * Based on this example:
 *    https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
 **/
int parse_cmdl_args(int argc, char* argv[])
{
   int c;
   
   while (1)
   {
      /* Define command-line options to be parsed with getopt_long */
      static struct option long_options[] =
      {
         /* These options set a flag. */
         {"verbose", no_argument, &verbose, 1},
         {"random",  no_argument, &random_fill,  1},
         /* These options don’t set a flag.
            We distinguish them by their indices. */
         {"braille",  no_argument,   0, 'b'},
         {"bigpixel",  no_argument,   0, 'i'},
         {"sixel",  no_argument,   0, 's'},
         {"help", no_argument,       0, 'h'},
         {"rows", required_argument, 0, 'r'},
         {"cols", required_argument, 0, 'c'},
         {"fps", required_argument,  0, 'f'},
         {"fraction", required_argument,  0, 'a'},
         {"maxsteps", required_argument,  0, 'm'},
         {0, 0, 0, 0}
      };

      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "hs:",
            long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
         break;

      switch (c)
      {
         case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
               break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
               printf (" with arg %s", optarg);
            printf ("\n");
            break;

         case 'h':
            print_usage(stdout, 0);
            break;

         case 'b':
            draw_type = BRAILLE;
            break;

         case 'i':
            draw_type = BIGPIXEL;
            break;
         
         case 's':
            draw_type = SIXEL;
            break;

         case 'r':
            rows = strtol(optarg, NULL, 10);
            break;
         
         case 'c':
            cols = strtol(optarg, NULL, 10);
            break;

         case 'f':
            fps = strtof(optarg, NULL);
            break;

         case 'a':
            random_fraction = strtof(optarg, NULL);
            break;

         case 'm':
            max_steps = strtol(optarg, NULL, 10);
            break;

         case '?':
            /* getopt_long already printed an error message. */
            break;

         default:
            abort ();
      }
   }

   /* Instead of reporting ‘--verbose’
      and ‘--brief’ as they are encountered,
      we report the final status resulting from them. */
   if (verbose)
      printf("verbose flag is set");

   /* Print any remaining command line arguments (not options). */
   if (optind < argc)
   {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
         printf ("%s ", argv[optind++]);
      putchar ('\n');
   }
   
   return 0;
}

#define CLOCK_TYPE CLOCK_MONOTONIC_RAW

/**
 * Main
 **/
int main(int argc, char* argv[])
{
   // Read command line
   parse_cmdl_args(argc, argv);
   srand(time(0));

   int buffer_size;
   
   // Init draw mechanism
   switch(draw_type)
   {
      case ASCII:
         draw_frame  = draw_frame_ascii;
         buffer_size = cols * rows;
         break;
      case BRAILLE:
         draw_frame  = draw_frame_braille;
         buffer_size = (cols + cols % 2) * (rows + rows % 4);
         break;
      case BIGPIXEL:
         draw_final  = draw_final_bigpixel;
         draw_frame  = draw_frame_bigpixel;
         buffer_size = cols * (rows + rows % 2);
         break;
      case SIXEL:
         draw_init  = draw_init_sixel;
         draw_final = draw_final_sixel;
         draw_frame = draw_frame_sixel;
         buffer_size = cols * (rows + rows % 6);
         break;
   }

   // Init board
   int board[buffer_size] = {0};
   int sum  [buffer_size] = {0};

   if(random_fill)
   {
      init_board_random(board, rows, cols);
   }
   else
   {
      //init_board_file(board, rows, cols);

      board[2 * cols + 3] = 1;
      board[3 * cols + 4] = 1;
      board[4 * cols + 2] = 1;
      board[4 * cols + 3] = 1;
      board[4 * cols + 4] = 1;
   }

   if(draw_init)
      draw_init();

   draw_frame(board, rows, cols);

   // Run the game
   timespec clock_start_frame;
   timespec clock_sleep_frame;
   timespec clock_desired_frame { .tv_sec = 0, .tv_nsec = 1000000000.0f / fps };
   
   int nsteps    = 0;
   while 
      (  keep_running
      && (  max_steps == 0  
         || nsteps != max_steps
         )
      )
   {
      clock_gettime(CLOCK_TYPE, &clock_start_frame);
      
      update_board(board, sum, rows, cols);
      draw_frame  (board, rows, cols);

      clock_gettime(CLOCK_TYPE, &clock_sleep_frame);
      clock_sleep_frame.tv_sec  = clock_sleep_frame.tv_sec  - clock_start_frame.tv_sec;
      clock_sleep_frame.tv_nsec = clock_sleep_frame.tv_nsec - clock_start_frame.tv_nsec;

      if (  clock_desired_frame.tv_sec  > clock_sleep_frame.tv_sec
         || clock_desired_frame.tv_nsec > clock_sleep_frame.tv_nsec
         )
      {
         clock_sleep_frame.tv_sec  = clock_desired_frame.tv_sec  - clock_sleep_frame.tv_sec;
         clock_sleep_frame.tv_nsec = clock_desired_frame.tv_nsec - clock_sleep_frame.tv_nsec;
         
         nanosleep(&clock_sleep_frame, NULL);
      }

      ++nsteps;
   }

   if(draw_final)
      draw_final();
}
