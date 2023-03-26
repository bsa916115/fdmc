#ifndef _edit_utils_inc
#define _edit_utils_inc

typedef struct ew
{
	char *buf; /* Character buffer to hold result */
	int y;     /* y coordinate of prompt text */
	int x;     /* x coordinate of prompt text */
	int ty;    /* y coordinate of input line */
	int tx;    /* x coordinate of input line */
	char prompt[81];        /* text of the prompt */
	int prompt_attributes;  /* attributes of the prompt */
	int buf_attributes;     /* attributes of the input text line */
	int bufsize;            /* size of the input buffer */
	int invisible;          /* Password mode switch */
	int (*check_proc)(struct ew *ed_win, int mode, int pos); /* Procedure for input checking */
	int last_char; /* Last character entered */
	int disabled;
} edit_window, FDMC_EDIT_WINDOW;


/* Structure designed to process escape 
 * sequences from the keyboard
 */		
typedef struct escape_struct
{
	char escape_key;
	int escape_length;
	struct escape_struct *next;
	int return_code;
} escape_sequence;

// Common functions
// Print text aligned by center
void fdmc_wiprintcenter(int ky, char *fmt, ...);
// Display text at the error line
int fdmc_widisplay_error(edit_window *ed_win, char *fmt, ...);
// Print empty error line
void fdmc_wiclear_error(edit_window *ed_win);
int fdmc_wiget_line(edit_window *ed_win);
int fdmc_widraw(edit_window *ed_win);
int fdmc_wiuse(edit_window *ed_win);
int fdmc_wiedit_form(edit_window *ed_win);
edit_window *fdmc_wifind(char *buf, edit_window *root);
int fdmc_wiescape_sequence(escape_sequence *sequence);
void fdmc_wizero_form_vars(edit_window *ed_win);
int fdmc_wiupdate_field(edit_window *ed_win, char *buf, char *newvalue);

//
int fdmc_wiamount(edit_window *ed_win, int mode, int pos);
int fdmc_wiinteger(edit_window *ed_win, int mode, int pos );
int fdmc_wipan(edit_window *ed_win, int mode, int pos);
int fdmc_wiok(edit_window *ed_win, int mode, int pos);
int hold_escape_sequence(escape_sequence *sequence);

#define CHECK_STRING 0
#define CHECK_LETTER 1

#define ERR_LINE 23

#endif
