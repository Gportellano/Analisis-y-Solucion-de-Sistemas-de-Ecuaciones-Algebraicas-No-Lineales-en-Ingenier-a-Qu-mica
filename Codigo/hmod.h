/**********************************************************************

Fichero de encabezamiento auxiliar  

************************************************************************/ 

#include <stdbool.h>
#include <kinsol/kinsol.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_types.h>   /* defs. of sunrealtype, sunindextype */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix       */   

#define MAX_ECS 1000

void yyerror(char* msg);

struct lista {
    char *name;
    struct lista *next;
};

typedef struct identificador { 

	char *name;
    char *type;
	int indice_variable;
	int indice_parametro;

	struct identificador *next;
} identificador;

typedef struct symbol {
    char *name;            
    char *type;

	int flag_fixed; //bandera que indica que es una variable dato
	int flag_active; //bandera que indica si aprece en alguna de las ecuaciones

	double value;
	double lower; 
	double upper;

	struct ecuacion *ecuaciones[MAX_ECS];
	int num_ec;
}symbol;

typedef struct ecuacion{
    int indice;
}ecuacion;

typedef struct symbol_parameter{
	char *name;
	double value;
}symbol_parameter;

//Estructuras para ASTs

typedef struct ast{
	int nodetype;
	struct ast *l;
	struct ast *r;
	char *func;
}ast;

struct num{
	int nodetype;
	double num;
};

struct name{
	int nodetype;
	char *name;
	int indice_vector; // puede ser del vector_variables o vector_parametros
};

// Estrucuturas de propiedades 

typedef struct propiedades{
	char *type;
	double value;
	double lower;
	double upper;
}propiedades;


struct lista_prop2{ 

	struct lista_prop1 *actual;
	struct lista_prop2 *next;
};

struct lista_prop1{ 
	char *name;
	struct num *value;
};

//Struct del vector de incognitas

typedef struct var_incognita{
    char* name;
    double value;
	double upper;
	double lower;
	int indice_vector;

	struct ecuacion *ecuaciones[MAX_ECS];
	int num_ec;
}var_incognita;



extern int num_equations;
extern int num_incognitas;
extern struct var_incognita* vector_incognitas[MAX_ECS];
extern int n_iter;
extern int indice_ec;
extern N_Vector scale;

// FUNCIONES PROTOTIPO

identificador *hash_lookup(char *name);
void add_to_hash_table(struct lista *list, char *name, int def_type_flag, struct lista_prop2 *prop);
void update_value(struct lista  *list, struct num *value);
void guardar_prop_tipo(char *tipo_de_variable, struct lista_prop2 *prop, int tipo_de_propiedad);
void guardar_ec(struct ast *ecuacion);															
ast *new_ast(struct ast *l,int nodetype,struct ast *r);
ast *fun_in_eq(char *func_name, struct ast *r);
ast *var_in_eq(char *name);
ast *num_in_eq(double value);
void completa_valores();
double *enviar_a_evaluar();

void print_table();
void print_props();
void init_hash_table();
void imprime_vector_variables();
void imprime_vector_parametros();
void imprime_ecs_en_var();

int func(N_Vector u, N_Vector fval, void* user_data);
int jac(N_Vector u, N_Vector f, SUNMatrix J, void* user_data, N_Vector tmp1, N_Vector tmp2);
double* enviar_a_evaluar();
double evaluate(struct ast *nodo);

void set_initial_guess(N_Vector u);
void create_vec_incog();

int buscar_ecuacion(symbol *var, int indice_ec);

























