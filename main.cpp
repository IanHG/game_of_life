#include <iostream>

#include <time.h>
#include <getopt.h>

// Global variables
int rows = 10;
int cols = 10;
int verbose = 0;
int braille = 0;
float fps   = 10;

void (*draw_frame)(const int* board, int rows, int cols) = NULL;

// Some input
int random_fill = 0;

/// SET COLORS
//         /* Handle foreground */
//			if ((color_fg ^ *(uint32_t*)pixel_fg) & 0x00FFFFFF) {
//				*buf++ = '\033'; *buf++ = '[';
//				*buf++ = '3'; *buf++ = '8'; /* Set foreground color */
//				*buf++ = ';'; *buf++ = '2';
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->r);
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->g);
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_fg->b);
//				*buf++ = 'm';
//				color_fg = *(uint32_t*)pixel_fg;
//			}
//         /* Handle background */
//			if ((color_bg ^ *(uint32_t*)pixel_bg) & 0x00FFFFFF) {
//				*buf++ = '\033'; *buf++ = '[';
//				*buf++ = '4'; *buf++ = '8'; /* Set background color */
//				*buf++ = ';'; *buf++ = '2';
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->r);
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->g);
//				*buf++ = ';'; BYTE_TO_TEXT(buf, pixel_bg->b);
//				*buf++ = 'm';
//				color_bg = *(uint32_t*)pixel_bg;
//			}

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
      board[i] = r < 0.5;
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

//struct fps_type
//{
//   float fps        = 0.0f;
//   float fps_max    = 10.0f;
//   float frame_time = 0.0f;
//};
//
//void calculate_fps
//   ( fps_type* fps
//   )
//{
//   static const int num_samples = 10;
//   static float frame_times[num_samples] = {0.0f};
//   static int   current_frame = 0;
//   static float previous_ticks = clock();
//
//   float current_ticks = clock();
//
//   fps->frame_time = current_ticks - previous_ticks;
//   frame_times[current_frame] = fps->frame_time;
//
//   float frame_time_average = 0.0f;
//   for(int i = 0; i < num_samples; ++i)
//   {
//      frame_time_average += frame_times[i];
//   }
//   frame_time_average /= num_samples;
//
//   if(frame_time_average)
//   {
//      fps->fps = 1000.0f / frame_time_average;
//   }
//   
//   previous_ticks = current_ticks;
//   current_frame = (current_frame + 1) % num_samples;
//}

char output_buffer[2048 * 1024];

void draw_frame_basic
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	/* fill output buffer */
	//uint32_t color_fg     = 0xFFFFFF00;
	//uint32_t color_bg     = 0xFFFFFF00;
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
   /* Reset char (not really needed, but also doesn't cost that much) and NULL terminate */
	//*buf++ = '\033'; *buf++ = '[';
	//*buf++ = '0';
	//*buf++ = 'm';
	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	//CALL_STDOUT(fputs(output_buffer, stdout), "DG_DrawFrame: fputs error %d");
	fputs(output_buffer, stdout);

	/* clear output buffer */
	//memset(output_buffer, '\0', buf - output_buffer + 1u);
}

void draw_frame_braille
   (  const int* const        board
   ,  int                     rows
   ,  int                     cols
   )
{
	/* fill output buffer */
	//uint32_t color_fg     = 0xFFFFFF00;
	//uint32_t color_bg     = 0xFFFFFF00;
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
          * See also: https://en.wikipedia.org/wiki/Braille_Patterns
          *           https://stackoverflow.com/questions/6240055/manually-converting-unicode-codepoints-into-utf-8-and-utf-16
          *
          * 1110xxxx 10xxxxxx 10xxxxxx
          *
          **/

         unsigned char c0 = 0b10100000;
         unsigned char c1 = 0b10000000;

         unsigned char d1 = 0b00000001;
         unsigned char d2 = 0b00000010;
         unsigned char d3 = 0b00000100;
         unsigned char d4 = 0b00001000;
         unsigned char d5 = 0b00010000;
         unsigned char d6 = 0b00100000;

         unsigned char d7 = 0b00000001;
         unsigned char d8 = 0b00000010;

         c0 |= board_buf3[0] * d7 | board_buf3[1] * d8;
         c1 |= board_buf0[0] * d1 | board_buf0[1] * d4
            |  board_buf1[0] * d2 | board_buf1[1] * d5
            |  board_buf2[0] * d3 | board_buf2[1] * d6;
         
         unsigned char braille_char[] = { 0xe2, c0, c1 };
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
   /* Reset char (not really needed, but also doesn't cost that much) and NULL terminate */
	//*buf++ = '\033'; *buf++ = '[';
	//*buf++ = '0';
	//*buf++ = 'm';
	*buf = '\0'; /* NULL termination */

	/* flush output buffer */
	//CALL_STDOUT(fputs(output_buffer, stdout), "DG_DrawFrame: fputs error %d");
	fputs(output_buffer, stdout);

	/* clear output buffer */
	//memset(output_buffer, '\0', buf - output_buffer + 1u);
}

/**
 * Print usage help message.
 * Will exit the program.
 **/
void print_usage (FILE* stream, int exit_code)
{
  //fprintf (stream, "Usage:  %s [options ...]\n", global.program);
  //fprintf (stream, "\n"
  //                 "  Sending USR1 signal to a running %s, will cause a safe shutdown.\n"
  //                 "\n"
  //               , global.program 
  //        );
  fprintf (stream,
           "  -h  --help         Display this usage information.\n"
           "  -v  --verbose      Print verbose messages.\n"
           "  -r  --rows <rows>  Set number of rows (default: 10).\n"
           "  -c  --cols <cols>  Set number of cols (default: 10).\n"
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
         {"braille",  no_argument, &braille,  1},
         /* These options don’t set a flag.
            We distinguish them by their indices. */
         {"help", no_argument,       0, 'h'},
         {"rows", required_argument, 0, 'r'},
         {"cols", required_argument, 0, 'c'},
         {"fps", required_argument,  0, 'f'},
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

         case 'r':
            rows = strtol(optarg, NULL, 10);
            break;
         
         case 'c':
            cols = strtol(optarg, NULL, 10);
            break;

         case 'f':
            fps = strtof(optarg, NULL);
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
   parse_cmdl_args(argc, argv);
   srand(time(0));

   int size = cols * rows;
   
   // Init board
   int board[size] = {0};
   int sum  [size] = {0};

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

   if(braille)
   {
      draw_frame = draw_frame_braille;
   }  
   else
   {
      draw_frame = draw_frame_basic;
   }

   draw_frame(board, rows, cols);

   // Run the game
   //fps_type fps;
   //
   
   
   timespec clock_start_frame;
   timespec clock_sleep_frame;
   timespec clock_desired_frame { .tv_sec = 0, .tv_nsec = 1000000000.0f / fps };
  
   while(true)
   {
      clock_gettime(CLOCK_TYPE, &clock_start_frame);

      update_board(board, sum, rows, cols);
      //calculate_fps(&fps);
      draw_frame (board, rows, cols);
      //break;


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
   }
}
