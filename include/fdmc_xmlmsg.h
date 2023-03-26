#ifndef _FDMC_XMLMSG_INCLUDE_
#define _FDMC_XMLMSG_INCLUDE_

#include "fdmc_global.h"
#include "fdmc_list.h"
#include "fdmc_hash.h"
#include "fdmc_logfile.h"
#include "fdmc_msgfield.h"

#include <stdio.h>

/* Типы разбираемых лексем */
typedef enum
{
	XML_STREAM_END,
	XML_OPEN_TAG,
	XML_CLOSE_TAG,
	XML_OPEN_CLOSETAG,
	XML_CLOSE_EMPTYTAG,
	XML_OPEN_COMMENT,
	XML_CLOSE_COMMENT,
	XML_OPEN_INSTRUCTION,
	XML_CLOSE_INSTRUCTION,
	XML_OPEN_DESCRIPTION,
	XML_CLOSE_DESCRIPTION,
	XML_OPEN_CDATA,
	XML_CLOSE_CDATA,
	XML_OPEN_ELEMENT,
	XML_CLOSE_ELEMENT,
	XML_QUOTA,
	XML_AMPERSAND,
	XML_SEMICOLON,
	XML_EQUAL,
	XML_IDENTIFIER,
	XML_MACROS,
	XML_SLASH,
	XML_APOSTROPH,
	XML_NOT_LEXEME,
	XML_TEXT,
	XML_NUMBER,
	XML_AMP,
	XML_GT,
	XML_LT,
	XML_QUOT,
	XML_APOS, 
	XML_BLANK
} XML_LEXTYPE;

/* Структура буфера обмена данными с файлом */
typedef struct
{
	/* Последний считанный символ */
	char lastch;
	/* Тип последней считанной лексемы */
	XML_LEXTYPE lastlextype;
	/* Указатель на последнюю считанную лексему */
	char *lastlex;
	/* Текущая позиция в буфере */
	int bufpos;
	/* Количество значащих символов в буфере */
	int bufmax;
	/* Размер буфера */
	int bufsize;
	/* Общее количество считанных символов */
	int buftotal;
	/* Количество обработанных текстовых строк */
	int lineno, lastlineno;
	/* Символ в текущей строке */
	int linepos, lastlinepos;
	/* Файл данных*/
	FILE *bufsource;
	/* Указатель на входной буфер */
	char *buffer;
} XML_PARSER_BUFFER;

typedef struct
{
	char *lexeme;
	XML_LEXTYPE lextype;
} XML_LEXEME;

/* Размеры буферов */
#define XML_TOKENMAX 1024
#define XML_TEXTMAX 4096

/* прототипы функций */
/* функции создания */
extern XML_TREE_NODE* fdmc_parse_xml_file(char *filename, FDMC_EXCEPTION *err);
extern XML_TREE_NODE *fdmc_parse_xml_buffer(void *buffer, int bufsize, FDMC_EXCEPTION *err);

/* Функции добавления в дерево */
extern XML_TREE_NODE *fdmc_xml_add_tag(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION*);
extern XML_TREE_NODE *fdmc_xml_add_attr(XML_TREE_NODE *root, char *name, char *value, FDMC_EXCEPTION*);
extern XML_TREE_NODE *fdmc_xml_add_attrf(XML_TREE_NODE *root, FDMC_EXCEPTION *ex, char *name, char *fmt, ...);
extern XML_TREE_NODE *fdmc_xml_add_text(XML_TREE_NODE *root, char *text, FDMC_EXCEPTION*);
extern XML_TREE_NODE *fdmc_xml_add_textf(XML_TREE_NODE *root, FDMC_EXCEPTION *ex, char *fmt, ...);

/* Функции печати и экспорта */
extern int fdmc_s_print_xml_tree(XML_TREE_NODE *root, int tabul, char *dest, int dest_size,
					  FDMC_EXCEPTION *err);
extern int fdmc_print_xml_tree(XML_TREE_NODE *root, FDMC_EXCEPTION *err);
extern int fdmc_export_xml_node(XML_TREE_NODE *xml_tree, char *buffer, int bufsize, FDMC_EXCEPTION *err);

/* Функции поиска */
extern char* fdmc_xml_find_cdata(XML_TREE_NODE *root, FDMC_EXCEPTION *err);
extern XML_TREE_NODE *fdmc_xml_find_node(XML_TREE_NODE *root, char *name, int count, FDMC_EXCEPTION *err);
extern char *fdmc_xml_find_value(XML_TREE_NODE *root, char *name, int count, FDMC_EXCEPTION *err);
extern XML_TREE_NODE *fdmc_xml_find_attr(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION *err);
extern XML_TREE_NODE *fdmc_xml_find_tag(XML_TREE_NODE *root, char *name, FDMC_EXCEPTION *err);
extern char* fdmc_xml_find_data(XML_TREE_NODE *root, FDMC_EXCEPTION *err);
extern int fdmc_free_xml_tree(XML_TREE_NODE *root);


typedef struct _XML_TREE_REF
{
	char *name;
	void *buf;
} XML_TREE_REF;

// This structure describes xml message layout
// Possible vales for xtype are:
// XML_TAG, XML_CLOSETAG, XML_ATTRIBUTE, XML_PCDATA
typedef struct _FDMC_XML_MESSAGE
{
	XML_NODETYPE xtype; // xml data type i.e. tag, attribute or data)
	char *xname; // name of tag or attribute
	int xnumber; // number of tag with this name in message
	void *xdata; // address of host variable
	void (*xfunction)(struct _FDMC_XML_MESSAGE*, XML_TREE_NODE *, int ); // conversion function p_data->xdata
	char *xformat; // format of field in any application style
} FDMC_XML_MESSAGE;

// Tags
#define FDMC_XMLMSG_TAG(name, number) {XML_TAG, name, number}
#define FDMC_XMLMSG_CLOSETAG {XML_CLOSETAG}

// Attributes
#define FDMC_XMLMSG_INT_ATTR(name, buf, format) {XML_ATTRIBUTE, name, 1, &buf, fdmc_xml_int_field, format}
#define FDMC_XMLMSG_FLOAT_ATTR(name, buf, format) {XML_ATTRIBUTE, name, 1, &buf, fdmc_xml_float_field, format}
#define FDMC_XMLMSG_CHAR_ATTR(name, buf) {XML_ATTRIBUTE, name, 1, buf, fdmc_xml_char_field}

// Tag data
#define FDMC_XMLMSG_INT_DATA(buf, format) {XML_PCDATA, NULL, 1, &buf, fdmc_xml_int_field, format}
#define FDMC_XMLMSG_FLOAT_DATA(buf, format) {XML_PCDATA, NULL, 1, &buf, fdmc_xml_float_field, format}
#define FDMC_XMLMSG_CHAR_DATA(buf) {XML_PCDATA, NULL, 1, buf, fdmc_xml_char_field}

// Processor functions
XML_TREE_NODE *fdmc_format_tag(FDMC_XML_MESSAGE **p_msg, XML_TREE_NODE *p_node, FDMC_EXCEPTION *err);
int fdmc_extract_tag(FDMC_XML_MESSAGE **p_msg, XML_TREE_NODE *p_node, FDMC_EXCEPTION *err);
void fdmc_xml_int_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag);
void fdmc_xml_float_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag);
void fdmc_xml_char_field(FDMC_XML_MESSAGE *p_msg, XML_TREE_NODE *p_node, int p_flag);

#endif
