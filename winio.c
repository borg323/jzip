/*
 * winio.c
 *
 * Screen I/O routines for Windows.
 *
 * Based on bccio.c (c) 2000 John Holder
 *
 * Including public domain code from conio21
 * (https://sourceforge.net/projects/conio/) written by:
 * Hongli Lai <hongli@telekabel.nl>
 * tkorrovi <tkorrovi@altavista.net> on 2002/02/26.
 * Andrew Westcott <ajwestco@users.sourceforge.net>
 * Michal Molhanec <michal@molhanec.net>
 *
 */

#define UNICODE

#include <windows.h>
#include <conio.h>

#include "ztypes.h"

#include <time.h>
#include <sys/types.h>

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define WHITE 7

#define BRIGHT 0x08
static unsigned short emphasis = BRIGHT;

static DWORD prevmode;
static unsigned short prevattr;
static int win_cursor_on = 1;
static int screen_started = FALSE;
static int cursor_saved = OFF;
static int saved_row = 0;
static int saved_col = 0;
static ZINT16 current_fg;
static ZINT16 current_bg;
extern ZINT16 default_fg;
extern ZINT16 default_bg;

int timed_read_key( int );
int read_key( void );

int BUFFER_SIZE;
char *commands;
static int space_avail;
static int ptr1, ptr2 = 0;
static int end_ptr = 0;
static int row, head_col;

static void get_prev_command( void );
static void get_next_command( void );
static void get_first_command( void );
static void delete_command( void );
static void add_command( char *, int size );
static int display_command( char * );

void initialize_screen( void )
{
   CONSOLE_SCREEN_BUFFER_INFO info;
   int screenheight, screenwidth;

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
   screenwidth  = info.srWindow.Right - info.srWindow.Left + 1;
   screenheight = info.srWindow.Bottom - info.srWindow.Top + 1;

   prevattr = info.wAttributes;

#if 0
   gettextinfo( &ti );
   prevmode = ti.currmode;

   if ( bigscreen )
   {
      textmode( C4350 );
   }
   else
   {
      textmode( C80 );
   }
#endif

   GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &prevmode);
   if ( SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | 0x0010 /*ENABLE_LVB_GRID_WORLDWIDE*/) )
   {
      emphasis = COMMON_LVB_UNDERSCORE;
   }
   else
   {
      SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT);
   }

   if ( screen_rows == 0 )
      screen_rows = screenheight;
   if ( screen_cols == 0 )
      screen_cols = screenwidth;

   if ( monochrome )
      default_bg = 0;           /* black */

   set_colours( 1, 1 );         /* set default colours */
   set_attribute( NORMAL );
   clear_screen(  );
   move_cursor( screen_rows / 2 - 1, ( screen_cols - sizeof ( JZIPVER ) ) / 2 );
   _cputs( JZIPVER );
   move_cursor( screen_rows / 2, ( screen_cols - sizeof ( "The story is loading..." ) ) / 2 );
   _cputs( "The story is loading..." );

   /* set up the history buffer to be the right size */
   commands = ( char * ) malloc( hist_buf_size * sizeof ( char ) );

   if ( commands == NULL )
   {
      /* try again, with smaller buffer if failure */
      if ( hist_buf_size > 1024 )
      {
         commands = ( char * ) malloc( 1024 * sizeof ( char ) );

         if ( commands == NULL )
         {
            fatal( "initialize_screen(): Could not allocate history buffer." );
         }
         else
         {
            hist_buf_size = 1024;
         }
      }
      else
      {
         fatal( "initialize_screen(): Could not allocate history buffer." );
      }
   }

   BUFFER_SIZE = hist_buf_size;
   space_avail = hist_buf_size - 1;

   screen_started = TRUE;

   h_interpreter = INTERP_MSDOS;
   JTERP = INTERP_MSDOS;

}                               /* initialize_screen */

void restart_screen( void )
{
   zbyte_t high = 1, low = 0;
   /* changing the character set is not standard compiant */
   if ( fIBMGraphics )
      high = low = 0;

   cursor_saved = OFF;

   set_byte( H_STANDARD_HIGH, high );
   set_byte( H_STANDARD_LOW, low );
   if ( h_type < V4 )
   {
      set_byte( H_CONFIG, ( get_byte( H_CONFIG ) | CONFIG_WINDOWS ) );
   }
   else
   {
      /* turn stuff on */
      set_byte( H_CONFIG,
                ( get_byte( H_CONFIG ) | CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED |
                  CONFIG_TIMEDINPUT | CONFIG_COLOUR ) );
      /* turn stuff off */
      set_byte( H_CONFIG, ( get_byte( H_CONFIG ) & ~CONFIG_PICTURES & ~CONFIG_SFX ) );
   }

   /* Force graphics off as we can't do them */
   set_word( H_FLAGS, ( get_word( H_FLAGS ) & ( ~GRAPHICS_FLAG ) ) );

}                               /* restart_screen */

void reset_screen( void )
{
   if ( screen_started == TRUE )
   {
      output_new_line(  );
      output_string( "[Hit any key to exit.]" );
      ( void ) read_key(  );
      output_new_line(  );
//      textmode( prevmode );
      SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), prevmode);
      SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), prevattr);
      current_fg = prevattr & 0xf;
      current_bg = prevattr >> 4;
      clear_screen(  );
   }
   screen_started = FALSE;

}                               /* reset_screen */

void clear_screen( void )
{
   DWORD written;
   int i;
   CONSOLE_SCREEN_BUFFER_INFO info;

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

   for (i = info.srWindow.Top; i < info.srWindow.Bottom + 1; i++) {
      FillConsoleOutputAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
        current_fg + (current_bg << 4), info.srWindow.Right - info.srWindow.Left + 1,
        (COORD) {info.srWindow.Left, i},
        &written);
      FillConsoleOutputCharacter (GetStdHandle(STD_OUTPUT_HANDLE), ' ',
        info.srWindow.Right - info.srWindow.Left + 1,
        (COORD) {info.srWindow.Left, i},
        &written);
   }
   SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), (COORD) {info.srWindow.Left, info.srWindow.Top});

}                               /* clear_screen */

void create_status_window( void )
{

}                               /* create_status_window */

void delete_status_window( void )
{

}                               /* delete_status_window */

void select_status_window( void )
{
   save_cursor_position(  );
   if ( !win_cursor_on )
   {
      CONSOLE_CURSOR_INFO Info;
      Info.bVisible = FALSE;
      SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &Info);
   }

}                               /* select_status_window */

void select_text_window( void )
{
   if ( !win_cursor_on )
   {
      CONSOLE_CURSOR_INFO Info;
      Info.dwSize = 20;
      Info.bVisible = TRUE;
      SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE), &Info);
   }
   restore_cursor_position(  );

}                               /* select_text_window */

void clear_line( void )
{
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

    FillConsoleOutputAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
      current_fg + (current_bg << 4),
      info.srWindow.Right - info.dwCursorPosition.X + 1, info.dwCursorPosition, &written);
    FillConsoleOutputCharacter (GetStdHandle (STD_OUTPUT_HANDLE),
      ' ', info.srWindow.Right - info.dwCursorPosition.X + 1, info.dwCursorPosition, &written);

    SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), info.dwCursorPosition);
}                               /* clear_line */

void clear_text_window( void )
{
   CONSOLE_SCREEN_BUFFER_INFO info;
   int row, col, i;
   int screenheight;

   get_cursor_position( &row, &col );

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
   screenheight = info.srWindow.Bottom - info.srWindow.Top + 1;

   for (i = status_size+1; i <= screenheight; i++)
   {
      move_cursor(i, 1);
      clear_line();
   }

   move_cursor( row, col );
}                               /* clear_text_window */

void clear_status_window( void )
{
   int row, col, i;

   get_cursor_position( &row, &col );

   for (i = 1; i <= status_size; i++)
   {
      move_cursor(i, 1);
      clear_line();
   }

   move_cursor( row, col );
}                               /* clear_status_window */

void move_cursor( int row, int col )
{
  COORD c;
  CONSOLE_SCREEN_BUFFER_INFO info;

  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

  c.X = info.srWindow.Left + col - 1;
  c.Y = info.srWindow.Top  + row - 1;
  SetConsoleCursorPosition (GetStdHandle(STD_OUTPUT_HANDLE), c);
}                               /* move_cursor */

void get_cursor_position( int *row, int *col )
{
   CONSOLE_SCREEN_BUFFER_INFO info;

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

   *row = info.dwCursorPosition.Y - info.srWindow.Top  + 1;;
   *col = info.dwCursorPosition.X - info.srWindow.Left + 1;

}                               /* get_cursor_position */

void save_cursor_position( void )
{

   if ( cursor_saved == OFF )
   {
      get_cursor_position( &saved_row, &saved_col );
      cursor_saved = ON;
   }

}                               /* save_cursor_position */

void restore_cursor_position( void )
{

   if ( cursor_saved == ON )
   {
      move_cursor( saved_row, saved_col );
      cursor_saved = OFF;
   }

}                               /* restore_cursor_position */

void set_attribute( int attribute )
{
   int new_fg, new_bg;
   unsigned short attrib;

   if ( attribute == NORMAL )
   {
      attrib = current_fg + (current_bg << 4);
   }
   else
   {
      CONSOLE_SCREEN_BUFFER_INFO info;

      GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

      attrib = info.wAttributes;
   }

   if ( attribute & REVERSE )
   {
      attrib &= ~0x77;
      attrib |= current_bg + (current_fg << 4);
   }

   if ( attribute & BOLD )
   {
      attrib |= BRIGHT;
   }

   if ( attribute & EMPHASIS )
   {
      attrib |= emphasis;
   }
/*
   if ( attribute & FIXED_FONT )
   {
      new_fg = current_fg;
      new_bg = current_bg;
   }
*/
   SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), attrib );

}                               /* set_attribute */

void display_char( int c )
{
   wchar_t string[2];

   string[0] = c;
   string[1] = '\0';
   _cputws( string );
   if ( c == '\n' )
   {
      display_char( '\r' );
   }
}                               /* display_char */

/*
 * Previous command system
 *
 * Here's how this works:
 *
 * The previous command buffer is BUFFER_SIZE bytes long. After the player
 * presses Enter, the command is added to this buffer, with a trailing '\n'
 * added. The '\n' is used to show where one command ends and another begins.
 *
 * The up arrow key retrieves a previous command. This is done by working
 * backwards through the buffer until a '\n' is found. The down arrow
 * retieves the next command by counting forward. The ptr1 and ptr2
 * values hold the start and end of the currently displayed command.
 *
 *
 * Special Key Summary:
 *
 *  left arrow         - move one character to the left
 *  right arrow        - move one character to the right
 *  ctrl + left arrow  - move one word to the left
 *  ctrl + right arrow - move one word to the right
 *  home               - move to beginning of line
 *  end                - move to end of line
 *  backspace          - delete character to the left of the cursor
 *  delete             - delete character below cursor
 *  cursor up          - get previous command
 *  cursor down        - get next command
 *  page up            - get "oldest" command
 *  page down          - display blank prompt (i.e. clears current line)
 *  esc                - display blank prompt (i.e. clears current line)
 *
 */

int display_command( char *buffer )
{
   int counter, loop;

   move_cursor( row, head_col );
   clear_line(  );

   /* ptr1 = end_ptr when the player has selected beyond any previously
    * * saved command.
    */

   if ( ptr1 == end_ptr )
   {
      return ( 0 );
   }
   else
   {
      /* Put the characters from the save buffer into the variable "buffer".
       * The return value (counter) is the value of *read_size.
       */
      counter = 0;
      for ( loop = ptr1; loop <= ptr2; loop++ )
      {
         buffer[counter] = commands[loop];
         display_char( translate_from_zscii( buffer[counter++]) );
      }
      return ( counter );
   }
}                               /* display_command */

void get_prev_command( void )
{

   /* Checking to see if ptr1 > 0 prevents moving ptr1 and ptr2 into
    * never-never land.
    */

   if ( ptr1 > 0 )
   {

      /* Subtract 2 to jump over any intervening '\n' */

      ptr2 = ptr1 -= 2;

      /* If we've jumped too far, fix it */

      if ( ptr1 < 0 )
         ptr1 = 0;
      if ( ptr2 < 0 )
         ptr2 = 0;

      if ( ptr1 > 0 )
      {
         do
         {                      /* Decrement ptr1 until a '\n' is found */
            ptr1--;
         }
         while ( ( ptr1 >= 0 ) && ( commands[ptr1] != '\n' ) );

         /* Then advance back to the position after the '\n' */
         ptr1++;
      }
   }
}                               /* get_prev_command */

void get_next_command( void )
{

   if ( ptr2 < end_ptr )
   {
      /* Add 2 to advance over any intervening '\n' */

      ptr1 = ptr2 += 2;
      if ( ptr2 >= end_ptr )
      {
         ptr1 = ptr2 = end_ptr;
      }
      else
      {
         do
         {
            ptr2++;
         }
         while ( ( commands[ptr2] != '\n' ) && ( ptr2 <= end_ptr ) );
         ptr2--;
      }
   }
}                               /* get_next_command */

void get_first_command( void )
{

   if ( end_ptr > 1 )
   {
      ptr1 = ptr2 = 0;
      do
      {
         ptr2++;
      }
      while ( commands[ptr2] != '\n' );
      ptr2--;
   }
}                               /* get_first_command */

void delete_command( void )
{

   /* Deletes entire commands from the beginning of the command buffer */

   int loop;

   /* Keep moving the characters in the command buffer one space to the left
    * until a '\n' is found...
    */

   do
   {
      for ( loop = 0; loop < end_ptr; loop++ )
      {
         commands[loop] = commands[loop + 1];
      }
      end_ptr--;
      space_avail++;

   }
   while ( commands[0] != '\n' );

   /* ...then delete the '\n' */

   for ( loop = 0; loop < end_ptr; loop++ )
   {
      commands[loop] = commands[loop + 1];
   }
   end_ptr--;
   space_avail++;
   ptr1 = ptr2 = end_ptr;

}                               /* delete_command */

void add_command( char *buffer, int size )
{
   int loop, counter;

   /* Add the player's last command to the command buffer */

   counter = 0;
   for ( loop = end_ptr; loop < ( end_ptr + size ); loop++ )
   {
      commands[loop] = buffer[counter++];
   }

   /* Add one space for '\n' */

   end_ptr += size + 1;
   ptr1 = ptr2 = end_ptr;
   commands[end_ptr - 1] = '\n';
   space_avail -= size + 1;

}                               /* add_command */

int input_line( int buflen, char *buffer, int timeout, int *read_size, int start_col )
{
   int c, col;
   int init_char_pos, curr_char_pos;
   int loop, tail_col;

   /*
    * init_char_pos : the initial cursor location
    * curr_char_pos : the current character position within the input line
    * head_col: the head of the input line (used for cursor position)
    *  (global variable)
    * tail_col: the end of the input line (used for cursor position)
    */

/*
    if (timeout != 0) {
	ftime (&timenow);
/*	tmptr = gmtime (&timenow.time);
	target_second = (tmptr->tm_sec + (timeout/10));*/

/*	target_second = timenow.time + (timeout/10);
	target_millisecond = timenow.millitm + (timeout*10);
	while (target_millisecond >= 1000)
	{
	   target_millisecond -= 1000;
	   target_second++;
	}

    }
*/
   get_cursor_position( &row, &col );
   head_col = start_col;
   tail_col = start_col + *read_size;

   init_char_pos = curr_char_pos = col - start_col;

   ptr1 = ptr2 = end_ptr;

   for ( ;; )
   {
      /* Read a single keystroke */
      do
      {
         if ( timeout == 0 )
         {
            c = read_key(  );
         }
         else
         {
            c = timed_read_key( timeout );
            if ( c == -1 )
               return ( c );
         }
      }
      while ( c == 0 );

      /****** Previous Command Selection Keys ******/

      if ( c == ( unsigned char ) '\x081' )
      {                         /* Up arrow */
         get_prev_command(  );
         curr_char_pos = *read_size = display_command( buffer );
         tail_col = head_col + *read_size;
      }
      else if ( c == ( unsigned char ) '\x082' )
      {                         /* Down arrow */
         get_next_command(  );
         curr_char_pos = *read_size = display_command( buffer );
         tail_col = head_col + *read_size;
      }
      else if ( c == ( unsigned char ) '\x09a' )
      {                         /* PgUp */
         get_first_command(  );
         curr_char_pos = *read_size = display_command( buffer );
         tail_col = head_col + *read_size;
      }
      else if ( ( c == ( unsigned char ) '\x094' ) || ( c == 27 ) )
      {                         /* PgDn or Esc */
         ptr1 = ptr2 = end_ptr;
         curr_char_pos = *read_size = display_command( buffer );
         tail_col = head_col + *read_size;
      }

      /****** Cursor Editing Keys ******/

      else if ( c == ( unsigned char ) '\x083' )
      {                         /* Left arrow */
         get_cursor_position( &row, &col );

         /* Prevents moving the cursor into the prompt */
         if ( col > head_col )
         {
            move_cursor( row, --col );
            curr_char_pos--;
         }
      }
#if 0
      else if ( c == ( unsigned char ) '\x0aa' )
      {                         /* Ctrl + Left arrow */
         get_cursor_position( &row, &col );
         if ( col > head_col )
         {
            col--;
            curr_char_pos--;
            do
            {
               /* Decrement until a ' ' is found */
               col--;
               curr_char_pos--;
            }
            while ( ( buffer[curr_char_pos] != ' ' ) && ( col >= head_col ) );
            curr_char_pos++;
            move_cursor( row, ++col );
         }
      }
#endif
      else if ( c == ( unsigned char ) '\x084' )
      {                         /* Right arrow */
         get_cursor_position( &row, &col );

         /* Prevents moving the cursor beyond the end of the input line */
         if ( col < tail_col )
         {
            move_cursor( row, ++col );
            curr_char_pos++;
         }
      }
#if 0
      else if ( c == ( unsigned char ) '\x0ba' )
      {                         /* Ctrl + Right arrow */
         get_cursor_position( &row, &col );
         if ( col < tail_col )
         {
            do
            {
               /* Increment until a ' ' is found */
               col++;
               curr_char_pos++;
            }
            while ( ( buffer[curr_char_pos] != ' ' ) && ( col < tail_col ) );

            if ( col == tail_col )
            {
               move_cursor( row, tail_col );
            }
            else
            {
               curr_char_pos++;
               move_cursor( row, ++col );
            }
         }
      }
#endif
      else if ( c == ( unsigned char ) '\x092' )
      {                         /* End */
         move_cursor( row, tail_col );
         curr_char_pos = init_char_pos + *read_size;
      }
      else if ( c == ( unsigned char ) '\x098' )
      {                         /* Home */
         move_cursor( row, head_col );
         curr_char_pos = init_char_pos;
      }

      else if ( c == ( unsigned char ) '\x096' )
      {                         /* Delete */
// ZSCII 0x96 is keypad 5, not delete.
#if 0
         if ( curr_char_pos < *read_size )
         {
            get_cursor_position( &row, &col );

            /* Moves the input line one to the left */
            for ( loop = curr_char_pos; loop < *read_size; loop++ )
            {
               buffer[loop] = buffer[loop + 1];
            }

            /* Decrements the end of the input line and the *read_size value */
            tail_col--;
            ( *read_size )--;

            /* Displays the input line */
            clear_line(  );

            for ( loop = curr_char_pos; loop < *read_size; loop++ )
            {
               display_char( translate_from_zscii( buffer[loop] ) );
            }

            /* Restores the cursor position */
            move_cursor( row, col );
         }
#endif
      }
      else if ( c >= 133 && c <= 144 )
      {                         /* F1 - F12 */
      }
      else if ( c == '\b' )
      {                         /* Backspace */
         get_cursor_position( &row, &col );
         if ( col > head_col )
         {
            move_cursor( row, --col );
            clear_line(  );
            for ( loop = curr_char_pos; loop < *read_size; loop++ )
            {
               buffer[loop - 1] = buffer[loop];
               display_char( translate_from_zscii( buffer[loop - 1] ) );
            }
            curr_char_pos--;
            tail_col--;
            ( *read_size )--;
            move_cursor( row, col );
         }

      }
      else
      {                         /* Normal key action */

         if ( *read_size == ( buflen - 1 ) )
         {                      /* Ring bell if buffer is full */
            putchar( '\a' );
         }
         else
         {                      /* Scroll line if return key pressed */
            if ( c == '\r' || c == '\n' )
            {
               c = '\n';
               move_cursor( row, tail_col );
               scroll_line(  );

               /* Add the current command to the command buffer */

               if ( *read_size > space_avail )
               {
                  do
                  {
                     delete_command(  );
                  }
                  while ( *read_size > space_avail );
               }
               if ( *read_size > 0 )
               {
                  add_command( buffer, *read_size );
               }

               /* Return key if it is a line terminator */

               return ( ( unsigned char ) c );

            }
            else
            {
               get_cursor_position( &row, &col );

               /* Used if the cursor is not at the end of the line */
               if ( col < tail_col )
               {

                  /* Moves the input line one character to the right */

                  for ( loop = *read_size; loop >= curr_char_pos; loop-- )
                  {
                     buffer[loop + 1] = buffer[loop];
                  }

                  /* Puts the character into the space created by the
                   * "for" loop above */

                  buffer[curr_char_pos] = ( char ) c;

                  /* Increment the end of the line values */

                  ( *read_size )++;
                  tail_col++;

                  /* Move the cursor back to its original position */

                  move_cursor( row, col );

                  /* Redisplays the input line from the point of
                   * * insertion */

                  for ( loop = curr_char_pos; loop < *read_size; loop++ )
                  {
                     display_char( translate_from_zscii( buffer[loop] ) );
                  }

                  /* Moves the cursor to the next position */

                  move_cursor( row, ++col );
                  curr_char_pos++;
               }
               else
               {                /* Used if the cursor is at the end of the line */
                  buffer[curr_char_pos++] = ( char ) c;
                  display_char( translate_from_zscii( c ) );
                  ( *read_size )++;
                  tail_col++;
               }
            }
         }
      }
   }
}                               /* input_line */

int input_character( int timeout )
{
   int c;

   if ( timeout == 0 )
   {
      c = read_key(  );
   }
   else
   {
      c = timed_read_key( timeout );
   }
   return ( c );

}                               /* input_character */


int timed_read_key( int timeout )
{
   int c;
   register clock_t curr_tick, target_tick;

   /* do math BEFORE calling clock for stability */
   target_tick = ( int ) ( timeout * CLK_TCK ) / 10;
   target_tick += clock(  );

   for ( ;; )
   {
      do
      {
         curr_tick = clock(  );
      }
      while ( ( curr_tick < target_tick ) && !kbhit(  ) );

      if ( !kbhit(  ) )
      {
         return ( -1 );
      }
      else
      {
         c = read_key(  );
         if ( c > 31 || c == 8 || c == 13 || c == 27)
         {
            return ( c );
         }
      }
   }                            /* for */
}                               /* timed_read_key */

int read_key( void )
{
   int c;
   DWORD mode, ct;
   INPUT_RECORD event;

   GetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), &mode );
   SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), ENABLE_PROCESSED_INPUT );

 read_key_top:

   do
     ReadConsoleInput( GetStdHandle( STD_INPUT_HANDLE ), &event, 1, &ct );
   while ( event.EventType != KEY_EVENT || !event.Event.KeyEvent.bKeyDown );

   c = event.Event.KeyEvent.uChar.UnicodeChar;

   if ( c )
   {
      if ( c < 32 && !( c == 8 || c == 13 || c == 27 ) )
         goto read_key_top;

      if ( c == 0x7f )
         c = '\b';

      SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), mode );
      if ( !unicode && c >= 0xa0 )
         return '?';
      return ( translate_to_zscii( c ) );
   }

   c = event.Event.KeyEvent.wVirtualKeyCode;

   if ( c == 0x26 )
      c = 0x81;                        /* Up arrow                */
   else if ( c == 0x28 )
      c = 0x82;                        /* Down arrow              */
   else if ( c == 0x25 )
      c = 0x83;                        /* Left arrow              */
   else if ( c == 0x27 )
      c = 0x84;                        /* Right arrow             */
   else if ( c == 0x23 )
      c= 0x92;                         /* End (SW)                */
   else if ( c == 0x22 )
      c = 0x94;                        /* PgDn (SE)               */
   else if ( c == 0x0c )
      c = 0x96;                        /* Keypad 5                */
   else if ( c == 0x24 )
      c = 0x98;                        /* Home (NW)               */
   else if ( c == 0x21 )
      c = 0x9a;                        /* PgUp (NE)               */
   else if ( c >= 0x70 && c <= 0x7b )
      c += 21;                         /* Function keys F1 to F12 */
   else
      goto read_key_top;

   SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), mode );
   return c;

}                               /* read_key */

void scroll_line( void )
{
   CONSOLE_SCREEN_BUFFER_INFO info;
   int row, col;
   int screenheight;
   get_cursor_position( &row, &col );
   clear_line();

   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
   screenheight = info.srWindow.Bottom - info.srWindow.Top + 1;

   if ( row == screenheight )
   {
      SMALL_RECT r;
      CHAR_INFO* buffer;
      COORD size;

      size.X = info.srWindow.Right - info.srWindow.Left + 1;
      size.Y = screenheight - status_size - 1;

      if (size.Y > 0)
      {
         buffer = malloc (size.X * size.Y * sizeof(CHAR_INFO));

         r = (SMALL_RECT) {info.srWindow.Left, info.srWindow.Top + status_size + 1,
            info.srWindow.Right, info.srWindow.Bottom};

         ReadConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
            (PCHAR_INFO) buffer, size, (COORD) {0, 0}, &r);

         r = (SMALL_RECT) {info.srWindow.Left, info.srWindow.Top + status_size,
            info.srWindow.Right, info.srWindow.Bottom - 1};

         WriteConsoleOutput (GetStdHandle (STD_OUTPUT_HANDLE),
            buffer, size, (COORD) {0, 0}, &r);

         free(buffer);
      }
   }
   else
   {
      row++;
   }
   move_cursor( row, 1);
   clear_line();
}                               /* scroll_line */

/*
 * set_colours
 *
 * Sets the screen foreground and background colours.
 *
 */

void set_colours( zword_t foreground, zword_t background )
{
   ZINT16 fg, bg;
   int colour_map[] = { BLACK, RED, GREEN, BROWN, BLUE, MAGENTA, CYAN, WHITE };

   /* Translate from Z-code colour values to natural colour values */
   if ( ( ZINT16 ) foreground >= 1 && ( ZINT16 ) foreground <= 9 )
   {
      fg = ( foreground == 1 ) ? colour_map[default_fg] : colour_map[( ZINT16 ) foreground - 2];
   }
   if ( ( ZINT16 ) background >= 1 && ( ZINT16 ) background <= 9 )
   {
      bg = ( background == 1 ) ? colour_map[default_bg] : colour_map[( ZINT16 ) background - 2];
   }

   /* Set foreground and background colour */
   SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
      fg + (bg << 4));

   /* Save new foreground and background colours for restoring colour */
   current_fg = ( ZINT16 ) fg;
   current_bg = ( ZINT16 ) bg;

}                               /* set_colours */

int check_font_char( int c )
{
   HWND handle = GetConsoleWindow();
   HDC dc = GetDC(handle);
   HGDIOBJ of;
   LOGFONT lf;
   CONSOLE_FONT_INFOEX font;
   int size;
   int supported = 0;

   font.cbSize = sizeof(CONSOLE_FONT_INFOEX);
   GetCurrentConsoleFontEx(GetStdHandle (STD_OUTPUT_HANDLE), 0, &font);

   memset(&lf, 0, sizeof(lf));
   lf.lfHeight = font.dwFontSize.Y;
   wcscpy(lf.lfFaceName, font.FaceName);

   of = SelectObject(dc, CreateFontIndirect (&lf));
   size = GetFontUnicodeRanges(dc, NULL);

   if (size)
   {
      GLYPHSET* set = (GLYPHSET*)malloc(size);
      int x;

      if (GetFontUnicodeRanges(dc, set))
      {
         for (x = 0; x < set->cRanges; x++)
         {
            if (c >= set->ranges[x].wcLow &&
                c < (set->ranges[x].wcLow + set->ranges[x].cGlyphs))
            {
               supported = 1;
               break;
            }
         }
      }
      free(set);
   }

   DeleteObject(SelectObject(dc, of));
   ReleaseDC(handle, dc);

   return supported;
}

