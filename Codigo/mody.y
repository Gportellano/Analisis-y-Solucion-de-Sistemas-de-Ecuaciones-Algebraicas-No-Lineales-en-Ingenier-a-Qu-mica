%{
/*
 *	MODANA_0    
 */


/*extern FILE  *info;*/

#define YYDEBUG 1
//extern int yydebug;

#include "hmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *yytext;
extern char linebuf[];
extern int yylex();

extern FILE* yyin;

int i = 0;
int indice_ec = 0;
int num_var = 0;

%}

%union {                     /* Devuelve los valores en yylval   */
      char * texto;          /* con el tipo que les corresponda  */
      int entero;            /* Sin esta sentencia yylval es int */
	  double real;

	  struct lista *list;
	  struct ast *a;
	  struct lista_prop1 *prop1;
	  struct lista_prop2 *prop2;
       }

%token MODEL
%token END
%token VARIABLE
%token AS
%token VALUE
%token LOWER
%token UPPER
%token INTEGERPARAMETER
%token LOGICALPARAMETER
%token FIXED
%token FREE

%token	SIN
%token	COS
%token	TAN
%token 	SINH
%token	COSH
%token	TANH
%token	ASIN
%token	ACOS
%token	ATAN
%token  SQRT
%token	EXP
%token	LOGe
%token 	LOG10
%token	ABS
%token	MAX
%token	MIN
%token	MID

%token<real> NUM
%token<texto> NAME
%token<texto> REALPARAMETER

%type<list>lista_de_nombres
%type<list>declaracion_de_parametros
%type<a>expresion ecuacion
%type<prop1> propiedad1 propiedad2
%type<prop2> lista_de_propiedades1 lista_de_propiedades2
%type<texto> funcion



/*
 *	Tabla de prelacion 
 */

/* %precedence '='  */
/* %precedence NEG no existe %precedence negation--unary minus */
%left '-' '+'
%left '*' '/'
%right '^' /* exponentiation */
%nonassoc  UMINUS


%%

problema
	: MODEL NAME lista_de_sentencias END  {printf("\nTODAY IS SUNDAY\n");}
	;

lista_de_sentencias
	: sentencia
	| lista_de_sentencias sentencia
	;

sentencia
	: definicion
	| declaracion
	| asignacion
	;




definicion
	: definicion_tipo_de_variable
/*	| definicion_tipo_de_parametros */
	;

declaracion
	: declaracion_de_variables
	| declaracion_de_parametros
	| declaracion_de_ecuaciones
	;


asignacion
	: asignacion_de_variables
/*	| asignacion_de_parametros */
	;



									/*declaración de tipo de variable*/



definicion_tipo_de_variable
	: VARIABLE NAME lista_de_propiedades1 END {guardar_prop_tipo(strdup($2), $3, 1);}
	;

lista_de_propiedades1
	: propiedad1 {struct lista_prop2 *lista = malloc(sizeof(struct lista_prop2)); lista->actual = $1; lista->next = NULL; $$ = lista;}
	| lista_de_propiedades1 propiedad1 {struct lista_prop2 *lista = malloc(sizeof(struct lista_prop2)); lista->actual = $2; lista->next = $1; $$ = lista;}
	;

propiedad1
	: VALUE ':' expresion ';' {struct lista_prop1 *value = malloc(sizeof(struct lista_prop1)); value->name = strdup("value"); value->value = malloc(sizeof(struct num)); value->value->num = ((struct num *)$3)->num; $$ = value;}
	| LOWER ':' expresion ';' {struct lista_prop1 *lower = malloc(sizeof(struct lista_prop1)); lower->name = strdup("lower"); lower->value = malloc(sizeof(struct num)); lower->value->num = ((struct num *)$3)->num; $$ = lower;}
	| UPPER ':' expresion ';' {struct lista_prop1 *upper = malloc(sizeof(struct lista_prop1)); upper->name = strdup("upper"); upper->value = malloc(sizeof(struct num)); upper->value->num = ((struct num *)$3)->num; $$ = upper;}
	;

lista_de_propiedades2
	: propiedad2 {struct lista_prop2 *lista = malloc(sizeof(struct lista_prop2)); lista->actual = $1; lista->next = NULL; $$ = lista;}
	| lista_de_propiedades2 ',' propiedad2  {struct lista_prop2 *lista = malloc(sizeof(struct lista_prop2)); lista->actual = $3; lista->next = $1; $$ = lista;}
	;

propiedad2
	:  VALUE ':' expresion {struct lista_prop1 *value = malloc(sizeof(struct lista_prop1)); value->name = strdup("value"); value->value = malloc(sizeof(struct num)); value->value->num = ((struct num *)$3)->num; $$ = value;}
	|  LOWER ':' expresion {struct lista_prop1 *lower = malloc(sizeof(struct lista_prop1)); lower->name = strdup("lower"); lower->value = malloc(sizeof(struct num)); lower->value->num = ((struct num *)$3)->num; $$ = lower;}
	|  UPPER ':' expresion {struct lista_prop1 *upper = malloc(sizeof(struct lista_prop1)); upper->name = strdup("upper"); upper->value = malloc(sizeof(struct num)); upper->value->num = ((struct num *)$3)->num; $$ = upper;}
	;


									/* definicion_tipo_de_parametro  */ 



declaracion_de_parametros
	: lista_de_nombres AS REALPARAMETER '(' expresion cp  ';'{struct lista *parametro = $1; add_to_hash_table(parametro, $3, 1, NULL); update_value(parametro, (struct num*)$5);}
	| lista_de_nombres AS INTEGERPARAMETER '(' expresion cp ';'
	;


declaracion_de_variables
	: lista_de_nombres AS NAME ';'{struct lista *listgrande = $1;  
	while (listgrande != NULL) {add_to_hash_table(listgrande, $3, 1, NULL); listgrande = listgrande->next;}}
	| lista_de_nombres AS NAME '(' lista_de_propiedades2 cp ';' {struct lista *listgrande = $1;
	while (listgrande != NULL) {add_to_hash_table(listgrande, $3, 2, $5); listgrande = listgrande->next;};}
	;

lista_de_nombres
	: NAME {struct lista *list = malloc(sizeof(struct lista)); list->name = strdup($1); list->next=NULL; $$=list;} 
	| lista_de_nombres ',' NAME {struct lista *list = malloc(sizeof(struct lista)); list->name = strdup($3); list->next = NULL; 
			struct lista *tmp = $1;
			while (tmp->next != NULL) {
				tmp = tmp->next;
			}
			tmp->next = list;
			$$ = $1;
		}
	;

declaracion_de_ecuaciones
	: ecuacion
	;



													/*asignación de valor a variable o tipo (fixed, free) */



asignacion_de_variables
	: NAME ':' expresion ',' specification ';'  {struct lista *name = malloc(sizeof(struct lista)); name->name = strdup($1); update_value(name, (struct num*)$3);}
	| NAME ':' expresion ','
	| NAME ':' specification ';'
	;

specification
	: FIXED
	| FREE 
	;




										/* asignacion_de_parametros */ 





ecuacion
	: expresion '=' expresion ';' {guardar_ec(new_ast($1,'=',$3)); indice_ec++;}
	;


expresion
	: NUM 						{$$ = num_in_eq($1);}
	| NAME        				{$$ = var_in_eq(strdup($1));}										
	| funcion '(' expresion cp	{$$ = fun_in_eq($1 , $3);}
	| expresion '+' expresion	{$$ = new_ast($1,'+', $3);}
	| expresion '-' expresion	{$$ = new_ast($1,'-', $3);}
	| expresion '*' expresion	{$$ = new_ast($1,'*', $3);}
	| expresion '/' expresion	{$$ = new_ast($1,'/', $3);}
	| '-' expresion %prec UMINUS {$$ = new_ast(0,'u',$2);}
	| expresion '^' expresion 	{$$ = new_ast($1,'^', $3);}
	| '(' expresion cp			{$$ = $2}
	;

funcion
	: SIN {$$ = strdup("sin");}
	| COS {$$ = strdup("cos");} 
	| TAN {$$ = strdup("tan");}  
	| SINH {$$ = strdup("sinh");} 
	| COSH  {$$ = strdup("cosh");}
	| TANH  {$$ = strdup("tanh");}
	| ASIN  {$$ = strdup("asin");}
	| ACOS  {$$ = strdup("acos");}
	| ATAN  {$$ = strdup("atan");}
	| SQRT  {$$ = strdup("sqrt");}
	| EXP	{$$ = strdup("exp");}
	| LOGe  {$$ = strdup("loge");}
	| LOG10 {$$ = strdup("logd");}
	| ABS   {$$ = strdup("abs");}  
	;



/*
*   Simbolos para recuperacion de errores
*/

cp  : ')'  {yyerrok;}

%%


