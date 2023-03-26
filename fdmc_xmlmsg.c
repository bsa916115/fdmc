#include "fdmc_global.h"
#include "fdmc_xmlmsg.h"
#include "fdmc_list.h"
#include "fdmc_logfile.h"
#include "fdmc_hash.h"

#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

MODULE_NAME("fdmc_xmlmsg.c");
#define dbg_trace dbg_print
static int xml_build_lexeme_table(void);
static int export_char(char *buffer, char *result);

static XML_LEXEME xml_lexemes_list[] =
{
	{"<", XML_OPEN_TAG},
	{"</", XML_OPEN_CLOSETAG},
	{"<!--", XML_OPEN_COMMENT},
	{"<?", XML_OPEN_INSTRUCTION},
	{"<!DOCTYPE", XML_OPEN_DESCRIPTION},
	{"<!ELEMENT", XML_OPEN_ELEMENT},
	{"<![CDATA[", XML_OPEN_CDATA},
	{">", XML_CLOSE_TAG},
	{"/>", XML_CLOSE_EMPTYTAG},
	{"-->", XML_CLOSE_COMMENT},
	{"?>", XML_CLOSE_INSTRUCTION},
	{"]>", XML_CLOSE_DESCRIPTION},
	{"]]>", XML_CLOSE_CDATA},
	{"\"", XML_QUOTA},
	{"&", XML_AMPERSAND},
	{";", XML_SEMICOLON},
	{"=", XML_EQUAL},
	{"'", XML_APOSTROPH},
	{"&#", XML_NUMBER},
	{"&gt;", XML_GT},
	{"&lt;", XML_LT},
	{"&amp;", XML_AMP},
	{"&quot;", XML_QUOT},
	{"&apos;", XML_APOS},
	{"\0", XML_STREAM_END}
};

static char xml_syntax_literals[] = "<>/-?\"]';&=";
static char xml_blanks[] = "\t\n\r ";
static char xml_lexemes[1024];
static FDMC_HASH_TABLE *xml_lexeme_table = NULL;

/********************************************************************
*                      Вспомогательные функции
*********************************************************************/

/* Проверить, входит ли символ в подмножество используемых
в синтаксисе языка */
static int is_syntax(char ch)
{
	//FUNC_NAME("is_syntax");
	if(ch == 0)
		return 0;
	return strchr(xml_syntax_literals, ch) != NULL;
}

/* Проверить, входит ли символ в подмножество символов разрыва */
static int is_blank(char ch)
{
	//FUNC_NAME("is_blank");
	if(ch == 0)
		return 0;
	return strchr(xml_blanks, ch) != NULL;
}

static int is_text(char ch)
{
	//FUNC_NAME("is_text");
	return !is_syntax(ch) && !is_blank(ch) && ch != 0;
}

/* Проверить входит ли строка в список лексем */
static int in_lexemes(char *token)
{
	//FUNC_NAME("in_lexemes");
	char buf[129];

	if(token[0] == 0)
		return 0;
	strcpy(buf, " ");
	strcat(buf, token);

	return strstr(xml_lexemes, buf) != NULL;
}

/* Проверить, является ли последовательность символов лексемой XML*/
static XML_LEXTYPE get_lexeme_type(char *text)
{
	//FUNC_NAME("get_lexeme");
	XML_LEXEME *fx, fs;

	if(text == NULL)
	{
		return XML_STREAM_END;
	}
	if(text[0] == 0)
	{
		return XML_STREAM_END;
	}
	if(text[0] == ' ')
	{
		return XML_BLANK;
	}
	fs.lexeme = text;
	fx = fdmc_hash_entry_find(xml_lexeme_table, &fs, NULL);
	if(!fx)
	{
		return XML_TEXT;
	}
	return fx->lextype;
}

/* Проверить, корректно ли имя идентификатора */
int is_xml_identifier(char *id)
{
	//FUNC_NAME("is_xml_identifier");
	int i, len;

	len = (int)strlen(id);
	for(i = 0; i < len; i++)
	{
		if(i == 0 && !(isalpha(id[i]) || id[i] == '_' ))
		{
			return 0;
		}
		else if(i != 0 && !(isalnum(id[i]) || id[i] == ':' || id[i] == '_'))
		{
			return 0;
		}
	}
	return 1;
}

/********************************************************************/

/* ******************************************************************
*          XML hash table functions
*********************************************************************/

static int lexeme_hash_function(FDMC_HASH_TABLE *h, void *data)
{
	//FUNC_NAME("lexeme_hash_function");

//	dbg_trace();
	return fdmc_hash_proc(h, data);
}

/* Функция для работы со списком хэш таблицы лексем*/
static int lexeme_list_processor(FDMC_LIST_ENTRY *lst, void *data, int flag)
{
	//FUNC_NAME("lexeme_list_processor");
	XML_LEXEME *lexx=(XML_LEXEME*)data, *lstlx;
	int res;

//	dbg_trace();
	switch(flag)
	{
	case FDMC_COMPARE:
		lstlx = lst->data;
		res = (int)strcmp(lstlx->lexeme, lexx->lexeme);
		break;
	case FDMC_INSERT:
		res = FDMC_OK;
		break;
	}
	return res;
}

/* Функция инициализации хэш-таблицы лексем*/
static int xml_build_lexeme_table()
{
	FUNC_NAME("build_lexeme_table");
	int i, res;

	//dbg_trace();
	if(xml_lexeme_table != NULL)
	{
		return 1;
	}
	xml_lexeme_table = fdmc_hash_table_create(1024,
		lexeme_hash_function, lexeme_list_processor, NULL);
	if(xml_lexeme_table == NULL)
	{
		trace("Cannot create hash table");
		return 0;
	}
	strcpy(xml_lexemes, " ");
	for(i = 0; xml_lexemes_list[i].lexeme[0] != 0; i++)
	{
		res = fdmc_hash_entry_add(xml_lexeme_table, &xml_lexemes_list[i], NULL);
		if(res == 0)
		{
			trace("Cannot add lexeme into hash table");
			return 0;
		}
		strcat(xml_lexemes, xml_lexemes_list[i].lexeme);
		strcat(xml_lexemes, " ");
	}
	//tprintf("FDML xml lexemes: %s\n", xml_lexemes);
	return 1;
}

/**************************************************************************/

/***************************************************************************
*                 XML Tree functions
***************************************************************************/

//---------------------------------------------------------
//  name: xml_node_processor
//---------------------------------------------------------
//  purpose: List porcessor for xml tree
//  designer: Serge Borisov (BSA)
//  started: 18.11.2010
//	parameters:
//		list_entry - entry of list
//		data - data to process
//		flag - function indicator
//	return:
//		result of operation
//---------------------------------------------------------
static int xml_node_processor(FDMC_LIST_ENTRY *list_entry, void *data,
										   int flag)
{
	//FUNC_NAME("xml_node_processor");
	XML_TREE_NODE *xml_node_data, *xml_node_list;
	int ret = FDMC_FAIL;

//	dbg_trace();
	switch(flag)
	{
	case FDMC_INSERT:
		ret = FDMC_OK;
		break;
	case FDMC_DELETE:
		//free(list_entry->data);
		ret = FDMC_OK;
		break;
	case FDMC_COMPARE:
		xml_node_data = data;
		xml_node_list = list_entry->data;
		ret = strcmp(xml_node_data->name, xml_node_list->name);
		break;
	}
	return ret;
}

/* Выделить память под новый элемент дерева xml */
//---------------------------------------------------------
//  name: create_xml_node
//---------------------------------------------------------
//  purpose: allocate and initialize memory for new node
//  designer: Serge Borisov (BSA)
//  started: 18.11.10
//	parameters: none
//	return:
//		On Success - pointer to new node
//		On Failure - NULL
//---------------------------------------------------------
static XML_TREE_NODE *create_xml_node(void)
{
	//FUNC_NAME("create_xml_node");
	XML_TREE_NODE *newnode;

	/* Выделить память */
	newnode = malloc(sizeof(*newnode));
	if(newnode == NULL)
	{
		return NULL;
	}
	/* Инициализировать новый элемент */
	memset(newnode, 0, sizeof(*newnode));
	return newnode;
}

//---------------------------------------------------------
//  name: fdmc_free_xml_tree
//---------------------------------------------------------
//  purpose: free memory allocated for xml tree
//  designer: Serge Borisov (BSA)
//  started: 18.11.10
//	parameters:
//		root - pointer to xml tree
//	return:
//		On Success - true
//		On Failure - false
//---------------------------------------------------------
int fdmc_free_xml_tree(XML_TREE_NODE *root)
{
	//FUNC_NAME("fdmc_free_xml_tree");
	FDMC_LIST_ENTRY *xml_entry, *lentry;
	XML_TREE_NODE *currnode;

	//dbg_trace();
	if(root == NULL)
	{
		return 1;
	}
	fdmc_free(root->name, NULL);
	fdmc_free(root->value, NULL);
	xml_entry = root->child->first;
	while(xml_entry != NULL)
	{
		currnode = xml_entry->data;
		if(currnode->nodetype == XML_TAG || currnode->nodetype == XML_EMPTYTAG)
		{
			fdmc_free_xml_tree(currnode);
		}
		else
		{
			if(currnode->name != NULL)
			{
				free(currnode->name);
				currnode->name = NULL;
			}
			if(currnode->value != NULL)
			{
				free(currnode->value);
				currnode->value = NULL;
			}
		}
		lentry = xml_entry;
		xml_entry = xml_entry->next;
		free(lentry);
	}
	free(root->child);
	free(root);
	return 1;
}

							


/**************************************************************************/

/**************************************************************************
*                      Работа с буфером XML текста
***************************************************************************/


//---------------------------------------------------------
//  name: fdmc_file_init_parser
//---------------------------------------------------------
//  purpose: initialize buffer to parse xml file
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		filename - name of file with xml code
//		err - error handler
//	return:
//		On Success - New buffer
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_PARSER_BUFFER *fdmc_file_init_parser(char *filename, FDMC_EXCEPTION *err)
{
	FUNC_NAME("file_init_parser");
	XML_PARSER_BUFFER *pbuf = NULL;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(filename, "filename", x);
		pbuf = fdmc_malloc(sizeof(*pbuf), &x);
		pbuf->bufsource = fopen(filename, "r");
		if(pbuf->bufsource == NULL)
		{
			fdmc_raisef(FDMC_IO_ERROR, &x, "Cannot open file '%s', error %s", filename, strerror(errno));
		}
		pbuf->buffer = fdmc_malloc(XML_TOKENMAX, &x);
		pbuf->bufmax = -1;
		pbuf->bufsize = XML_TOKENMAX;
		pbuf->lineno = pbuf->lastlineno = 1;
		pbuf->linepos = pbuf->lastlinepos = 1;
		pbuf->lastlextype = XML_STREAM_END;
	}
	EXCEPTION
	{
		if(pbuf) fdmc_free(pbuf->buffer, NULL);
		fdmc_free(pbuf, NULL);
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return pbuf;
}

//---------------------------------------------------------
//  name: fdmc_buffer_init_parser
//---------------------------------------------------------
//  purpose: parse xml buffer
//  designer: Serge Borisov (BSA)
//  started: 19/07/10
//	parameters:
//		buffer - pointer to xml code
//		bufsize - size of code buffer
//		err - error handler
//	return:
//		On Success - new parser buffer
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_PARSER_BUFFER* buffer_init_parser(char *buffer, int bufsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("buffer_init_parser");
	XML_PARSER_BUFFER *pbuf;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		pbuf = malloc(sizeof(*pbuf));
		if(pbuf == NULL)
		{
			trace("cannot allocate memory for xml buffer");
			return NULL;
		}
		memset(pbuf, 0, sizeof(*pbuf));
		pbuf->bufsource = NULL;
		pbuf->buffer = buffer;
		pbuf->buftotal = 0;
		pbuf->bufpos = 0;
		pbuf->bufmax = bufsize-1;
		pbuf->buftotal = 0;
		pbuf->bufsize = bufsize;
		pbuf->lastch = 0;
		pbuf->lineno = 1;
		pbuf->linepos = 1;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}

	return pbuf;
}

//---------------------------------------------------------
//  name: fdmc_close_parser
//---------------------------------------------------------
//  purpose: delete xml parser from memory
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		pbuf - parser buffer
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//
//---------------------------------------------------------
int file_close_parser(XML_PARSER_BUFFER *pbuf)
{
	//FUNC_NAME("file_close_parser");

	if(!pbuf) return 0;
	if(pbuf->bufsource)
	{
		fclose(pbuf->bufsource);
		pbuf->bufsource = NULL;
	}
	return 1;
}

/* Распечатать координаты текущего символа */
static void parser_print_location(XML_PARSER_BUFFER *pbuf)
{
	//FUNC_NAME("parser_print_location");

	trace("Line %d Col %d\n", pbuf->lastlineno, pbuf->lastlinepos);
	return;
}
/* Считать символ из входного потока */
static char parser_getch(XML_PARSER_BUFFER *buf)
{
	//FUNC_NAME("parser_getch");

	if(buf->bufpos > buf->bufmax)
	{
		/* Достигнут конец данных в памяти, произвести подкачку из файла, 
		если он открыт, или вернуть признак конца данных */
		if(buf->bufsource)
		{
			buf->bufmax = (int)fread(buf->buffer, 1, buf->bufsize, buf->bufsource) - 1;
			if(buf->bufmax < 0)
			{
				/* Достигнут конец файла */
				fclose(buf->bufsource);
				buf->bufsource = NULL;
				return buf->lastch = 0;
			}
			/* Установить указатель на начало буфера */
			buf->bufpos = 0;
		}
		else
		{
			return buf->lastch = 0;
		}
	}
	/* Считать символ и инкрементировать указатель */
	buf->buftotal ++;
	buf->linepos ++;
	buf->lastch = buf->buffer[buf->bufpos++];
	if(buf->lastch == '\n')
	{
		/* New line in text source */
		buf->lineno ++;
		buf->linepos = 1;
	}
	return buf->lastch;
}

/* Выбрать часть текта, подлежащую обработке*/
static XML_LEXTYPE parser_get_lex(XML_PARSER_BUFFER *pbuf, char *buf, int maxsize)
{
	FUNC_NAME("parser_get_lex");
	int bp;

	/* Достигнут конец файла */
	if(pbuf->lastch == 0)
	{
		buf[0] = 0;
		return pbuf->lastlextype = XML_STREAM_END;
	}
	bp = 0;
	buf[bp] = pbuf->lastch;
	buf[bp+1] = '\0';
	if(is_blank(pbuf->lastch))
	{
		/* Суммировать символы разрыва */
		while(is_blank(pbuf->lastch))
		{
			parser_getch(pbuf);
		}
		buf[0] = ' ';
		buf[1] = '\0';
		return pbuf->lastlextype = XML_BLANK;
	}
	else if(is_syntax(pbuf->lastch))
	{
		while(in_lexemes(buf) && !is_blank(pbuf->lastch) && pbuf->lastch)
		{
			bp++;
			buf[bp] = parser_getch(pbuf);
			buf[bp+1] = 0;
			/* Проверить переполнение строки */
			if(bp >= maxsize)
			{
				tprintf("%s Too big input string\n", _function_id);
				return pbuf->lastlextype = XML_STREAM_END;
			}
		}
		buf[bp] = 0;
	}
	else
	{
		while(is_text(pbuf->lastch))
		{
			bp++;
			buf[bp] = parser_getch(pbuf);
			buf[bp+1] = 0;
			/* Проверить переполнение строки */
			if(bp >= maxsize)
			{
				tprintf("%s Too big input string\n", _function_id);
				return pbuf->lastlextype = XML_STREAM_END;
			}
		}
		buf[bp] = 0;
	}
	pbuf->lastlex = buf;
	return pbuf->lastlextype = get_lexeme_type(buf);
}

/* Получиь лексему не пробел (valuable token) */
static XML_LEXTYPE parser_get_vlex(XML_PARSER_BUFFER *pbuf, char *buf, int bufsize)
{
	//FUNC_NAME("parser_get_vlex");
	XML_LEXTYPE ret;

	while((ret = parser_get_lex(pbuf, buf, bufsize)) == XML_BLANK)
	{
		continue;
	}
	return ret;
}


/**************************************************************************/

/**************************************************************************
*                 Синтаксический разбор входного буфера
***************************************************************************/

/* Процессировать встроенные сущности */
static int fdmc_xml_parse_entities(XML_PARSER_BUFFER *pbuf, char *result, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_parse_entities");
	char *tokenbuf = pbuf->lastlex;
	int dg;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		*result = 0;
		if(tokenbuf[0] != '&')
			return 0;
		switch (pbuf->lastlextype)
		{
		case XML_APOS:
			strcpy(result, "'");
			break;
		case XML_AMP:
			strcpy(result, "&");
			break;
		case XML_QUOT:
			strcpy(result, "\"");
			break;
		case XML_GT:
			strcpy(result, ">");
			break;
		case XML_LT:
			strcpy(result, "<");
			break;
		case XML_NUMBER:
			/* Задано числовое представление символа */
			parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
			if(pbuf->lastlextype != XML_TEXT)
			{
				fdmc_raisef(FDMC_SYNTAX_ERROR, &x, "Character code expected");
				return 0;
			}
			/* Преобразовать симолы */
			if(sscanf(tokenbuf, "%d", &dg) == 0)
			{
				fdmc_raisef(FDMC_SYNTAX_ERROR, &x, 
					"Invalid character code '%s'", tokenbuf);
				return 0;
			}
			/* Код символа должен завершаться точкой с запятой */
			parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
			if(pbuf->lastlextype != XML_SEMICOLON)
			{
				fdmc_raisef(FDMC_SYNTAX_ERROR, &x, "Expected ';' here");
				return 0;
			}
			result[0] = (char)dg;
			result[1] = 0;
			break;
		}
	}
	EXCEPTION
	{
		if(x.errorcode == FDMC_SYNTAX_ERROR)
		{
			parser_print_location(pbuf);
		}
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: fdmc_xml_parse_attr
//---------------------------------------------------------
//  purpose: parse attribute of tag in xml parser buffer
//  designer: Serge Borisov (BSA)
//  started: 20/09/10
//	parameters:
//		pbuf - parser buffer
//		parent - parent tag of attribute
//		err - error handler
//	return:
//		On Success - new node
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_TREE_NODE *fdmc_xml_parse_attr(XML_PARSER_BUFFER *pbuf, XML_TREE_NODE *parent, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_parse_attr");
	FDMC_EXCEPTION attr_handler;
	XML_TREE_NODE *newnode=NULL, *ret;
	FDMC_LIST_ENTRY *atr;
	XML_LEXTYPE tok, exp_tok;
	char *tokenbuf;
	char *attrbuf = NULL;

	//dbg_trace();
	TRYF(attr_handler)
	{
		CHECK_NULL(pbuf, "pbuf", attr_handler);
		CHECK_NULL(parent, "parent", attr_handler);
		if(!is_xml_identifier(pbuf->lastlex))
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &attr_handler, "'%s' - bad identifier name", pbuf->lastlex);
		}
		if(pbuf == NULL || parent == NULL)
		{
			fdmc_raisef(FDMC_COMMON_ERROR, &attr_handler, "NULL buffer in parameters");
		}
		attrbuf = fdmc_malloc(XML_TOKENMAX, &attr_handler);
		newnode = fdmc_malloc(sizeof(*newnode), &attr_handler);
		/* Последняя лексема - это имя атрибута */
		newnode->name = fdmc_strdup(pbuf->lastlex, &attr_handler);
		newnode->nodetype = XML_ATTRIBUTE;
		/* Прежде чем прописываться в список атрибутов, нужно
		проверить нет ли там с таким же именем */
		atr = parent->child->first;
		while(atr)
		{
			ret = atr->data;
			if(ret->nodetype == XML_ATTRIBUTE && strcmp(ret->name, newnode->name) == 0)
			{
				fdmc_raisef(FDMC_SYNTAX_ERROR, &attr_handler, 
					"Duplicate attribute name '%s'", newnode->name);
			}
			atr = atr->next;
		}
		newnode->brother = fdmc_list_entry_create(newnode, &attr_handler);
		tokenbuf = pbuf->lastlex;
		tok = parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
		/* Должен быть знак равенства */
		if(tok != XML_EQUAL)
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &attr_handler, "Expected '='");
		}
		/* Далее обязательно или кавычка или апостроф */
		tok = parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
		if(tok == XML_QUOTA || tok == XML_APOSTROPH)
		{
			exp_tok = tok;
		}
		else
		{
			parser_print_location(pbuf);
			fdmc_raisef(FDMC_SYNTAX_ERROR, &attr_handler, 
				"Need ' or \" here for attribute value");
		}
		/* Далее следует значение в виде текста */
		attrbuf[0] = 0;
		tok = parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		while(tok != exp_tok && tok != XML_STREAM_END && tok != XML_OPEN_TAG)
		{
			/* Обработка встроенных сущностей */
			if(tokenbuf[0] == '&')
			{
				char buf[2];
				fdmc_xml_parse_entities(pbuf, buf, &attr_handler);
				strcpy(tokenbuf, buf);
			}
			strcat(attrbuf, tokenbuf);
			tok = parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		}
		if(tok != exp_tok)
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &attr_handler, "Expected %c here to close literal", 
				exp_tok == XML_APOSTROPH ? '\'':'"');
		}
		newnode->value = fdmc_strdup(attrbuf, &attr_handler);
		/* Добавить атрибут в список */
		fdmc_list_entry_add(parent->child, newnode->brother, NULL);
	}
	EXCEPTION
	{
		trace("\n");
		/* Освободить выделенную память */
		if(newnode != NULL)
		{
			fdmc_free(newnode->name, NULL);
			fdmc_free(newnode->value, NULL);
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_free(newnode, NULL);
		}
		newnode = NULL;
		if(attr_handler.errorcode == FDMC_SYNTAX_ERROR)
		{
			parser_print_location(pbuf);
		}
		fdmc_raiseup(&attr_handler, err);
	}
	fdmc_free(attrbuf, NULL);
	return newnode;
}


//---------------------------------------------------------
//  name: fdmc_xml_parse_cdata
//---------------------------------------------------------
//  purpose: parse CDATA lexeme in parser buffer
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		pbuf - paser buffer
//		parent - parent tag 
//		err - error handler
//	return:
//		On Success - new CDATA node
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_TREE_NODE *fdmc_xml_parse_cdata(XML_PARSER_BUFFER *pbuf, XML_TREE_NODE *parent, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_parse_cdata");
	FDMC_EXCEPTION cdata_handler;
	XML_TREE_NODE *newnode=NULL;
	XML_LEXTYPE tok;
	FDMC_LIST_ENTRY *ret;
	char *tokenbuf;
	char cdatabuf[XML_TEXTMAX];
	//char *attrbuf;

	/*dbg_trace();*/
	tokenbuf = pbuf->lastlex;
	TRYF(cdata_handler)
	{
		int bp = 0;
		CHECK_NULL(pbuf, "pbuf", cdata_handler);
		CHECK_NULL(parent, "parent", cdata_handler);
		cdatabuf[0] = 0;
		parser_getch(pbuf);
		while(pbuf->lastch != XML_STREAM_END)
		{
			if(bp >= XML_TEXTMAX)
			{
				/* Превышен максимальный размер буфера */
				fdmc_raisef(FDMC_SYNTAX_ERROR, &cdata_handler, "Too big CDATA source");
			}
			if(pbuf->lastch == ']')
			{
				tok = parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
				if(tok == XML_CLOSE_CDATA)
				{
					/* Блок завершен */
					break;
				}
				/* Добавить данные в буфер */
				bp += (int)strlen(tokenbuf);
				strcat(cdatabuf, tokenbuf);
			}
			cdatabuf[bp] = pbuf->lastch;
			cdatabuf[++bp] = 0;
			parser_getch(pbuf);
		}
		if(pbuf->lastch == 0)
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &cdata_handler,
				"Unexpected end of file, CDATA section not closed");
		}
		/* Создать новый элемент CDATA и добавить его в родительское дерево */
		newnode = fdmc_malloc(sizeof(newnode[0]), &cdata_handler);
		newnode->value = fdmc_strdup(cdatabuf, &cdata_handler);
		newnode->nodetype = XML_CDATA;
		newnode->brother = fdmc_list_entry_create(newnode, &cdata_handler);
		ret = fdmc_list_entry_add(parent->child, newnode->brother, &cdata_handler);
	}
	EXCEPTION
	{
		if(newnode != NULL)
		{
			fdmc_free(newnode->value, NULL);
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_free(newnode, NULL);
			newnode = NULL;
		}
		if(cdata_handler.errorcode == FDMC_SYNTAX_ERROR)
		{
			parser_print_location(pbuf);
		}
		fdmc_raiseup(&cdata_handler, err);
	}
	return newnode;
}


//---------------------------------------------------------
//  name: fdmc_xml_parse_comment
//---------------------------------------------------------
//  purpose: parse comment lexeme in xml paser buffer
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		pbuf - parser buffer
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_xml_parse_comment(XML_PARSER_BUFFER *pbuf, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_parse_comment");
	char *tokenbuf;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(pbuf, "pbuf", x);
		tokenbuf = pbuf->lastlex;
		parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		while(pbuf->lastlextype != XML_STREAM_END)
		{
			if(pbuf->lastlextype == XML_CLOSE_COMMENT)
			{
				break;
			}
			parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		}
		if(pbuf->lastlextype == XML_STREAM_END)
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &x, "Unexpected end of input, comment not closed.");
		}
		parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		return 1;
	}
	EXCEPTION
	{
		if(x.errorcode == FDMC_SYNTAX_ERROR)
		{
			parser_print_location(pbuf);
		}
		fdmc_raiseup(&x, err);
	}
	return 0;
}

//---------------------------------------------------------
//  name: fdmc_xml_parse_pcdata
//---------------------------------------------------------
//  purpose: parse text data in xml parser buffer
//  designer: Serge Borisov (BSA)
//  started: 20/09/10
//	parameters:
//		pbuf - parser buffer
//		parent - tag owned the text
//		err - error handler
//	return:
//		On Success - new node
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_TREE_NODE *fdmc_xml_parse_pcdata(XML_PARSER_BUFFER *pbuf, XML_TREE_NODE *parent, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_parse_pcdata");
	char *tokenbuf;
	char textbuf[XML_TEXTMAX];
	FDMC_EXCEPTION text_handler;
	XML_TREE_NODE *newnode=NULL;

	TRYF(text_handler)
	{
		CHECK_NULL(pbuf, "pbuf", text_handler);
		CHECK_NULL(parent, "parent", text_handler);
		textbuf[0] = 0;
		/* Фиксация позиции буфера для вывода сообщений об ошибке */
		pbuf->lastlineno = pbuf->lineno;
		pbuf->lastlinepos = pbuf->linepos;
		tokenbuf = pbuf->lastlex;
		while(pbuf->lastlextype != XML_STREAM_END)
		{
			switch(pbuf->lastlextype)
			{
			case XML_OPEN_TAG:
			case XML_OPEN_CLOSETAG:
			case XML_OPEN_CDATA:
				/* Закончить обрабтку входного текста
				и возвратить созданный элемент данных*/
				newnode = fdmc_malloc(sizeof(newnode[0]), &text_handler);
				newnode->value = fdmc_strdup(textbuf, &text_handler);
				newnode->nodetype = XML_PCDATA;
				newnode->brother = fdmc_list_entry_create(newnode, &text_handler);
				fdmc_list_entry_add(parent->child, newnode->brother, &text_handler);
				return newnode;
			case XML_OPEN_COMMENT:
				/* Пропустить строку комментария */
				fdmc_xml_parse_comment(pbuf, &text_handler);
				break;
			/* Предопределенные сущности */
			default:
				if(tokenbuf[0] == '&')
				{
					char buf[2];
					fdmc_xml_parse_entities(pbuf, buf, &text_handler);
					strcpy(tokenbuf, buf);
				}
				break;
			}
			if(strlen(textbuf) + strlen(tokenbuf) >= XML_TEXTMAX)
			{
				fdmc_raisef(FDMC_SYNTAX_ERROR, &text_handler, "Too big text data");
			}
			strcat(textbuf, tokenbuf);
			parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		}
		if(pbuf->lastlextype == XML_STREAM_END)
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &text_handler, 
				"Unexpected end of file. Text data not closed");
		}
	}
	EXCEPTION
	{
		if(newnode != NULL)
		{
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_free(newnode->value, NULL);
			fdmc_free(newnode, NULL);
		}
		newnode = NULL;
		if(text_handler.errorcode == FDMC_SYNTAX_ERROR)
		{
			parser_print_location(pbuf);
		}
		fdmc_raiseup(&text_handler, err);
	}
	return newnode;
}


//---------------------------------------------------------
//  name: fdmc_xml_parse_tag
//---------------------------------------------------------
//  purpose: parse input buffer to create new node
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		pbuf - parser buffer
//		parent - pointer to parent node
//		err - error handler
//	return:
//		On Success - New node
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static XML_TREE_NODE* fdmc_xml_parse_tag(XML_PARSER_BUFFER *pbuf, XML_TREE_NODE *parent, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_parse_tag");
	XML_TREE_NODE *newnode=NULL, *ret;
	char *tokenbuf;
	char textbuf[XML_TEXTMAX];
	//int texbufsize = sizeof(textbuf);
	XML_LEXTYPE tokentype;
	FDMC_EXCEPTION tag_handler;

	textbuf[0] = 0;
	tokenbuf = pbuf->lastlex;
	TRYF(tag_handler)
	{
		CHECK_NULL(pbuf, "pbuf", tag_handler);
		/* Читать имя тега */
		tokentype = parser_get_lex(pbuf, tokenbuf, XML_TOKENMAX);
		/* Имя тега должно идти сразу за символом < */
		if(tokentype != XML_TEXT)
		{
			/* Имя тега не текст */
			fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler, "invalid tag name");
		}
		if(!is_xml_identifier(tokenbuf))
		{
			fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler,
				"'%s' - bad identifier name", tokenbuf);
		}
		newnode = fdmc_malloc(sizeof(*newnode), &tag_handler);
		newnode->child = fdmc_list_create(xml_node_processor, &tag_handler);
		//screen_trace("child created %lX", newnode->child);
		newnode->name = fdmc_strdup(tokenbuf, &tag_handler);
		newnode->nodetype = XML_TAG;
		//trace("<%s>:new node created", newnode->name);
		if(parent != NULL)
		{
			/* Мы не корневой элемент. Добавляем себя в список потомков */
			newnode->brother = fdmc_list_entry_create(newnode, &tag_handler);
			fdmc_list_entry_add(parent->child, newnode->brother, &tag_handler);
		}
		/* Читать из буфера лексемы, пока это название атрибута */
		parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
		while(pbuf->lastlextype == XML_TEXT)
		{
			/* Возможно это атрибут */
			/* Фиксация позиции буфера для вывода сообщений об ошибке */
			pbuf->lastlineno = pbuf->lineno;
			pbuf->lastlinepos = pbuf->linepos;
			ret = fdmc_xml_parse_attr(pbuf, newnode, &tag_handler);
			parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
		}
		switch(pbuf->lastlextype)
		{
		case XML_CLOSE_EMPTYTAG:
			newnode->nodetype = XML_EMPTYTAG;
			parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
			return newnode;
		case XML_CLOSE_TAG:
			break;
		default:
			parser_print_location(pbuf);
			fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler, 
				"Expected '>' or '/>' here");
		}
		parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
		while(pbuf->lastlextype != XML_STREAM_END)
		{
			/* Фиксация позиции буфера для вывода сообщений об ошибке */
			pbuf->lastlineno = pbuf->lineno;
			pbuf->lastlinepos = pbuf->linepos;
			switch(pbuf->lastlextype)
			{
			case XML_OPEN_TAG:
				/* Встретился символ открытия нового тэга. 
				Он будет вложенным для текущего */
				fdmc_xml_parse_tag(pbuf, newnode, &tag_handler);
				break;
			case XML_OPEN_CLOSETAG:
				/* Открытие закрывающего тега. После этой лексемы должно стоять
				точно такое же имя, как и в открывающем */
				tokentype = parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
				if(tokentype != XML_TEXT || strcmp(tokenbuf, newnode->name) != 0)
				{
					/* Это не текст, то есть не имя тэга, либо имена не совпадают */
					parser_print_location(pbuf);
					fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler, "Expected '</%s>' here",
						newnode->name);
				}
				/* Затем обязательно должна быть скобка закрытия тега */
				tokentype = parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
				if(tokentype != XML_CLOSE_TAG)
				{
					parser_print_location(pbuf);
					fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler, "Expected '>' here");
				}
				/* Процессирование тэга успешно завершено */
				parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
				return newnode;
			case XML_OPEN_CDATA:
				/* Обработать блок данных */
				fdmc_xml_parse_cdata(pbuf, newnode, &tag_handler);
				break;
			case XML_CLOSE_CDATA:
				parser_get_vlex(pbuf, tokenbuf, XML_TOKENMAX);
				break;
			case XML_OPEN_COMMENT:
				fdmc_xml_parse_comment(pbuf, &tag_handler);
				break;
			case XML_OPEN_INSTRUCTION:
			case XML_OPEN_DESCRIPTION:
			case XML_OPEN_ELEMENT:
				parser_print_location(pbuf);
				fdmc_raisef(FDMC_SYNTAX_ERROR, &tag_handler, 
					"'%s' is not allowed in this context", pbuf->lastlex);
				break;
			case XML_TEXT:
			default:
				fdmc_xml_parse_pcdata(pbuf, newnode, &tag_handler);
				break;
			}
		}
	}
	EXCEPTION
	{
		/* Освободить выделенную память */
		if(newnode)
		{
			fdmc_free(newnode->name, NULL);
			fdmc_free(newnode->value, NULL);
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_list_delete(newnode->child, NULL);
			fdmc_free(newnode, NULL);
		}
		newnode = NULL;
		fdmc_raiseup(&tag_handler, err);
	}
	return newnode;
}


/***********************************************************
*                     Экспортируемые функции
************************************************************/

//---------------------------------------------------------
//  name: fdmc_parse_xml_file
//---------------------------------------------------------
//  purpose: parse file with xml code and create xml tree
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		filename - name of xml file
//		err - error handler
//	return:
//		On Success - New node
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE* fdmc_parse_xml_file(char *filename, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_parse_xml_file");

	XML_PARSER_BUFFER *pbuf = NULL;
	FDMC_EXCEPTION x;
	char token_buf[XML_TOKENMAX];
	XML_LEXTYPE t;
	XML_TREE_NODE *root=NULL;

	TRYF(x)
	{
		CHECK_NULL(filename, "filename", x);
		/* Создать файловый буфер */
		/* Инициализировать входной поток для чтения из файла */
		pbuf = fdmc_file_init_parser(filename, &x);
		if(!xml_build_lexeme_table())
		{
			fdmc_raisef(FDMC_COMMON_ERROR, &x,	"Cannot build lexeme table");
		}
		parser_getch(pbuf);
		t = parser_get_lex(pbuf, token_buf, XML_TOKENMAX);
		while(t != XML_STREAM_END)
		{
			if(t == XML_OPEN_TAG)
			{
				root = fdmc_xml_parse_tag(pbuf, NULL, &x);
				break;
			}
			t = parser_get_lex(pbuf, token_buf, XML_TOKENMAX);
		}
	}
	EXCEPTION
	{
		if(pbuf != NULL)
		{
			file_close_parser(pbuf);
			fdmc_free(pbuf->buffer, NULL);
			free(pbuf);
		}
		if(root != NULL)
		{
			fdmc_free_xml_tree(root);
		}
		root = NULL;
		fdmc_raiseup(&x, err);
		return root;
	}
	fdmc_free(pbuf->buffer, NULL);
	fdmc_free(pbuf, NULL);
	return root;
}

//---------------------------------------------------------
//  name: fdmc_parse_xml_buffer
//---------------------------------------------------------
//  purpose: create and build xml tree from xml data buffer
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		buffer - xml data buffer
//		bufsize - size of buffer
//		err - error handler
//	return:
//		On Success - new xml tree root node
//		On Failure - null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_parse_xml_buffer(void *buffer, int bufsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_parse_xml_buffer");
	FDMC_EXCEPTION parser_exception;
	XML_PARSER_BUFFER *xbuffer;
	XML_TREE_NODE *root=NULL;
	XML_LEXTYPE t;
	char token_buf[XML_TOKENMAX];

	TRYF(parser_exception)
	{
		CHECK_NULL(buffer, "buffer", parser_exception);
		xbuffer = buffer_init_parser(buffer, bufsize, &parser_exception);
		if(!xml_build_lexeme_table())
		{
			fdmc_raisef(FDMC_COMMON_ERROR, &parser_exception,
				"Cannot build lexeme table");
		}
		parser_getch(xbuffer);
		t = parser_get_lex(xbuffer, token_buf, XML_TOKENMAX);
		while(t != XML_STREAM_END)
		{
			if(t == XML_OPEN_TAG)
			{
				root = fdmc_xml_parse_tag(xbuffer, NULL, &parser_exception);
				break;
			}
			t = parser_get_lex(xbuffer, token_buf, XML_TOKENMAX);
		}
		return root;
	}
	EXCEPTION
	{
		if(xbuffer != NULL)
		{
			free(xbuffer);
		}
		if(root != NULL)
		{
			fdmc_free_xml_tree(root);
		}
		root = NULL;
		func_trace("Input buffer was not processed");
		fdmc_raiseup(&parser_exception, err);
	}
	return NULL;
}


//---------------------------------------------------------
//  name: fdmc_xml_add_tag
//---------------------------------------------------------
//  purpose: add new tag to xml tree
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		ex - error handler
//	return:
//		On Success - new node
//		On Failure - null
//  special features:
//		if ex is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_add_tag(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION *ex)
{
	FUNC_NAME("fdmc_xml_add_tag");
	XML_TREE_NODE *newnode = NULL;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		CHECK_NULL(name, "name", x);
		newnode = fdmc_malloc(sizeof(XML_TREE_NODE), &x);
		newnode->nodetype = XML_TAG;
		newnode->name = fdmc_strdup(name, &x);
		newnode->child = fdmc_list_create(xml_node_processor, &x);
		newnode->brother = fdmc_list_entry_create(newnode, &x);
		fdmc_list_entry_add(root->child, newnode->brother, &x);
	}
	EXCEPTION
	{
		if(newnode)
		{
			fdmc_free(newnode->name, NULL);
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_list_delete(newnode->child, NULL);
			fdmc_free(newnode, NULL);
		}
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return newnode;
}

//---------------------------------------------------------
//  name: fdmc_xml_add_attr
//---------------------------------------------------------
//  purpose: add attribute node to tag node
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - tag node
//		name - name for attribute
//		value - value for attribute
//		ex - error handler
//	return:
//		On Success - root node
//		On Failure - null
//  special features:
//		if ex is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_add_attr(XML_TREE_NODE *root, char *name, char *value, FDMC_EXCEPTION *ex)
{
	FUNC_NAME("fdmc_xml_add_attr");
	XML_TREE_NODE *newnode;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		CHECK_NULL(name, "name", x);
		CHECK_NULL(value, "value", x);
		newnode = fdmc_malloc(sizeof(XML_TREE_NODE), &x);
		newnode->nodetype = XML_ATTRIBUTE;
		newnode->name = fdmc_strdup(name, &x);
		newnode->value = fdmc_strdup(value, &x);
		newnode->brother = fdmc_list_entry_create(newnode, &x);
		fdmc_list_entry_add(root->child, newnode->brother, &x);
	}
	EXCEPTION
	{
		if(newnode)
		{
			fdmc_free(newnode->name, NULL);
			fdmc_free(newnode->value, NULL);
			fdmc_list_entry_delete(newnode->brother, NULL);
			fdmc_free(newnode, NULL);
		}
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return root;
}

//---------------------------------------------------------
//  name: fdmc_xml_add_attrf
//---------------------------------------------------------
//  purpose: add new attribute to tag using printf style
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		err - error handler
//	return:
//		On Success - root node
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_add_attrf(XML_TREE_NODE *root, FDMC_EXCEPTION *ex, char *name, char *fmt, ...)
{
	FUNC_NAME("fdmc_xml_add_attrf");
	char buf[1024];
	va_list argptr;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(fmt, "fmt", x);
		va_start(argptr, fmt);
		vsprintf(buf, fmt, argptr);
		va_end(argptr);
		fdmc_xml_add_attr(root, name, buf, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return root;
}

//---------------------------------------------------------
//  name: fdmc_xml_add_text
//---------------------------------------------------------
//  purpose: add new text node to tag
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - xml tag node
//		text - text to add
//		ex - error handler
//	return:
//		On Success - root node
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_add_text(XML_TREE_NODE *root, char *text, FDMC_EXCEPTION *ex)
{
	FUNC_NAME("fdmc_xml_add_text");
	XML_TREE_NODE *newnode;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		CHECK_NULL(text, "text", x);
		newnode = fdmc_malloc(sizeof(XML_TREE_NODE), &x);
		newnode->nodetype = XML_PCDATA;
		newnode->value = fdmc_strdup(text, &x);
		newnode->brother = fdmc_list_entry_create(newnode, &x);
		fdmc_list_entry_add(root->child, newnode->brother, &x);
	}
	EXCEPTION
	{
		if(newnode)
		{
			fdmc_free(newnode->value, NULL);
			fdmc_free(newnode->brother, NULL);
			fdmc_free(newnode, NULL);
		}
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return root;
}

//---------------------------------------------------------
//  name: fdmc_xml_add_textf
//---------------------------------------------------------
//  purpose: add new text node to tag using printf style
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		err - error handler
//	return:
//		On Success - root node
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_add_textf(XML_TREE_NODE *root, FDMC_EXCEPTION *ex, char *fmt, ...)
{
	FUNC_NAME("fdmc_xml_add_textf");
	char buf[1024];
	va_list argptr;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(fmt, "fmt", x);
		va_start(argptr, fmt);
		vsprintf(buf, fmt, argptr);
		va_end(argptr);
		fdmc_xml_add_text(root, buf, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, ex);
		return NULL;
	}
	return root;
}


//---------------------------------------------------------
//  name: fdmc_xml_find_tag
//---------------------------------------------------------
//  purpose: find nested tag with specified name
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - Tag where to search
//		name - name of tag to search
//		err - error handler
//	return:
//		On Success - Tree node for tag
//		On Failure - Null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_find_tag(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_find_tag");
	FDMC_LIST_ENTRY *xml_entry;
	XML_TREE_NODE *xml_node;
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		xml_entry = root->child->first;
		while(xml_entry)
		{
			xml_node = xml_entry->data;
			if(xml_node->nodetype == XML_TAG || xml_node->nodetype == XML_EMPTYTAG)
			{
				if(!strcmp(xml_node->name, name))
				{
					return xml_node;
				}
			}
			xml_entry = xml_entry->next;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_xml_find_attr
//---------------------------------------------------------
//  purpose: find attribute with specified name for tag
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		root - Tag to search for
//		name - Name of attribute
//		err - error handler
//	return:
//		On Success - tree node for attr
//		On Failure - Null
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_find_attr(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION *err)
{
	FUNC_NAME("xml_find_attr");
	FDMC_LIST_ENTRY *child_node;
	XML_TREE_NODE *xml_node;
	FDMC_EXCEPTION x;
	
	TRYF(x)
	{
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		if(root == NULL || root->child == NULL)
		{
			return NULL;
		}
		child_node = root->child->first;
		while(child_node)
		{
			xml_node = child_node->data;
			if(xml_node->nodetype != XML_ATTRIBUTE)
			{
				return NULL;
			}
			if(!strcmp(xml_node->name, name))
			{
				return xml_node;
			}
			child_node = child_node->next;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: xml_find_tag
//---------------------------------------------------------
//  purpose: recursive pad for fdmc_xml_find_node
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		root - node to search from
//		name - node to search for
//		count - recursive counter
//	return:
//		On Success - Node
//		On Failure - NULL
//  special features:
//---------------------------------------------------------
static XML_TREE_NODE *xml_find_tag(XML_TREE_NODE *root, char *name, int *count)
{
//	FUNC_NAME("xml_find_tag");
	FDMC_LIST_ENTRY *xml_entry;
	XML_TREE_NODE *xml_node;

	//dbg_trace();
	if(strcmp(root->name, name) == 0)
	{
		(*count) --;
		if(*count == 0)
		{
			return root;
		}
	}
	if(!root->child) 
	{
		return NULL;
	}
	xml_entry = root->child->first;
	while(xml_entry)
	{
		xml_node = xml_entry->data;
		if(xml_node->nodetype == XML_TAG || xml_node->nodetype == XML_EMPTYTAG)
		{
			xml_node = xml_find_tag(xml_node, name, count);
			if(xml_node != NULL)
			{
				return xml_node;
			}
		}
		xml_entry = xml_entry->next;
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_xml_find_node
//---------------------------------------------------------
//  purpose: find tag with name in all tree start from root
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - node to search from
//		name - node to search for
//		count - occurance number of node with name
//		err - error handler
//	return:
//		On Success - Result Node 
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
XML_TREE_NODE *fdmc_xml_find_node(XML_TREE_NODE *root, char *name, int count, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_find_node");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		CHECK_NULL(name, "name", x);
		return xml_find_tag(root, name, &count);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

char *fdmc_xml_find_value(XML_TREE_NODE *root, char *name, int count, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_find_value");
	XML_TREE_NODE *v_node;
	FDMC_EXCEPTION x;
	TRYF(x)
	{
		dbg_trace();
		func_trace("searching value '%s' count %d", name, count);
		v_node = fdmc_xml_find_node(root, name, count, &x);
		if(v_node != NULL)
		{
			char *p = fdmc_xml_find_data(v_node, &x);
			return p;
		}
		return NULL;
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
}

//---------------------------------------------------------
//  name: fdmc_xml_find_data
//---------------------------------------------------------
//  purpose: find text data for tag
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - node to search for
//		err - error handler
//	return:
//		On Success - poiner to text
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
char* fdmc_xml_find_data(XML_TREE_NODE *root, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_find_data");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *node;
	FDMC_LIST_ENTRY *list;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		list = root->child->first;
		while(list)
		{
			node = list->data;
			if(node->nodetype == XML_PCDATA)
			{
				return node->value;
			}
			list = list->next;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

XML_TREE_NODE *fdmc_xml_find_data_node(XML_TREE_NODE *root, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_find_data_node");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *node;
	FDMC_LIST_ENTRY *list;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		list = root->child->first;
		while(list)
		{
			node = list->data;
			if(node->nodetype == XML_PCDATA)
			{
				return node;
			}
			list = list->next;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return NULL;
}

//---------------------------------------------------------
//  name: fdmc_xml_find_cdata
//---------------------------------------------------------
//  purpose: find CDATA for tag
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - node to search for

//		err - error handler
//	return:
//		On Success - poiner to text
//		On Failure - NULL
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
char* fdmc_xml_find_cdata(XML_TREE_NODE *root, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_xml_find_cdata");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *node;
	FDMC_LIST_ENTRY *list;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		CHECK_NULL(root->child, "root->child", x);
		list = root->child->first;
		while(list)
		{
			node = list->data;
			if(node->nodetype == XML_CDATA)
			{
				return node->value;
			}
			list = list->next;
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}
	return NULL;
}

//---------------------------------------------------------
//  name: print_xml_tree
//---------------------------------------------------------
//  purpose: recursive pad for fdmc_print_xml_tree
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		root - node to print
//		tabul - recursive counter
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//---------------------------------------------------------
static int print_xml_tree(XML_TREE_NODE *root, int *tabul)
{
	//FUNC_NAME("fdmc_print_xml_tree");
	XML_TREE_NODE *data;
	FDMC_LIST_ENTRY *xml_entry;

	/* Обход дерева по ветвям */
	/* dbg_trace(); */
	tprintf("%*s<%s", *tabul, "", root->name);
	xml_entry = root->child->first;
	/* Печать атрибутов */
	while(xml_entry != NULL)
	{
		data = xml_entry->data;
		if(data->nodetype == XML_ATTRIBUTE)
		{
			tprintf(" %s=\"%s\"", data->name, data->value);
		}
		else
		{
			break;
		}
		xml_entry = xml_entry->next;
	}
	if(xml_entry == NULL)
	{
		/* Пустой тэг */
		tprintf("/>\n");
		return FDMC_OK;
	}
	tprintf(">\n");
	while(xml_entry != NULL)
	{
		data = xml_entry->data;
		switch(data->nodetype)
		{
		case XML_TAG:
		case XML_EMPTYTAG:
			*tabul += 4;
			print_xml_tree(data, tabul);
			*tabul -= 4;
			break;
		case XML_PCDATA:
			tprintf("%*s%s\n", *tabul, "", data->value);
			break;
		case XML_CDATA:
			tprintf("%*s<![CDATA[%s%*s]]>\n", *tabul, "", data->value,
				*tabul, "");
			break;
		}
		xml_entry = xml_entry->next;
	}
	tprintf("%*s</%s>\n", *tabul, "", root->name);
	return FDMC_OK;
}

//---------------------------------------------------------
//  name: fdmc_print_xml_tree
//---------------------------------------------------------
//  purpose: print xml tree into thread log stream with tabs
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		root - xml tree node
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_print_xml_tree(XML_TREE_NODE *root, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_print_xml_tree");
	FDMC_EXCEPTION x;
	int tabul = 0;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		print_xml_tree(root, &tabul);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------
//  name: s_print_xml_tree
//---------------------------------------------------------
//  purpose: pad recursive function for fdmc_s_print_xml_tree
//		print xml tree into string with tabs 
//  designer: Serge Borisov (BSA)
//  started: 16/09/10
//	parameters:
//		root - tree to print
//		tabul - pointer to tabulation counter
//		dest - destination string
//		dest_size - maximum dest length
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
static int s_print_xml_tree(XML_TREE_NODE *root, int *tabul, char *dest, int *dest_size,
					  FDMC_EXCEPTION *err)
{
	FUNC_NAME("s_print_xml_tree");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *data;
	FDMC_LIST_ENTRY *xml_entry;
	char *buf = NULL;
#define XMLMSG_CHECK_LEN() if(*dest_size <= 0) fdmc_raisef(FDMC_MEMORY_ERROR, &x, "buffer too small");\
else strcat(dest, buf) 
	int i;

	TRYF(x)
	{
		/* Обход дерева по ветвям */
		buf = fdmc_malloc(1024, &x);
		*dest_size -= sprintf(buf, "%*s<%s", *tabul, "", root->name);
		XMLMSG_CHECK_LEN();
		xml_entry = root->child->first;
		/* Печать атрибутов */
		while(xml_entry != NULL)
		{
			data = xml_entry->data;
			if(data->nodetype == XML_ATTRIBUTE)
			{
				*dest_size -= sprintf(buf, " %s=\"%s\"", data->name, data->value);
				XMLMSG_CHECK_LEN();
			}
			else
			{
				break;
			}
			xml_entry = xml_entry->next;
		}
		if(xml_entry == NULL)
		{
			/* Пустой тэг */
			*dest_size -= sprintf(buf, "/>\n");
			XMLMSG_CHECK_LEN();
			return FDMC_OK;
		}
		*dest_size -= sprintf(buf, ">\n");
		XMLMSG_CHECK_LEN();
		/* Содержимое тэга */
		while(xml_entry != NULL)
		{
			data = xml_entry->data;
			switch(data->nodetype)
			{
			case XML_TAG:
			case XML_EMPTYTAG:
				*tabul += 4;
				fdmc_free(buf, &x);
				s_print_xml_tree(data, tabul, dest, dest_size, &x);
				buf = fdmc_malloc(1024, &x);
				*tabul -= 4;
				break;
			case XML_PCDATA:
				if(*dest_size < (int)strlen(data->value) + *tabul)
				{
					fdmc_raisef(FDMC_MEMORY_ERROR, &x, "");
				}
				*dest_size -= sprintf(buf, "%*s", *tabul, "");
				for(i = 0; i < (int)strlen(data->value); i++)
				{
					*dest_size -= export_char(&data->value[i], buf);
					XMLMSG_CHECK_LEN();
				}
				XMLMSG_CHECK_LEN();
				break;
			case XML_CDATA:
				if(*dest_size < (int)strlen(data->value))
				{
					fdmc_raisef(FDMC_MEMORY_ERROR, &x, "");
				}
				*dest_size -= sprintf(buf, "%*s<![CDATA[%s\n%*s]]>\n", *tabul, "", data->value,
					*tabul, "");
				XMLMSG_CHECK_LEN();
				break;
			}
			xml_entry = xml_entry->next;
		}
		tprintf("%*s</%s>\n", *tabul, "", root->name);
	}
	EXCEPTION
	{
		fdmc_free(buf, NULL);
		fdmc_raiseup(&x, err);
		return 0;
	}
	fdmc_free(buf, NULL);
	return 1;
#undef XMLMSG_CHECK_LEN
}

//---------------------------------------------------------
//  name: fdmc_s_print_xml_tree
//---------------------------------------------------------
//  purpose: print xml tree into buffer with tabs
//  designer: Serge Borisov (BSA)
//  started: 16/09/10
//	parameters:
//		root - tree to print
//		tabul - tabulation start
//		dest - destination string
//		dest_size - maximum dest length
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		if err is not null function performs long jump on error
//---------------------------------------------------------
int fdmc_s_print_xml_tree(XML_TREE_NODE *root, int tabul, char *dest, int dest_size,
					  FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_s_print_xml_tree");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(root, "root", x);
		CHECK_NULL(dest, "dest", x);
		CHECK_ZERO(dest_size, "dest_size", x);
		CHECK_NEG(tabul, "tabul", x);
		s_print_xml_tree(root, &tabul, dest, &dest_size, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}
//---------------------------------------------------------
//  name: export_char
//---------------------------------------------------------
//  purpose: copy char to output string considering xml features
//  designer: Serge Borisov (BSA)
//  started:
//	parameters:
//		buffer - pointer to character to export
//		result - result buffer
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//		function is static
//---------------------------------------------------------
static int export_char(char *buffer, char *result)
{
	//FUNC_NAME("export_char");
	unsigned char uch;
	int len;

	switch(buffer[0])
	{
	case '>':
		strcpy(result, "&gt;");
		len = 4;
		break;
	case '<':
		strcpy(result, "&lt;");
		len = 4;
		break;
	case '\'':
		strcpy(result, "&apos;");
		len = 6;
		break;
	case '"':
		strcpy(result, "&quot;");
		len = 6;
		break;
	case '&':
		strcpy(result, "&amp;");
		len = 5;
		break;
	default:
		if(buffer[0] == 0)
		{
			result[0] = 0;
			len = 0;
			break;
		}
		else if( (unsigned)buffer[0] < 32 )//|| (unsigned)(buffer[0]) > (unsigned)127)
		{
			uch = buffer[0];
			len = sprintf(result, "&#%u;", uch);
		}
		else
		{
			result[0] = buffer[0];
			result[1] = 0;
			len = 1;
		}
		break;
	}
	return len;
}

//---------------------------------------------------------
//  name: fdmc_export_xml_node
//---------------------------------------------------------
//  purpose: export xml node to destination string
//  designer: Serge Borisov (BSA)
//  started: 17/09/10
//	parameters:
//		xml_tree - node to export
//		buffer - destination buffer
//		bufsize - recursive buffer size
//		err - error handler
//	return:
//		On Success - true
//		On Failure - false
//  special features:
//---------------------------------------------------------
/* Recursive pad retruns number of characters */
static int export_xml_node(XML_TREE_NODE *xml_tree, char *buffer, int *bufsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("export_xml_node");
	int total, len;
	XML_TREE_NODE *xml_node;
	FDMC_LIST_ENTRY *xml_list;
	char expbuf[XML_TOKENMAX];
	char *buf = NULL;
	char *value;
	FDMC_EXCEPTION x;
#define XML_COPYBUF() total += len; *bufsize -= len; \
	if(*bufsize <= 0) fdmc_raisef(FDMC_MEMORY_ERROR, &x, "buffer too small"); else strcat(buffer, buf)
	TRYF(x)
	{
		if(xml_tree->nodetype != XML_TAG && xml_tree->nodetype != XML_EMPTYTAG)
		{
			fdmc_raisef(FDMC_PARAMETER_ERROR, &x, "invalid start node (not tag)");
		}
		total = 0;
		buf = fdmc_malloc(XML_TOKENMAX, NULL);
		len = sprintf(buf, "<%s", xml_tree->name);
		XML_COPYBUF();
		xml_list = xml_tree->child->first;
		while(xml_list != NULL)
		{
			xml_node = xml_list->data;
			if(xml_node->nodetype == XML_ATTRIBUTE)
			{
				len = sprintf(buf, " %s='", xml_node->name);
				XML_COPYBUF();
				value = xml_node->value;
				while(*value)
				{
					len = export_char(value, buf);
					XML_COPYBUF();
					value ++;
				}
				len = sprintf(buf, "'");
				XML_COPYBUF();
			}
			else
			{
				break;
			}
			xml_list = xml_list->next;
		}
		if(xml_list == NULL)
		{
			len = sprintf(buf, "/>");
		}
		else
		{
			len = sprintf(buffer + total, ">");
			XML_COPYBUF();
			while(xml_list != NULL)
			{
				xml_node = xml_list->data;
				switch(xml_node->nodetype)
				{
				case XML_TAG:
				case XML_EMPTYTAG:
					len = export_xml_node(xml_node, buffer, bufsize, &x);
					XML_COPYBUF();
					break;
				case XML_PCDATA:
					value = xml_node->value;
					while(*value)
					{
						len = export_char(value, expbuf);
						XML_COPYBUF();
						value++;
					}
					break;
				case XML_CDATA:
					len = sprintf(buffer+total, "<![CDATA[%s]]>", xml_node->value);
					XML_COPYBUF();
					break;
				}
				xml_list = xml_list->next;
			}
			len = sprintf(buffer + total, "</%s>", xml_tree->name);
			XML_COPYBUF();
		}
		fdmc_free(buf, NULL);
	}
	EXCEPTION
	{
		fdmc_free(buf, NULL);
		total = 0;
		fdmc_raiseup(&x, err);
	}
	return total;
#undef XML_COPYBUF
}


int fdmc_export_xml_node(XML_TREE_NODE *xml_tree, char *buffer, int bufsize, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_export_xml_node");
	FDMC_EXCEPTION x;

	TRYF(x)
	{
		// Start error handling
		CHECK_NULL(xml_tree, "xml_tree", x);
		CHECK_NULL(buffer, "buffer", x);
		CHECK_NEGZERO(bufsize, "bufsize", x);
		return export_xml_node(xml_tree, buffer, &bufsize, &x);
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
	}

	return 0;
}

// Extract tag record in xml message
int fdmc_extract_tag(FDMC_XML_MESSAGE **p_msg, XML_TREE_NODE *p_node, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_extract_tag");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *v_node;
	// dbg_trace();

	TRYF(x)
	{
		trace("processing node %s", p_node->name);
		if(strcmp((*p_msg)->xname, p_node->name))
		{
			fdmc_raisef(FDMC_DATA_ERROR, &x, "Invalid tag name '%s'. Expected '%s'", p_node->name, (*p_msg)->xname);
		}
		// ** psg - tag descriptor
		// start from next record until close tag
		for(++(*p_msg); (*p_msg)->xtype != XML_CLOSETAG; (*p_msg)++)
		{
			switch ((*p_msg)->xtype)
			{
			case XML_PCDATA:
				// Plain text in message
				v_node = fdmc_xml_find_data_node(p_node, &x);
				if(v_node == NULL)
				{
					func_trace("No data node in tag <%s>", p_node->name);
					break;
				}
				(*(*p_msg)->xfunction)(*p_msg, v_node, FDMC_EXTRACT);
				break;
			case XML_ATTRIBUTE:
				// Find attribute node an get attribute value
				v_node = fdmc_xml_find_attr(p_node, (*p_msg)->xname, &x);
				if(v_node == NULL)
				{
					func_trace("No attribute <%s> in tag <%s>", (*p_msg)->xname, p_node->name);
					break;
				}
				(*(*p_msg)->xfunction)(*p_msg, v_node, FDMC_EXTRACT);
				break;
			case XML_TAG:
				// Process nested tag
				v_node = fdmc_xml_find_node(p_node, (*p_msg)->xname, (*p_msg)->xnumber, &x);
				if(v_node == NULL)
				{
					func_trace("No nested tag <%s> in tag <%s>", (*p_msg)->xname, p_node->name);
					while((*p_msg)->xtype != XML_CLOSETAG)
					{
						++(*p_msg);
						if((*p_msg)->xtype == XML_EMPTY)
						{
							//fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Unexpected end of message descriptor");
							return 1;
						}
					}
					break;
				}
				fdmc_extract_tag(p_msg, v_node, &x);
				break;
			case XML_EMPTY:
				return 1;
				//fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Unexpected end of message descriptor");
				break;
			default:
				fdmc_raisef(FDMC_FORMAT_ERROR, &x, "Unsupported lexeme type: %d", (*p_msg)->xtype);
				break;
			}
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return 0;
	}
	return 1;
}

//-------------------------------------------
// Name: fdmc_format_tag
// 
//-------------------------------------------
// Purpose: generate xml tree from descriptor
// 
//-------------------------------------------
// Parameters: p_msg - message descriptor
// None
//-------------------------------------------
// Returns: New xml tree structure
// 
//-------------------------------------------
// Special features:
//
//-------------------------------------------
XML_TREE_NODE *fdmc_format_tag(FDMC_XML_MESSAGE **p_msg, XML_TREE_NODE *p_node, FDMC_EXCEPTION *err)
{
	FUNC_NAME("fdmc_format_tag");
	FDMC_EXCEPTION x;
	XML_TREE_NODE *v_node, *v_new;

	TRYF(x)
	{
		if(p_node == NULL)
		{
			if((*p_msg)->xtype != XML_TAG)
			{
				fdmc_raisef(FDMC_DATA_ERROR, &x, "first element in xml message must be tag");
			}
			// Create root node
			v_node = (XML_TREE_NODE*)fdmc_malloc(sizeof(*v_node), &x);
			// Create child list
			v_node->child = fdmc_list_create(xml_node_processor, &x);
			v_node->nodetype = XML_TAG;
			v_node->name = fdmc_strdup((*p_msg)->xname, &x);
		}
		else
		{
			v_node = p_node;
		}
		// start from next record until close tag
		for(++(*p_msg); (*p_msg)->xtype != XML_CLOSETAG; (*p_msg)++)
		{
			// Create new node
			v_new = (XML_TREE_NODE*)(fdmc_malloc(sizeof(*v_node), NULL));
			// Create brothers list entry
			v_new->brother = fdmc_list_entry_create(v_new, &x);
			// Add node to the parent's children
			fdmc_list_entry_add(v_node->child, v_new->brother, &x);
			switch ((*p_msg)->xtype)
			{
			case XML_PCDATA:
				// Plain text in message
				v_new->nodetype = XML_PCDATA;
				(*(*p_msg)->xfunction)(*p_msg, v_new, FDMC_FORMAT);
				break;
			case XML_ATTRIBUTE:
				// Attribute of tag
				v_new->nodetype = XML_ATTRIBUTE;
				v_new->name = fdmc_strdup((*p_msg)->xname, NULL);
				(*(*p_msg)->xfunction)(*p_msg, v_new, FDMC_EXTRACT);
				break;
			case XML_TAG:
				// Nested tag
				v_new->nodetype = XML_TAG;
				v_new->child = fdmc_list_create(xml_node_processor, &x);
				v_new->name = fdmc_strdup((*p_msg)->xname, NULL);
				// Process nested tag
				fdmc_format_tag(p_msg, v_new, &x);
				break;
			}
		}
	}
	EXCEPTION
	{
		fdmc_raiseup(&x, err);
		return NULL;
	}
	return v_node;
}


void fdmc_xml_int_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag)
{
	int *v_data;
	char v_buf[25];

	v_data = (int*)p_msg->xdata;
	switch(p_flag)
	{
	case FDMC_EXTRACT:
		*v_data = atoi(p_node->value);
		break;
	case FDMC_FORMAT:
		sprintf(v_buf, p_msg->xformat, *v_data);
		p_node->value = fdmc_strdup(v_buf, NULL);
		break;
	}
}

void fdmc_xml_float_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag)
{
	double *v_data;
	char v_buf[25];

	v_data = (double*)p_msg->xdata;
	switch(p_flag)
	{
	case FDMC_EXTRACT:
		*v_data = atof(p_node->value);
		break;
	case FDMC_FORMAT:
		sprintf(v_buf, p_msg->xformat, *v_data);
		p_node->value = fdmc_strdup(v_buf, NULL);
		break;
	}
}

void fdmc_xml_char_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag)
{
	switch(p_flag)
	{
	case FDMC_FORMAT:
		p_node->value = fdmc_strdup((char*)p_msg->xdata, NULL);
		break;
	case FDMC_EXTRACT:
		strcpy((char*)p_msg->xdata, p_node->value);
		break;
	}
}
