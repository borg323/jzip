
/* $Id: cursesio.c,v 1.2 2000/10/04 23:11:44 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: cursesio.c,v 1.2 2000/10/04 23:11:44 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: cursesio.c,v $
 * Revision 1.2  2000/10/04 23:11:44  jholder
 * made zscii2latin1 table global
 *
 * Revision 1.1.1.1  2000/05/10 14:20:50  jholder
 *
 * imported
 *
 *       
 * --------------------------------------------------------------------
 */

/*
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * WARNING: the curses/ncurses library support is _very_ raw and
 *          not quite complete!  Use at your own risk!
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/* cursesio.c */

/*
 *  I/O module by John Holder 27-May-1998
 */

#define NCURSES_WIDECHAR 1

#include "ztypes.h"
#include <wctype.h>
#include <locale.h>

#include <ncursesw/curses.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#if defined(AIX)
#include <sys/select.h>
#endif

#define EXTENDED 1
#define PLAIN    2

attr_t current_attrib;

#ifdef HARD_COLORS
static ZINT16 current_fg;
static ZINT16 current_bg;
#endif
extern ZINT16 default_fg;
extern ZINT16 default_bg;

extern int hist_buf_size;

/* new stuff for command editing */
int BUFFER_SIZE;
char *commands;
int space_avail;
static int ptr1, ptr2 = 0;
static int end_ptr = 0;
static int row, head_col;

/* done with editing global info */

static int current_row = 1;
static int current_col = 1;

static int saved_row;
static int saved_col;

static int cursor_saved = OFF;

/* begin windowing variables */

WINDOW *zipscr = NULL;

/* end windowing variables */

static void outc(  );
static void display_string( char *s );
static int wait_for_char(  );
static int read_key( int );
static void set_cbreak_mode( int );
//static void rundown(  );
static void get_prev_command(  );
static void get_next_command(  );
static void get_first_command(  );
static void delete_command(  );
void add_command( char *, int );
int display_command( char * );
int input_line( int, char *, int, int *, int );
int input_character( int );
static int wait_for_char( int );

void outc( int );
void move_cursor( int, int );
void get_cursor_position( int *, int * );
void set_attribute( int );
void display_char( int );

/* done with editing prototypes */

static int colour_map[] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
                            COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };

/* print unicode char */

void outc( int c )
{
   int x, y;

   getyx( stdscr, y, x );
   if ( c == '\n' )
   {
      if ( ++y < LINES )
         move(y, 0);
   }
   else
   {
      if ( x >= COLS - 1 )
      {
         /* Close to the edge so no wrapping */
         if ( c <= 0x7f )
         {
            chtype s = c | current_attrib | COLOR_PAIR( 8 * current_bg + current_fg + 1 );
            addchnstr(&s, 1);
         }
         else
         {
            cchar_t ch;
            wchar_t wch = c;
            setcchar( &ch, &wch, current_attrib, A_COLOR & (8 * current_bg + current_fg + 1), NULL );
            add_wchnstr( &ch, 1 );
         }
         move( current_row - 1, COLS );
      }
      else if ( c <= 0x7f )
         addch( c );
      else
      {
         cchar_t ch;
         wchar_t wch = c;
         setcchar( &ch, &wch, current_attrib, A_COLOR & (8 * current_bg + current_fg + 1), NULL );
         add_wch( &ch );
      }
   }
   refresh( );
}                               /* outc */

void initialize_screen(  )
{
   int row, col;
   int i,j;

   setlocale( LC_ALL, "" );

   /* start the curses environment */
   if ( !initscr(  ) )
   {
      fatal( "initialize_screen(): Couldn't init curses." );
   }
   noecho(  );                  /* by default, don't echo what user types */
   nonl(  );                    /* don't xlate newlines */
   keypad( stdscr, TRUE );      /* Turn on the keypad */
   meta( stdscr, TRUE );        /* Turn on 8-bit clean input */

   set_escdelay( 0 );

   /* set all colour pairs to be used */
   start_color();
   for ( i = 0; i < 8; i++ )
       for ( j = 0; j < 8; j++ )
         init_pair( 8 * j + i + 1, colour_map[i], colour_map[j] );

   /* COLS and LINES set by curses */
   screen_cols = COLS;
   screen_rows = LINES;

#if defined HARD_COLORS
   if ( monochrome )
   {
      default_bg = 0;           /* black */
   }

   set_colours( 1, 1 );
#endif

   clear_screen(  );

   row = screen_rows / 2 - 1;
   col = ( screen_cols - ( sizeof ( JZIPVER ) - 1 ) ) / 2;
   move_cursor( row, col );
   display_string( JZIPVER );
   row = screen_rows / 2;
   col = ( screen_cols - ( sizeof ( "The story is loading..." ) - 1 ) ) / 2;
   move_cursor( row, col );
   display_string( "The story is loading..." );

   /* Last release (2.0.1g) claimed DEC tops 20.  I'm a sadist. Sue me. */
   h_interpreter = INTERP_MSDOS;
   JTERP = INTERP_UNIX;

   commands = ( char * ) malloc( hist_buf_size * sizeof ( char ) );

   if ( commands == NULL )
      fatal( "initialize_screen(): Couldn't allocate history buffer." );
   BUFFER_SIZE = hist_buf_size;
   space_avail = hist_buf_size - 1;

   set_cbreak_mode( 1 );
   interp_initialized = 1;

}                               /* initialize_screen */

void restart_screen(  )
{
   zbyte_t high = 1, low = 0;
   /* changing the character set is not standard compiant */
   if ( fIBMGraphics )
      high = low = 0;

   cursor_saved = OFF;

   set_byte( H_STANDARD_HIGH, high );
   set_byte( H_STANDARD_LOW, low );
   if ( h_type < V4 )
      set_byte( H_CONFIG, ( get_byte( H_CONFIG ) | CONFIG_WINDOWS ) );
   else
   {
      /* turn stuff on */
      set_byte( H_CONFIG,
                ( get_byte( H_CONFIG ) | CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED |
                  CONFIG_TIMEDINPUT ) );
#if defined HARD_COLORS
      set_byte( H_CONFIG, ( get_byte( H_CONFIG ) | CONFIG_COLOUR ) );
      set_byte( H_BG_DEFAULT_COLOR, default_bg + 2 );
      set_byte( H_FG_DEFAULT_COLOR, default_fg + 2 );
#endif
      /* turn stuff off */
      set_byte( H_CONFIG, ( get_byte( H_CONFIG ) & ~CONFIG_PICTURES & ~CONFIG_SFX ) );
   }

   /* Force graphics off as we can't do them */
   set_word( H_FLAGS, ( get_word( H_FLAGS ) & ( ~GRAPHICS_FLAG ) ) );

}                               /* restart_screen */

void reset_screen(  )
{
   /* only do this stuff on exit when called AFTER initialize_screen */
   if ( interp_initialized )
   {
      scroll_line();
      display_string( "[Hit any key to exit.]" );
      getch(  );

      delete_status_window(  );
      select_text_window(  );

      set_attribute( NORMAL );

      erase(  );
      set_cbreak_mode( 0 );
      echo(  );
      nl(  );

      /* End the curses screen environment */
      endwin(  );
   }
}                               /* reset_screen */

void clear_screen(  )
{
//   erase(  );                   /* clear screen */
   int row;

   for ( row = 1; row <= screen_rows; row++ )
   {
      move_cursor( row, 1 );
      clear_line(  );
   }
   move_cursor( 1, 1 );
}                               /* clear_screen */


void select_status_window(  )
{
   save_cursor_position(  );
}                               /* select_status_window */


void select_text_window(  )
{
   restore_cursor_position(  );
}                               /* select_text_window */

void create_status_window(  )
{
   int row, col;

   get_cursor_position( &row, &col );

   /* set up a software scrolling region */

/*
    setscrreg(status_size, screen_rows-1); 
*/

   move_cursor( row, col );
}                               /* create_status_window */

void delete_status_window(  )
{
   int row, col;

   get_cursor_position( &row, &col );

   /* set up a software scrolling region */

/*
    setscrreg(0, screen_rows-1); 
*/

   move_cursor( row, col );

}                               /* delete_status_window */

void clear_line(  )
{
//   clrtoeol(  );
   int i;

   for ( i = 1; i <= screen_cols; i++ )
      outc( ' ' );
}                               /* clear_line */

void clear_text_window(  )
{
   int i, row, col;

   get_cursor_position( &row, &col );

   for ( i = status_size + 1; i <= screen_rows; i++ )
   {
      move_cursor( i, 1 );
      clear_line(  );
   }

   move_cursor( row, col );

}                               /* clear_text_window */

void clear_status_window(  )
{
   int i, row, col;

   get_cursor_position( &row, &col );

   for ( i = status_size; i; i-- )
   {
      move_cursor( i, 1 );
      clear_line(  );
   }

   move_cursor( row, col );

}                               /* clear_status_window */

void move_cursor( int row, int col )
{
   move( row - 1, col - 1 );
   current_row = row;
   current_col = col;

}                               /* move_cursor */

void get_cursor_position( int *row, int *col )
{
   int x, y;

   getyx( stdscr, y, x );
   *row = y + 1;
   *col = x + 1;
}                               /* get_cursor_position */

void save_cursor_position(  )
{
   if ( cursor_saved == OFF )
   {
      get_cursor_position( &saved_row, &saved_col );
      cursor_saved = ON;
   }
}                               /* save_cursor_position */


void restore_cursor_position(  )
{
   if ( cursor_saved == ON )
   {
      move_cursor( saved_row, saved_col );
      cursor_saved = OFF;
   }
}                               /* restore_cursor_position */

void set_attribute( int attribute )
{
   if ( attribute == NORMAL )
   {
      attroff( WA_REVERSE | WA_BOLD | WA_UNDERLINE );
      current_attrib = WA_NORMAL;
   }

   if ( attribute & REVERSE )
   {
      attron( WA_REVERSE );
      current_attrib |= WA_REVERSE;
   }

   if ( attribute & BOLD )
   {
      attron( WA_BOLD );
      current_attrib |= WA_BOLD;
   }
   if ( attribute & EMPHASIS )
   {
      attron( WA_UNDERLINE );
      current_attrib |= WA_UNDERLINE;
   }

   if ( attribute & FIXED_FONT )
   {
   }
}                               /* set_attribute */

static void display_string( char *s )
{
   while ( *s )
      display_char( *s++ );
}                               /* display_string */

void display_char( int c )
{
   if ( c == 7 )
   {
      beep();
      return;
   }
   outc( c );

   if ( ++current_col > screen_cols )
      current_col = screen_cols;
}                               /* display_char */

void scroll_line(  )
{
   int row, col;

   get_cursor_position( &row, &col );

   if ( row < screen_rows )
   {
      display_char( '\n' );
   }
   else
   {
      scrollok( stdscr, TRUE );
      setscrreg( status_size, screen_rows - 1 );
      wscrl( stdscr, 1 );
      setscrreg( 0, screen_rows - 1 );
      scrollok( stdscr, FALSE );
      display_char( '\n' );
   }

   current_col = 1;
   if ( ++current_row > screen_rows )
   {
      current_row = screen_rows;
   }
   move_cursor( current_row, 1 );
   clear_line();
   move_cursor( current_row, 1 );

}                               /* scroll_line */

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
 * PgUp displays the first ("oldest") command, while PgDn displays a blank
 * prompt.
 */
int display_command( char *buffer )
{
   int counter, loop;

   move_cursor( row, head_col );
   clear_line(  );
   move_cursor( row, head_col );

   /* ptr1 = end_ptr when the player has selected beyond any previously
    * saved command.
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
         display_char( translate_from_zscii ( buffer[counter++] ) );
      }
      return ( counter );
   }
}                               /* display_command */

void get_prev_command(  )
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

            /* Decrement ptr1 until a '\n' is found */

            ptr1--;
         while ( ( ptr1 >= 0 ) && ( commands[ptr1] != '\n' ) );

         /* Then advance back to the position after the '\n' */

         ptr1++;
      }
   }
}                               /* get_prev_command */

void get_next_command(  )
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
            ptr2++;
         while ( ( commands[ptr2] != '\n' ) && ( ptr2 <= end_ptr ) );
         ptr2--;
      }
   }
}                               /* get_next_command */

void get_first_command(  )
{

   if ( end_ptr > 1 )
   {
      ptr1 = ptr2 = 0;
      do
         ptr2++;
      while ( commands[ptr2] != '\n' );
      ptr2--;
   }
}                               /* get_first_command */

void delete_command(  )
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
   int keyfunc = 0;

   /*
    * init_char_pos : the initial cursor location
    * curr_char_pos : the current character position within the input line
    * head_col: the head of the input line (used for cursor position)
    *  (global variable)
    * tail_col: the end of the input line (used for cursor position)
    */

   get_cursor_position( &row, &col );
   head_col = start_col;
   tail_col = start_col + *read_size;

   init_char_pos = curr_char_pos = col - start_col;

   ptr1 = ptr2 = end_ptr;

   for ( ;; )
   {
      keyfunc = 0;

      /* Read a single keystroke */

      fflush( stdout );

      if ( timeout != 0 )
      {
         if ( wait_for_char( timeout ) )
            return ( -1 );
      }
      c = read_key( EXTENDED );

      if ( ( c >= 0x81 && c <= 0x9a ) || c == 27 || c == 0xff )
      {
         keyfunc = 1;
         if ( c == 0x81 )
         {                   /* Up arrow */
            get_prev_command(  );
            curr_char_pos = *read_size = display_command( buffer );
            tail_col = head_col + *read_size;
         }
         else if ( c == 0x82 )
         {                   /* Down arrow */
            get_next_command(  );
            curr_char_pos = *read_size = display_command( buffer );
            tail_col = head_col + *read_size;
         }
         else if ( c == 0x09a )
         {                   /* PgUp */
            get_first_command();
            curr_char_pos = *read_size = display_command( buffer );
            tail_col = head_col + *read_size;
         }
         else if ( c == 0x094 || c == 27 )
         {                   /* Esc */
            ptr1 = ptr2 = end_ptr;
            curr_char_pos = *read_size = display_command( buffer );
            tail_col = head_col + *read_size;
         }

         /****** Cursor Editing Keys ******/

         else if ( c == 0x83 )
         {                   /* Left arrow */
            get_cursor_position( &row, &col );

            /* Prevents moving the cursor into the prompt */

            if ( col > head_col )
            {
               move_cursor( row, --col );
               curr_char_pos--;
            }
         }
         else if ( c == 0x84 )
         {                   /* Right arrow */
            get_cursor_position( &row, &col );

            /* Prevents moving the cursor beyond the end of the input line */

            if ( col < tail_col )
            {
               move_cursor( row, ++col );
               curr_char_pos++;
            }
         }
         else if ( c == 0x092 )
         {                  /* End */
            move_cursor( row, tail_col );
            curr_char_pos = init_char_pos + *read_size;
         }
         else if ( c == 0x098 )
         {                  /* Home */
            move_cursor( row, head_col );
            curr_char_pos = init_char_pos;
         }
         else if ( c == 0xff )
         {                  /* Delete */
            if (curr_char_pos < *read_size)
            {
               get_cursor_position ( &row, &col );

               for ( loop = curr_char_pos; loop < *read_size; loop++ )
               {
                  buffer[loop] = buffer[loop + 1];
               }

               tail_col--;
               ( *read_size )--;

               for ( loop = curr_char_pos; loop < *read_size; loop++ )
               {
                  display_char( translate_from_zscii ( buffer[loop] ) );
               }

               display_char( ' ' );

               move_cursor( row, col );
            }
         }
      }

      if ( !keyfunc )
      {
         if ( c == '\b' )       /* Backspace */
         {
            get_cursor_position( &row, &col );
            if ( col > head_col )
            {
               move_cursor( row, --col );
               for ( loop = curr_char_pos; loop < *read_size; loop++ )
               {
                  buffer[loop - 1] = buffer[loop];
                  display_char( translate_from_zscii ( buffer[loop - 1] ) );
               }
               display_char( ' ' );
               curr_char_pos--;
               tail_col--;
               ( *read_size )--;
               move_cursor( row, col );
            }
         }
         else
         {
            /* Normal key action */
            if ( *read_size == ( buflen - 1 ) )
            {
               /* Ring bell if buffer is full */
               flash(  );
            }
            else
            {
               /* Scroll line if return key pressed */
               if ( c == '\r' || c == '\n' )
               {
                  c = '\n';
                  move_cursor( row, tail_col );
                  scroll_line(  );
               }

               if ( c == '\n' )
               {
                  /* Add the current command to the command buffer */
                  if ( *read_size > space_avail )
                  {
                     do
                        delete_command(  );
                     while ( *read_size > space_avail );
                  }
                  if ( *read_size > 0 )
                     add_command( buffer, *read_size );

                  /* Return key if it is a line terminator */
                  return ( c );
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
                      * insertion */

                     for ( loop = curr_char_pos; loop < *read_size; loop++ )
                     {
                        display_char( translate_from_zscii ( buffer[loop] ) );
                     }

                     /* Moves the cursor to the next position */

                     move_cursor( row, ++col );
                     curr_char_pos++;
                  }
                  else
                  {
                     /* Used if the cursor is at the end of the line */
                     buffer[curr_char_pos++] = ( char ) c;
                     display_char( translate_from_zscii ( c ) );
                     ( *read_size )++;
                     tail_col++;
                  }
               }
            }
         }
      }
   }
}                               /* input_line */

/*
 * Patched 28-June-1995: Changed this routine's expectation of a \n to
 *                       a \r so the form in Bureaucracy works.  Patch
 *                       applied by John Holder.
 */
int input_character( int timeout )
{
   int c;

   fflush( stdout );

   if ( timeout != 0 )
   {
      if ( wait_for_char( timeout ) )
         return ( -1 );
   }

   c = read_key( PLAIN );

   return ( c );

}                               /* input_character */

/* timeout is in tenths of a second */
static int wait_for_char( int timeout )
{
   int nfds, status;
   fd_set readfds;
   struct timeval tv;
   struct timezone tz;

   gettimeofday( &tv, &tz );

   if ( timeout == 0 )
      return ( -1 );

   tv.tv_sec = ( timeout * 100000 ) / 1000000;
   tv.tv_usec = ( timeout * 100000 ) % 1000000;

   nfds = FD_SETSIZE;

   FD_ZERO( &readfds );
   FD_SET( fileno( stdin ), &readfds );

   status = select( nfds, &readfds, NULL, NULL, &tv );
   if ( status < 0 )
   {
      perror( "select" );
      return ( -1 );
   }

   if ( status == 0 )
      return ( -1 );
   else
      return ( 0 );

}                               /* wait_for_char */

/* mode == extended to return 0xff for DEL instread of 8 (BACKSPACE) */
static int read_key( int mode )
{
   int r;
   wint_t ch;

   do
   {
      r = get_wch( &ch );

      if ( r == KEY_CODE_YES )
      {
         switch ( ch )
         {
         case KEY_UP:
            return 0x81;
         case KEY_DOWN:
            return 0x82;
         case KEY_LEFT:
            return 0x83;
         case KEY_RIGHT:
            return 0x84;
         case KEY_F( 1 ):
            return 0x85;
         case KEY_F( 2 ):
            return 0x86;
         case KEY_F( 3 ):
            return 0x87;
         case KEY_F( 4 ):
            return 0x88;
         case KEY_F( 5 ):
            return 0x89;
         case KEY_F( 6 ):
            return 0x8a;
         case KEY_F( 7 ):
            return 0x8b;
         case KEY_F( 8 ):
            return 0x8c;
         case KEY_F( 9 ):
            return 0x8d;
         case KEY_F( 10 ):
            return 0x8e;
         case KEY_F( 11 ):
            return 0x8f;
         case KEY_F( 12 ):
            return 0x90;
         case KEY_IC:
            return 0x91;
         case KEY_END:
            return 0x92;
         case KEY_NPAGE:
            return 0x94;
         case KEY_B2:
            return 0x96;
         case KEY_HOME:
            return 0x98;
         case KEY_PPAGE:
            return 0x9A;
         case KEY_BACKSPACE:
            return '\b';
         case KEY_DC:
            if ( mode == EXTENDED )
               return 0xff;
            return '\b';
         default:
            r = ERR;
         }
      }
      else if ( r == OK && ch == 4 )
      {
         reset_screen(  );
         exit( 0 );
      }                      /* CTRL-D (EOF) */
      else if ( r == OK && ch == 127 )
      {
         if ( mode == EXTENDED )
            return ch;
         return '\b';
      }
      else if ( r == OK && ch == 10 )
         return 13;
   }
   while ( r == ERR || ( !( ch == 13 || ch == 8 || ch == 27 ) && ch < 32 ) );

   return ( translate_to_zscii( ch ) );
}                               /* read_key */

static void set_cbreak_mode( int mode )
{
   if ( mode )
   {
      cbreak(  );
   }
   else
   {
      nocbreak(  );
   }

}                               /* set_cbreak_mode */

#if 0
static void rundown(  )
{
   unload_cache(  );
   close_story(  );
   close_script(  );
   reset_screen(  );
}                               /* rundown */
#endif

#if defined HARD_COLORS
void set_colours( zword_t foreground, zword_t background )
{
   int fg = 0, bg = 0;

   /* Translate from Z-code colour values to natural colour values */

   if ( ( ZINT16 ) foreground >= 1 && ( ZINT16 ) foreground <= 9 )
   {
      fg = ( foreground == 1 ) ? default_fg : foreground - 2;
   }
   if ( ( ZINT16 ) background >= 1 && ( ZINT16 ) background <= 9 )
   {
      bg = ( background == 1 ) ? default_bg : background - 2;
   }

   current_fg = ( ZINT16 ) fg;
   current_bg = ( ZINT16 ) bg;

   /* Set foreground and background colour */
   attron( COLOR_PAIR( 8 * bg + fg + 1) );
}
#endif

int check_font_char( int c )
{
   /* A first approximation */
   return iswprint( c );
}

