
/* $Id: input.c,v 1.3 2000/07/05 15:20:34 jholder Exp $   
 * --------------------------------------------------------------------
 * see doc/License.txt for License Information   
 * --------------------------------------------------------------------
 * 
 * File name: $Id: input.c,v 1.3 2000/07/05 15:20:34 jholder Exp $  
 *   
 * Description:    
 *    
 * Modification history:      
 * $Log: input.c,v $
 * Revision 1.3  2000/07/05 15:20:34  jholder
 * Updated code to remove warnings.
 *
 * Revision 1.2  2000/05/25 22:28:56  jholder
 * changes routine names to reflect zmachine opcode names per spec 1.0
 *
 * Revision 1.1.1.1  2000/05/10 14:21:34  jholder
 *
 * imported
 *
 *
 * --------------------------------------------------------------------
 */

/*
 * input.c
 *
 * Input routines
 *
 */

#include "ztypes.h"

/* Statically defined word separator list */

static const char *separators = " \t\n\f.,?";
static zword_t dictionary_offset = 0;
static ZINT16 dictionary_size = 0;
static unsigned int entry_size = 0;

static void tokenise_line( zword_t, zword_t, zword_t, zword_t );
static const char *next_token( const char *, const char *, const char **, int *, const char * );
static zword_t find_word( int, const char *, long );

/*
 * z_read_char
 *
 * Read one character with optional timeout
 *
 *    argv[0] = input device (must be 1)
 *    argv[1] = timeout value in tenths of a second (optional)
 *    argv[2] = timeout action routine (optional)
 *
 */

void z_read_char( int argc, zword_t * argv )
{
   int c;
   zword_t arg_list[2];

   /* Supply default parameters */

   if ( argc < 3 )
      argv[2] = 0;
   if ( argc < 2 )
      argv[1] = 0;

   /* Flush any buffered output before read */

   flush_buffer( FALSE );

   /* Reset line count */

   lines_written = 0;

   /* If more than one characters was asked for then fail the call */

   if ( argv[0] != 1 )

      c = 0;

   else
   {

      if ( ( c = playback_key(  ) ) == -1 )
      {

         /* Setup the timeout routine argument list */

         arg_list[0] = argv[2];
         arg_list[1] = 0;       /* as per spec 1.0 */
         /* was: arg_list[1] = argv[1]/10; */

         /* Read a character with a timeout. If the input timed out then
          * call the timeout action routine. If the return status from the
          * timeout routine was 0 then try to read a character again */

         do
         {
            flush_buffer( FALSE );
            c = input_character( ( int ) argv[1] );
         }
         while ( c == -1 && z_call( 1, arg_list, ASYNC ) == 0 );

         /* Fail call if input timed out */

         if ( c == -1 )
            c = 0;
         else
            record_key( c );
      }
   }

   store_operand( (zword_t)c );

}                               /* z_read_char */

/* Table generated automatically from UnicodeData.txt */

static int toloweru_array[152][2] =  {
   {0x130, 0x69}, {0x132, 0x133}, {0x134, 0x135}, {0x136, 0x137}, {0x178, 0xff}, {0x179, 0x17a},
   {0x17b, 0x17c}, {0x17d, 0x17e}, {0x181, 0x253}, {0x182, 0x183}, {0x184, 0x185}, {0x186, 0x254},
   {0x187, 0x188}, {0x189, 0x256}, {0x18a, 0x257}, {0x18b, 0x18c}, {0x18e, 0x1dd}, {0x18f, 0x259},
   {0x190, 0x25b}, {0x191, 0x192}, {0x193, 0x260}, {0x194, 0x263}, {0x196, 0x269}, {0x197, 0x268},
   {0x198, 0x199}, {0x19c, 0x26f}, {0x19d, 0x272}, {0x19f, 0x275}, {0x1a0, 0x1a1}, {0x1a2, 0x1a3},
   {0x1a4, 0x1a5}, {0x1a6, 0x280}, {0x1a7, 0x1a8}, {0x1a9, 0x283}, {0x1ac, 0x1ad}, {0x1ae, 0x288},
   {0x1af, 0x1b0}, {0x1b1, 0x28a}, {0x1b2, 0x28b}, {0x1b3, 0x1b4}, {0x1b5, 0x1b6}, {0x1b7, 0x292},
   {0x1b8, 0x1b9}, {0x1bc, 0x1bd}, {0x1c4, 0x1c6}, {0x1c5, 0x1c6}, {0x1c7, 0x1c9}, {0x1c8, 0x1c9},
   {0x1ca, 0x1cc}, {0x1f1, 0x1f3}, {0x1f2, 0x1f3}, {0x1f4, 0x1f5}, {0x1f6, 0x195}, {0x1f7, 0x1bf},
   {0x220, 0x19e}, {0x23a, 0x2c65}, {0x23b, 0x23c}, {0x23d, 0x19a}, {0x23e, 0x2c66}, {0x241, 0x242},
   {0x243, 0x180}, {0x244, 0x289}, {0x245, 0x28c}, {0x370, 0x371}, {0x372, 0x373}, {0x376, 0x377},
   {0x37f, 0x3f3}, {0x386, 0x3ac}, {0x388, 0x3ad}, {0x389, 0x3ae}, {0x38a, 0x3af}, {0x38c, 0x3cc},
   {0x38e, 0x3cd}, {0x38f, 0x3ce}, {0x3cf, 0x3d7}, {0x3f4, 0x3b8}, {0x3f7, 0x3f8}, {0x3f9, 0x3f2},
   {0x3fa, 0x3fb}, {0x3fd, 0x37b}, {0x3fe, 0x37c}, {0x3ff, 0x37d}, {0x4c0, 0x4cf}, {0x10c7, 0x2d27},
   {0x10cd, 0x2d2d}, {0x1cbd, 0x10fd}, {0x1cbe, 0x10fe}, {0x1cbf, 0x10ff}, {0x1e9e, 0xdf}, {0x1fb8, 0x1fb0},
   {0x1fb9, 0x1fb1}, {0x1fba, 0x1f70}, {0x1fbb, 0x1f71}, {0x1fbc, 0x1fb3}, {0x1fcc, 0x1fc3}, {0x1fd8, 0x1fd0},
   {0x1fd9, 0x1fd1}, {0x1fda, 0x1f76}, {0x1fdb, 0x1f77}, {0x1fe8, 0x1fe0}, {0x1fe9, 0x1fe1}, {0x1fea, 0x1f7a},
   {0x1feb, 0x1f7b}, {0x1fec, 0x1fe5}, {0x1ff8, 0x1f78}, {0x1ff9, 0x1f79}, {0x1ffa, 0x1f7c}, {0x1ffb, 0x1f7d},
   {0x1ffc, 0x1ff3}, {0x2126, 0x3c9}, {0x212a, 0x6b}, {0x212b, 0xe5}, {0x2132, 0x214e}, {0x2183, 0x2184},
   {0x2c60, 0x2c61}, {0x2c62, 0x26b}, {0x2c63, 0x1d7d}, {0x2c64, 0x27d}, {0x2c67, 0x2c68}, {0x2c69, 0x2c6a},
   {0x2c6b, 0x2c6c}, {0x2c6d, 0x251}, {0x2c6e, 0x271}, {0x2c6f, 0x250}, {0x2c70, 0x252}, {0x2c72, 0x2c73},
   {0x2c75, 0x2c76}, {0x2c7e, 0x23f}, {0x2c7f, 0x240}, {0x2ceb, 0x2cec}, {0x2ced, 0x2cee}, {0x2cf2, 0x2cf3},
   {0xa779, 0xa77a}, {0xa77b, 0xa77c}, {0xa77d, 0x1d79}, {0xa78b, 0xa78c}, {0xa78d, 0x265}, {0xa790, 0xa791},
   {0xa792, 0xa793}, {0xa7aa, 0x266}, {0xa7ab, 0x25c}, {0xa7ac, 0x261}, {0xa7ad, 0x26c}, {0xa7ae, 0x26a},
   {0xa7b0, 0x29e}, {0xa7b1, 0x287}, {0xa7b2, 0x29d}, {0xa7b3, 0xab53}, {0xa7c2, 0xa7c3}, {0xa7c4, 0xa794},
   {0xa7c5, 0x282}, {0xa7c6, 0x1d8e}
};

/* tolower for every char in UnicodeData.txt as of March 2019 */

static int toloweru ( int c )
{
   if ( c < 0x41 )
      return c;
   else if ( c >= 0x41 && c <= 0x5a )
      return c + 0x20;
   else if ( c > 0x5a && c < 0xc0 )
      return c;
   else if ( c >= 0xc0 && c <= 0xde && c != 0xd7 )
      return c + 0x20;
   else if ( c >= 0x100 && c <= 0x12e )
      return c | 1;
   else if ( c >= 0x139 && c <= 0x147 && ( c & 1 ) )
      return c + 1;
   else if ( c >= 0x14a && c <= 0x176 )
      return c | 1;
   else if ( c >= 0x1cb && c <= 0x1db && ( c & 1 ) )
      return c + 1;
   else if ( c >= 0x1de && c <= 0x1ee )
      return c | 1;
   else if ( c >= 0x1f8 && c <= 0x21e )
      return c | 1;
   else if ( c >= 0x222 && c <= 0x232 )
      return c | 1;
   else if ( c >= 0x246 && c <= 0x24e )
      return c | 1;
   else if ( c >= 0x391 && c <= 0x3ab && c != 0x3a2 )
      return c + 0x20;
   else if ( c >= 0x3d8 && c <= 0x3ee )
      return c | 1;
   else if ( c >= 0x400 && c <= 0x40f )
      return c + 0x50;
   else if ( c >= 0x410 && c <= 0x42f )
      return c + 0x20;
   else if ( c >= 0x460 && c <= 0x480 )
      return c | 1;
   else if ( c >= 0x48a && c <= 0x4be )
      return c | 1;
   else if ( c >= 0x4c1 && c <= 0x4cd && ( c & 1 ) )
      return c + 1;
   else if ( c >= 0x4d0 && c <= 0x52e )
      return c | 1;
   else if ( c >= 0x531 && c <= 0x556 )
      return c + 0x30;
   else if ( c >= 0x10a0 && c <= 0x10c5 )
      return c + 0x1c60;
   else if ( c >= 0x13a0 && c <= 0x13ef )
      return c + 0x97d0;
   else if ( c >= 0x13f0 && c <= 0x13f5 )
      return c + 0x8;
   else if ( c >= 0x1c90 && c <= 0x1cba )
      return c - 0xbc0;
   else if ( c >= 0x1e00 && c <= 0x1e94 )
      return c | 1;
   else if ( c >= 0x1ea0 && c <= 0x1efe )
      return c | 1;
   else if ( c >= 0x1f08 && c <= 0x1f4d && ( c & 0x8 ) )
      return c - 0x8;
   else if ( c >= 0x1f59 && c <= 0x1f5f && ( c & 1 ) )
      return c - 0x8;
   else if ( c >= 0x1f68 && c <= 0x1f6f )
      return c - 0x8;
   else if ( c >= 0x1f88 && c <= 0x1faf && ( c & 0x8 ) )
      return c - 0x8;
   else if ( c >= 0x1fc8 && c <= 0x1fcb )
      return c - 0x56;
   else if ( c >= 0x2160 && c <= 0x216f )
      return c + 0x10;
   else if ( c >= 0x24b6 && c <= 0x24cf )
      return c + 0x1a;
   else if ( c >= 0x2c00 && c <= 0x2c2e )
      return c + 0x30;
   else if ( c >= 0x2c80 && c <= 0x2ce2 )
      return c | 1;
   else if ( c >= 0xa640 && c <= 0xa66c )
      return c | 1;
   else if ( c >= 0xa680 && c <= 0xa69a )
      return c | 1;
   else if ( c >= 0xa722 && c <= 0xa72e )
      return c | 1;
   else if ( c >= 0xa732 && c <= 0xa76e )
      return c | 1;
   else if ( c >= 0xa77e && c <= 0xa786 )
      return c | 1;
   else if ( c >= 0xa796 && c <= 0xa7a8 )
      return c | 1;
   else if ( c >= 0xa7b4 && c <= 0xa7be )
      return c | 1;
   else if ( c >= 0xff21 && c <= 0xff3a )
      return c + 0x20;
#if 0
   else if ( c >= 0x10400 && c <= 0x10427 )
      return c + 0x28;
   else if ( c >= 0x104b0 && c <= 0x104d3 )
      return c + 0x28;
   else if ( c >= 0x10c80 && c <= 0x10cb2 )
      return c + 0x40;
   else if ( c >= 0x118a0 && c <= 0x118bf )
      return c + 0x20;
   else if ( c >= 0x16e40 && c <= 0x16e5f )
      return c + 0x20;
   else if ( c >= 0x1e900 && c <= 0x1e921 )
      return c + 0x22;
#endif
   else
   {
      int i;
      for ( i = 0; i < 152; i ++)
         if ( toloweru_array[i][0] == c )
            return toloweru_array[i][1];
   }
   return c;
}

/*
 * z_sread_aread
 *
 * Read a line of input with optional timeout.
 *
 *    argv[0] = character buffer address
 *    argv[1] = token buffer address
 *    argv[2] = timeout value in seconds (optional)
 *    argv[3] = timeout action routine (optional)
 *
 */

void z_sread_aread( int argc, zword_t * argv )
{
   int c, i, in_size, out_size, terminator;
   char *cbuf, *buffer;

   /* Supply default parameters */

   if ( argc < 4 )
      argv[3] = 0;
   if ( argc < 3 )
      argv[2] = 0;
   if ( argc < 2 )
      argv[1] = 0;

   /* Refresh status line */

   if ( h_type < V4 )
      z_show_status(  );

   /* Flush any buffered output before read */

   flush_buffer( TRUE );

   /* Reset line count */

   lines_written = 0;

   /* Initialise character pointer and initial read size */

   cbuf = ( char * ) &datap[argv[0]];
   in_size = ( h_type > V4 ) ? cbuf[1] : 0;

   /* Read the line then script and record it */

   terminator = get_line( cbuf, argv[2], argv[3] );
   script_line( ( h_type > V4 ) ? &cbuf[2] : &cbuf[1] );
   record_line( ( h_type > V4 ) ? &cbuf[2] : &cbuf[1] );

   /* Convert new text in line to lowercase */

   if ( h_type > V4 )
   {
      buffer = &cbuf[2];
      out_size = cbuf[1];
   }
   else
   {
      buffer = &cbuf[1];
      out_size = strlen( buffer );
   }

   if ( out_size > 0 )
      for ( i = 0; i < out_size; i++ )
      {
         c = translate_to_zscii( toloweru( translate_from_zscii( buffer[i] ) ) );
         if ( c != '?' )
            buffer[i] = ( char ) c;
      }

   /* Tokenise the line, if a token buffer is present */

   if ( argv[1] )
      tokenise_line( argv[0], argv[1], h_words_offset, 0 );

   /* Return the line terminator */

   if ( h_type > V4 )
      store_operand( ( zword_t ) terminator );

}                               /* z_sread_aread */

/*
 * get_line
 *
 * Read a line of input and lower case it.
 *
 */

int get_line( char *cbuf, zword_t timeout, zword_t action_routine )
{
   char *buffer;
   int buflen, read_size, status, c;
   zword_t arg_list[2];
   int row, col, prev_col, start_col;

   /* Set maximum buffer size to width of screen minus any
    * right margin and 1 character for a terminating NULL */

   buflen = ( screen_cols > 127 ) ? 127 : screen_cols;
   buflen -= right_margin + 1;
   if ( ( int ) cbuf[0] <= buflen )
      buflen = cbuf[0];

   /* Set read size and start of read buffer. The buffer may already be
    * primed with some text in V5 games. The Z-code will have already
    * displayed the text so we don't have to do that */

   if ( h_type > V4 )
   {
      read_size = cbuf[1];
      buffer = &cbuf[2];
   }
   else
   {
      read_size = 0;
      buffer = &cbuf[1];
   }

   /* Try to read input from command file */

   c = playback_line( buflen, buffer, &read_size );

   if ( c == -1 )
   {

      /* Setup the timeout routine argument list */

      arg_list[0] = action_routine;
      arg_list[1] = 0;          /*  as per spec.1.0  */
      /* arg_list[1] = timeout/10; */

      get_cursor_position( &row, &col );
      start_col = col - read_size;
      prev_col = col;

      /* Read a line with a timeout. If the input timed out then
       * call the timeout action routine. If the return status from the
       * timeout routine was 0 then try to read the line again */

      do
      {
         get_cursor_position( &row, &col );
         if (col != start_col + read_size)
         {
            int i;
            for (i = 0; i < read_size; i++ )
               write_zchar( ( unsigned char ) buffer[i] );
            flush_buffer( FALSE );
         }
         move_cursor( row, prev_col );
         c = input_line( buflen, buffer, timeout, &read_size, start_col );
         if ( c == -1)
         {
            get_cursor_position( &row, &prev_col );
            move_cursor( row, start_col + read_size );
         }
         status = 0;
      }
      while ( c == -1 && ( status = z_call( 1, arg_list, ASYNC ) ) == 0 );

      /* Throw away any input if timeout returns success */

      if ( status )
         read_size = 0;

   }

   /* Zero terminate line */

   if ( h_type > V4 )
   {
      cbuf[1] = ( char ) read_size;
   }
   else
   {
      /* Zero terminate line (V1-4 only) */
      buffer[read_size] = '\0';
   }

   return ( c );

}                               /* get_line */

/*
 * tokenise_line
 *
 * Convert a typed input line into tokens. The token buffer needs some
 * additional explanation. The first byte is the maximum number of tokens
 * allowed. The second byte is set to the actual number of token read. Each
 * token is composed of 3 fields. The first (word) field contains the word
 * offset in the dictionary, the second (byte) field contains the token length,
 * and the third (byte) field contains the start offset of the token in the
 * character buffer.
 *
 */

static void tokenise_line( zword_t char_buf, zword_t token_buf, zword_t dictionary, zword_t flag )
{
   int i, count, words, token_length;
   long word_index, chop = 0;
   int slen;
   char *str_end;
   char *cbuf, *tbuf, *tp;
   const char *cp, *token=NULL;
   char punctuation[16];
   zword_t word;

   /* Initialise character and token buffer pointers */

   cbuf = ( char * ) &datap[char_buf];
   tbuf = ( char * ) &datap[token_buf];

   /* Find the string length */

   if ( h_type > V4 )
   {
      slen = ( unsigned char ) ( cbuf[1] );
      str_end = cbuf + 2 + slen;
   }
   else
   {
      slen = strlen( cbuf + 1 );
      str_end = cbuf + 1 + slen;
   }

   /* Initialise word count and pointers */

   words = 0;
   cp = ( h_type > V4 ) ? cbuf + 2 : cbuf + 1;
   tp = tbuf + 2;

   /* Initialise dictionary */

   count = get_byte( dictionary++ );
   for ( i = 0; i < count; i++ )
      punctuation[i] = get_byte( dictionary++ );
   punctuation[i] = '\0';
   entry_size = get_byte( dictionary++ );
   dictionary_size = ( ZINT16 ) get_word( dictionary );
   dictionary_offset = dictionary + 2;

   /* Calculate the binary chop start position */

   if ( dictionary_size > 0 )
   {
      word_index = dictionary_size / 2;
      chop = 1;
      do
         chop *= 2;
      while ( word_index /= 2 );
   }

   /* Tokenise the line */

   do
   {

      /* Skip to next token */

      cp = next_token( cp, str_end, &token, &token_length, punctuation );
      if ( token_length ) {

         /* If still space in token buffer then store word */

         if ( words <= tbuf[0] )
         {

            /* Get the word offset from the dictionary */

            word = find_word( token_length, token, chop );

            /* Store the dictionary offset, token length and offset */

            if ( word || flag == 0 )
            {
               tp[0] = ( char ) ( word >> 8 );
               tp[1] = ( char ) ( word & 0xff );
            }
            tp[2] = ( char ) token_length;
            tp[3] = ( char ) ( token - cbuf );

            /* Step to next token position and count the word */

            tp += 4;
            words++;
         }
         else
         {

            /* Moan if token buffer space exhausted */

            output_string( "Too many words typed, discarding: " );
            output_line( token );
         }
      }
   }
   while ( token_length );

   /* Store word count */

   tbuf[1] = ( char ) words;

}                               /* tokenise_line */

/*
 * next_token
 *
 * Find next token in a string. The token (word) is delimited by a statically
 * defined and a game specific set of word separators. The game specific set
 * of separators look like real word separators, but the parser wants to know
 * about them. An example would be: 'grue, take the axe. go north'. The
 * parser wants to know about the comma and the period so that it can correctly
 * parse the line. The 'interesting' word separators normally appear at the
 * start of the dictionary, and are also put in a separate list in the game
 * file.
 *
 */

static const char *next_token( const char *s, const char *str_end, const char **token, int *length,
                               const char *punctuation )
{
   int i;

   /* Set the token length to zero */

   *length = 0;

   /* Step through the string looking for separators */

   for ( ; s < str_end; s++ )
   {

      /* Look for game specific word separators first */

      for ( i = 0; punctuation[i] && *s != punctuation[i]; i++ )
         ;

      /* If a separator is found then return the information */

      if ( punctuation[i] )
      {

         /* If length has been set then just return the word position */

         if ( *length )
            return ( s );
         else
         {

            /* End of word, so set length, token pointer and return string */

            ( *length )++;
            *token = s;
            return ( ++s );
         }
      }

      /* Look for statically defined separators last */

      for ( i = 0; separators[i] && *s != separators[i]; i++ )
         ;

      /* If a separator is found then return the information */

      if ( separators[i] )
      {

         /* If length has been set then just return the word position */

         if ( *length )
            return ( ++s );
      }
      else
      {

         /* If first token character then remember its position */

         if ( *length == 0 )
            *token = s;
         ( *length )++;
      }
   }

   return ( s );

}                               /* next_token */

/*
 * find_word
 *
 * Search the dictionary for a word. Just encode the word and binary chop the
 * dictionary looking for it.
 *
 */

static zword_t find_word( int len, const char *cp, long chop )
{
   ZINT16 word[3];
   long word_index, offset, status;

   /* Don't look up the word if there are no dictionary entries */

   if ( dictionary_size == 0 )
      return ( 0 );

   /* Encode target word */

   encode_text( len, cp, word );

   /* Do a binary chop search on the main dictionary, otherwise do
    * a linear search */

   word_index = chop - 1;

   if ( dictionary_size > 0 )
   {

      /* Binary chop until the word is found */

      while ( chop )
      {

         chop /= 2;

         /* Calculate dictionary offset */

         if ( word_index > ( dictionary_size - 1 ) )
            word_index = dictionary_size - 1;

         offset = dictionary_offset + ( word_index * entry_size );

         /* If word matches then return dictionary offset */

         if ( ( status = word[0] - ( ZINT16 ) get_word( offset + 0 ) ) == 0                &&
              ( status = word[1] - ( ZINT16 ) get_word( offset + 2 ) ) == 0                && 
              ( h_type < V4 || ( status = word[2] - ( ZINT16 ) get_word( offset + 4 ) ) == 0 ) )
            return ( ( zword_t ) offset );

         /* Set next position depending on direction of overshoot */

         if ( status > 0 )
         {
            word_index += chop;

            /* Deal with end of dictionary case */

            if ( word_index >= ( int ) dictionary_size )
               word_index = dictionary_size - 1;
         }
         else
         {
            word_index -= chop;

            /* Deal with start of dictionary case */

            if ( word_index < 0 )
               word_index = 0;
         }
      }
   }
   else
   {

      for ( word_index = 0; word_index < -dictionary_size; word_index++ )
      {

         /* Calculate dictionary offset */

         offset = dictionary_offset + ( word_index * entry_size );

         /* If word matches then return dictionary offset */

         if ( ( status = word[0] - ( ZINT16 ) get_word( offset + 0 ) ) == 0                &&
              ( status = word[1] - ( ZINT16 ) get_word( offset + 2 ) ) == 0                && 
              ( h_type < V4 || ( status = word[2] - ( ZINT16 ) get_word( offset + 4 ) ) == 0 ) )
            return ( ( zword_t ) offset );
      }
   }

   return ( 0 );

}                               /* find_word */

/*
 * z_tokenise
 *
 *    argv[0] = character buffer address
 *    argv[1] = token buffer address
 *    argv[2] = alternate vocabulary table
 *    argv[3] = ignore unknown words flag
 *
 */

void z_tokenise( int argc, zword_t * argv )
{

   /* Supply default parameters */

   if ( argc < 4 )
      argv[3] = 0;
   if ( argc < 3 )
      argv[2] = h_words_offset;

   /* Convert the line to tokens */

   tokenise_line( argv[0], argv[1], argv[2], argv[3] );

}                               /* z_tokenise */
