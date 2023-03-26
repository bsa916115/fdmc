#include <stdio.h>
#include <stdarg.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>

#include "fdmc_curses.h"

/* Some of UNIX curses don't provide getsyx function */
int scrx, scry;

#define selftest

/* Quickly designed structure for descrybing editor elements
 * It contains some information, that define the layout of editor
 * window and checking procedures (not implemented yet)
 *
 *         Designed for UNIX 
 *
 */
escape_sequence *terminal_sequences;


void fdmc_wiprintcenter(int ky, char *fmt, ...)
{
	va_list ap;
	char txt[256];
	
	va_start(ap, fmt);
	vsprintf(txt, fmt, ap);
	va_end(ap);
	move(ky, 1);
	printw("%79s", " ");
	move(ky, (80 - strlen(txt)) / 2);
	printw("%s", txt);
	refresh();
}

int fdmc_widisplay_error(edit_window *ed_win, char *fmt, ...)
{
	va_list ap;
	char s[256];
	int n;
	
	va_start(ap, fmt);
	n = vsprintf(s, fmt, ap);
	attrset(A_BOLD);
	refresh();
	fdmc_wiprintcenter(ERR_LINE, "ERROR : %s", s);
	draw_edit_window(ed_win); 
	va_end(ap);
	return n;
}

void fdmc_wiclear_error(edit_window *ed_win)
{
	attrset(A_BOLD);
	refresh();
	move(ERR_LINE, 1);
	printw("%79s", " ");
	refresh();
	draw_edit_window(ed_win);
}


void wprintwcenter(WINDOW *win, int y, char *txt)
{
	int maxy, maxx=80;

	move(y, 1);
	clrtoeol();	
	mvwprintw(win, y, (maxx - strlen(txt)) / 2, "%s", txt);
	return;
}


/* Get the string from terminal.
 * Some edit features are added
 */
int fdmc_wiget_line(edit_window *ed_win)
{
	int c;
	int vl; /* Uses as pointer to current position of edited string */
	int done;
	
	c = 0; vl = 0;
	/* Setup global cursor variables */
	scrx = ed_win->tx;
	scry = ed_win->ty;
	/* Main loop of the function */
	for(done = 0; ! done;)
	{
		c = ed_win->last_char = getch();
		/*
		to_file();
		trace("\nUser input %d, '%c'\n", c, c);
		to_screen();
		*/
		swchar:
		switch(c) 
		{
		case '\n': /* User have pressed enter key, end of edition */
			done = 1;
			break;
		case KEY_LEFT: /* Cursor movement to the left */
			if(scrx > ed_win->tx)
			{
				move(scry, --scrx);
				vl -- ;
			}
			break;
		case KEY_RIGHT: /* Cursor movement to the right */
			if(ed_win->buf[vl] != 0 && vl < ed_win->bufsize-1)
			{
				move(scry, ++scrx);
				vl ++;
			}
			break;
		case KEY_UP: /* This is useful to move from one editor window to another */
		case KEY_DOWN:
		case '\t':
			done = 1;
			break;
		case 27: /* Some terminals use escape sequences */
			c = hold_escape_sequence(terminal_sequences);
			goto swchar;
		case 2:
			/*
			to_file();
			trace("get_line returns 2");
			to_screen();
			*/
			return 2;
			break;
		case 8: 
		case 263:
			/* Delete character */
			if(vl != 0 && ed_win->buf[vl] == 0)
			{
				/* Last character of the string */
				move(scry, --scrx);
				vl --;
				printw(" ");
				move(scry, scrx);
				ed_win->buf[vl] = 0;
			}
			else if(vl)
			{
				/* Remove character inside the string */
				int i;
				move(scry, scrx-1);
				vl -- ;
				for(i = vl; i < strlen(ed_win->buf); i++)
				{
					if(ed_win->buf[i] = ed_win->buf[i+1])
						printw("%c", ed_win->invisible ? '*' : ed_win->buf[i]);
					else
						printw(" ");
				}
				move(scry, -- scrx);
			}
			break;
		default:
			/* Symbol character */
			if(c < ' ' || c > 255) 
			{
				beep();
				break;
			}
			/* Check for correctness of inputed character */
			if(ed_win->check_proc)
			{
				int res;
				res = (*(ed_win->check_proc))(ed_win, CHECK_LETTER, vl);
				c = ed_win->last_char;
				if(res == 0) 
					continue;;
			}
			/* print character to the screen */
			if(ed_win->invisible)
				printw("*"); /* Secured input */
			else
				printw("%c", c);
			/* Add character to the target buffer */
			if(ed_win->buf[vl] == 0)
			{
				ed_win->buf[vl] = c;
				ed_win->buf[++vl] = 0;
			}
			else
				ed_win->buf[vl++] = c;
			/* Terminte input if user reached the limit */
			if(vl >= ed_win->bufsize)
			{
				done = 1;
				break;
			}
			++scrx;
			break;
		} /* switch */
		if(done && ed_win->check_proc)
		{
			int res;
			res = (*(ed_win->check_proc))(ed_win, CHECK_STRING, 0);
			if(!res)
				c = -1;
			else 
				clear_error(ed_win);
		}
		refresh();
	}
	return c;
}

/* Find editor window by its buffer*/
edit_window *fdmc_wifind(char *buf, edit_window *root)
{
	while(root->buf != buf && root->buf)
		root ++;
	if(root->buf)
		return root;
	return NULL;
}

//
int fdmc_wiupdate(edit_window *ed_win, char *buf, char *newvalue)
{
	edit_window *f;
	f = find_edit_window(buf, ed_win);
	if(f)
	{
		if(newvalue)
		{
			if(strlen(newvalue) > f->bufsize)
				newvalue[f->bufsize - 1] = 0;
			strcpy(f->buf, newvalue);
		}
		draw_edit_window(f);
		return 1;
	}
	return 0;
}

/* Draw the whole editor window */
int fdmc_widraw(edit_window *ed_win)
{
	int bufx;
	int cx, cy;

	bufx = ed_win->tx;
	attrset(ed_win->prompt_attributes);
	move(ed_win->y, ed_win->x);
	printw("%s", ed_win->prompt);
	refresh();
	attrset(ed_win->buf_attributes);
	if(!ed_win->invisible)
	{
		move(ed_win->ty, ed_win->tx);
		printw("%-*s", ed_win->bufsize, ed_win->buf);
		refresh();
	}
	else
	{
		int h;
		move(ed_win->ty, ed_win->tx);
		for(h = 0; h < ed_win->bufsize; h ++)
			if( h < strlen(ed_win->buf))
				printw("%c", '*');
			else
				printw(" ");
	}
	move(scry, scrx);
	refresh();
}

/* 
 * Perform the loop of the editing process 
 * in the editor element 
 */
int fdmc_wiuse(edit_window *ed_win)
{
	int i;
	/* Go to the beginning of editor window */
	attrset(ed_win->buf_attributes);
	scrx = scry = 1;
	move(ed_win->ty, ed_win->tx);
	refresh();
	/* Read the string from the input */
	i = fdmc_wiget_line(ed_win);
	/*
	to_file();
	trace("get_line returns %d", i);
	to_screen();
	*/
	/* Restore the screen (it is nessesary for some curses versions) */
	if(!ed_win->invisible)
		mvprintw(ed_win->ty, ed_win->tx, "%-*s", 
			ed_win->bufsize, ed_win->invisible ? " " : ed_win->buf);
	else
	{
		int h;
		move(ed_win->ty, ed_win->tx);
		for(h = 0; h < ed_win->bufsize; h ++)
			if( h < strlen(ed_win->buf))
				printw("%c", '*');
			else
				printw(" ");
	}
	refresh();
	return i;
}

/* Perform the main loop of the editor window
 */
int fdmc_wiedit_form(edit_window *ed_win)
{
	edit_window *root;
	int res;

	/* Display editors */
	root = ed_win;
	while(root->buf)
	{
		draw_edit_window(root);
		root ++ ;
	}
	
	root = ed_win;
	/* Use edit windows in loop */
	while(root->buf)
	{
		int exitc;
		exitc = use_edit_window(root);
		swchar:
		switch(exitc)
		{
			case KEY_UP:
				if(root != ed_win)
					root --;
				if(root->disabled)
					goto swchar;
				break;
			case KEY_DOWN:
			case '\t':
				if((root + 1)->buf != NULL)
					root ++ ;
				else
					root = ed_win;
				if(root->disabled)
					goto swchar;
				break;
			case -1:
				beep();
				continue;
			case 2:
				return 0;
			default:
				root ++ ;
				if(root->disabled)
					goto swchar;
				break;
		}
	}
	return 1;
}

void fdmc_wizero_vars(edit_window *ed_win)
{
	while(ed_win->buf)
	{
		ed_win->buf[0] = 0;
		ed_win ++ ;
	}
}
/* ================================================================== */

/* Got a virtual char from the keyboard */
int hold_escape_sequence(escape_sequence *sequence)
{
	int c;
	escape_sequence *s;
	
	s = sequence;
	if ( !s )
		return 0;
	c = getch();
	while(s->escape_key && s->escape_key != c)
	{
		s++;
	}
	if(!s->escape_key)
	{
		return 0;
	}
	if(s->escape_length == 1)
	{
		return s->return_code;
	}
	else
	{
		return hold_escape_sequence(s->next);
	}
	return 0;
}

/*
 *-------------------------------------------------------------------------
 * Function Name: check_for_amount
 *-------------------------------------------------------------------------
 * Designer(s):  Serge A. Borisov
 *
 *
 * Purpose:      This functons checks the correctness of inputed characters
 *               and the whole string of authorization amount
 *
 * Return Values:
 *              1 - if successfull verification
 *              0 - Verification failed
 *
 *-------------------------------------------------------------------------
 */
static int check_for_amount(edit_window *ed_win, int mode, int pos)
{
	FUNC_NAME("check_for_amount");
	double amt=0;
	int n;
	
	to_screen();
	switch(mode)
	{
		case CHECK_STRING: /* Verify all string */
			/* Get the position of the first significant digit
			 */
			n = strspn(ed_win->buf, " ");
			/* Get the result of conversion of the string to the double
			 */
			if(!sscanf((ed_win->buf)+n, "%lf", &amt) || strlen(ed_win->buf+n) > 12)
			{
				display_error(ed_win, "Invalid amount string '%s'", ed_win->buf+n);
				return 0;
			}
			sprintf(ed_win->buf, "%*.2lf", ed_win->bufsize, amt);
			/* Reject zero amount */
			if(amt == 0.0)
			{
				display_error(ed_win, "Amount must be not zero");
				return 0;
			}
			return 1;
		case CHECK_LETTER: /* Verify the letter */
			/* Letter must be a digit */
			if(isdigit(ed_win->last_char))
			{
				/* Reinput */
				if(pos == 0)
				{
					amount[0] = 0;
					draw_edit_window(ed_win);
				}
				return 1;
			}
			/* If user inputs decimal point confirm input
			 * if there is no decimal point in the string
			 */
			else if(ed_win->last_char == '.')
			{
				char *p;
				p = strchr(ed_win->buf, '.');
				return p == NULL;
			}
			else
				return 0;
			break;
	}
}
