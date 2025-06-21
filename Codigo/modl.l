%{

/*
 *	Procesador de ficheros de entrada
 *	(Analisis lexico)
 * Win_flex --wincompat -i MODANA_0.l
 */

#include "mody.tab.h"
#include <string.h>
#include <stdio.h>

/*#include "MODANA_0_INP.h" */

	int lineno = 1;	    /* numero de linea del fichero de entrada */
	char linebuf[500];  /* almacenamiento de la ultima linea      */
	int tokenpos = 0;   /* posicion en la ultima linea leida      */
	int col;
%}

%option noyywrap
%option caseless
%x COMMENT L_COMMENT 

num1		[-+]?([[:digit:]]+)\.?([eE][-+]?([[:digit:]]+))?
num2		[-+]?([[:digit:]]*)\.([[:digit:]]+)([eE][-+]?([[:digit:]]+))?
real_num	{num1}|{num2}

name		[[:alpha:]][[:alnum:]_]* 

%%

  /* Commentarios: linea*/

"//" BEGIN(L_COMMENT);
<L_COMMENT>"\n" {col = 1; lineno++; BEGIN(INITIAL);}
<L_COMMENT><<EOF>> {BEGIN(INITIAL); }
<L_COMMENT>[^\n]+ ;

 /* Commentarios: bloque*/

"/*" BEGIN(COMMENT);
<COMMENT>[^*\n]*
<COMMENT>[^*\n]*\n ++lineno;
<COMMENT>"*"+[^*/\n]*
<COMMENT>"*"+[^*/\n]*\n ++lineno;
<COMMENT>"*"+"/" BEGIN(INITIAL);

[ \t]+	{tokenpos += yyleng;}

\n.*	{  strcpy(linebuf, yytext+1);  /* guarda la proxima linea */
	   lineno++;                    /* numero de lineas leidas */
	   yyless(1);                   /* devuelve todo salvo \n  */
	   tokenpos=0;                  /* inicializacion de posicion */
	 }


MODEL				{tokenpos += yyleng ; return MODEL;}

END					{tokenpos += yyleng ; return END;}

VARIABLE			{tokenpos += yyleng ; return VARIABLE;}

AS					{tokenpos += yyleng ; return AS;}

value				{tokenpos += yyleng ; return VALUE;}

lower				{tokenpos += yyleng ; return LOWER;}

upper				{tokenpos += yyleng ; return UPPER;}

RealParameter		{tokenpos += yyleng ; yylval.texto = strdup(yytext); return REALPARAMETER;}

INTEGERPARAMETER	{tokenpos += yyleng ; return INTEGERPARAMETER;}

LOGICALPARAMETER	{tokenpos += yyleng ; return LOGICALPARAMETER;}

fixed				{tokenpos += yyleng ; return FIXED;}

free				{tokenpos += yyleng ; return FREE;}


SIN  				{tokenpos += yyleng ; return SIN;} 

COS       			{tokenpos += yyleng ; return COS;}

TAN       			{tokenpos += yyleng ; return TAN;}

SINH     			{tokenpos += yyleng ; return SINH;}

COSH     			{tokenpos += yyleng ; return COSH;}

TANH     			{tokenpos += yyleng ; return TANH;}

ASIN  				{tokenpos += yyleng ; return ASIN;} 

ACOS       			{tokenpos += yyleng ; return ACOS;}

ATAN       			{tokenpos += yyleng ; return ATAN;}

SQRT       			{tokenpos += yyleng ; return SQRT;} 

EXP       			{tokenpos += yyleng ; return EXP;}

LOGe				{tokenpos += yyleng ; return LOGe;}

LOG10				{tokenpos += yyleng ; return LOG10;}

ABS       			{tokenpos += yyleng ; return ABS;} 

MAX       			{tokenpos += yyleng ; return MAX;}

MIN					{tokenpos += yyleng ; return MIN;}

MID					{tokenpos += yyleng ; return MID;}


{name}				{tokenpos += yyleng ; yylval.texto = strdup(yytext); return NAME;}		

{real_num}			{tokenpos += yyleng ; yylval.real = atof (yytext); return NUM;}

.					{tokenpos += yyleng ; return yytext[0];}


%% 

/* 
int yywrap(){
  yyterminate(); 
}
*/