#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "hmod.h" //aquí se encuntran todas los tipos de structs y todas las funciones prototipo 

#include <kinsol/kinsol.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_types.h>   /* defs. of sunrealtype, sunindextype */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix       */ 

#define max_name 256 //MAXIMA LONGITUD DEL NOMBRE DE UNA VARIABLE
#define table_size  1000 //TAMAÑO DE TABLA PARA TIPOS DE VARIABLES
#define MAX_ECS 1000 //MÁXIMO NUMERO DE ECUACIONES
 
//////// VECTORES

identificador *hash_table_variables[table_size];//HASH TABLE PARA GUARDAR VARIBALES, se guardan variables y parámetros por su similitud
propiedades *hash_table_propiedades[table_size]; //HASH TABLE PARA PROPIEDADES, se guardan propiedades 

symbol *vector_variables[table_size]; //VECTOR DE VARIABLES, se guardan las variables que aparecen en el archivo de ACM
symbol_parameter *vector_parametros[table_size]; //VECTOR DE PARÁMETROS, se guardan los parámetros que aprecen en el archivo de ACM
var_incognita *vector_incognitas[MAX_ECS]; //VECTOR DE INCÓGNITAS, se guardan variables incógnita de las ecuaciones

ast *vector_equations[MAX_ECS]; //VECTOR DE ASTs PARA GUARDAR ECUACIONES

//////// CONTADORES

int index_variable = 0; //Permite conocer la posición de la variable en "vector_variables"
int index_parameter = 0; //Permite conocer la posición de la variable en "vector_parametros"

int num_equations = 0; //CONTADOR DE ECUACIONES, permite situar las ecuaciones en "vector_equations"
int num_incognitas = 0; //CONTADOR DE INCÓGNITAS, permite situar las incógnitas en "vector_incognitas"
int n_iter = 0; //CONTADOR DE ITERACIONES, permite conocer el número de llamadas desde KINSOL a la función "func" (se encarga de ejecutar la siguiente iteración)


//FUNCIÓN HASH PARA CALCULAR LA POSICIÓN EN LA TABLA

//VACIAR (INICIALIZAR AMBAS HASH TABLE)
void init_hash_table(){

    printf("INICIALIZO LAS HASH TABLE\n\n");

    for (int i=0; i<table_size; i++){
        hash_table_variables[i]= NULL;
        hash_table_propiedades[i] = NULL;
    }
}

//Calculo el valor hash
unsigned int hash(char *name){      /* unsigned = non negative*/
    
    int length = strnlen(name, max_name);
    unsigned int hash_value =0;

    for(int i=0; i<length; i++){
        hash_value += name[i];
        hash_value = (hash_value * name[i]) % table_size;
    } 
    return hash_value ;
}

//Permite encontrar una variable o parametro en hash_table_variables                                
 identificador *hash_lookup(char *name){

    int index = hash(name);
    identificador *tmp= hash_table_variables[index];

    while(tmp != NULL && (strcmp(tmp->name, name)) != 0){
        tmp = tmp->next;
    }
    return tmp;
}

//Guarda los tipos de propiedades y sus valores (value, upper y lower)
void guardar_prop_tipo(char *tipo_de_variable, struct lista_prop2 *prop, int tipo_de_propiedad){
    
    if(prop==NULL){
        printf("NO ME HA LLEGADO LA LISTA DE PROPIEDADES"); 
    }

    struct lista_prop2 *tmp= prop;

    int index_propiedades = hash(tipo_de_variable);
    // se revisan las colisiones
    for(int i = 0; i < table_size; i++){
        int try = (i+index_propiedades) % table_size;
        if(hash_table_propiedades[try] == NULL){
            hash_table_propiedades[try] = malloc(sizeof(struct propiedades));
                if (hash_table_propiedades[try] == NULL) {
                    fprintf(stderr, "Error de memoria al insertar en tabla hash.\n");
                    exit(1);
                }
            hash_table_propiedades[try]->type = strdup(tipo_de_variable);
            while(tmp != NULL){ //va cambiadno de propiedad (value, lower o upper) en el ultimo tipo de variable declarado  

                if(strcmp(tmp->actual->name, "value") == 0){
                    hash_table_propiedades[try]->value = tmp->actual->value->num;
                }
                else if(strcmp(tmp->actual->name, "lower") == 0){
                    hash_table_propiedades[try]->lower = tmp->actual->value->num;
                }
                else if(strcmp(tmp->actual->name, "upper") == 0){
                    hash_table_propiedades[try]->upper = tmp->actual->value->num;
                }
                tmp = tmp->next;  
            }
            break;
        }
    }
}


//Añade las variables en la hash table y su indice en "vector_variables". Además añade los valores del tipo de variable con el que se declaran 
void add_to_hash_table(struct lista *list, char *type, int def_type_flag, struct lista_prop2 *prop){

    char *name = list->name;

    identificador *sym = malloc(sizeof(identificador));
        if (sym == NULL) {
            fprintf(stderr," No se ha podido crear espacio para el puntero\n");
            exit(1);
        }

    int index1 = hash(name);
    identificador *tmp= hash_lookup(name); //función para buscar si ya se ha declarado una misma variable dentro de un tipo en la hash table

    if(tmp == NULL){
        //Dos casos: el primero, se trata de un parámetro y por tanto de guarda en vector_parámeteros
        // segundo, se trata de una variable, se guarda en vector_variables
        if(strcmp(type, "RealParameter") == 0){
            
            sym->name = strdup(name);
            sym->type = strdup(type);
            sym->indice_parametro = index_parameter;

            vector_parametros[index_parameter] = malloc(sizeof(symbol_parameter));
            vector_parametros[index_parameter]->name = strdup(name);
            index_parameter++;
        }else{
            sym->name = strdup(name);
            sym->type = strdup(type);
            sym->indice_variable = index_variable;

            vector_variables[index_variable]= malloc(sizeof(symbol)); 
            vector_variables[index_variable]->name = strdup(name);
            vector_variables[index_variable]->num_ec = 0;

            //Procede a meter los valores del tipo de variable en la tabla de variables
            //Hace el hash del nombre del tipo, que le da acceso a la posicion en la hash table de las propiedades y por ello a los valores de las propiedades

                int index_propiedades = hash(type);
                for(int i=0; i < table_size; i++){
                    int try = (i+index_propiedades) % table_size;
                    if(hash_table_propiedades[try] != NULL && strcmp(hash_table_propiedades[try]->type, type) == 0){

                        vector_variables[index_variable]->value = hash_table_propiedades[try]->value;
                        vector_variables[index_variable]->upper = hash_table_propiedades[try]->upper;
                        vector_variables[index_variable]->lower = hash_table_propiedades[try]->lower;
                        break;
                    }
                }

            if(def_type_flag == 2){

                while(prop != NULL){

                    if(strcmp(prop->actual->name, "value") == 0){ 
                        vector_variables[index_variable]->value = prop->actual->value->num;
                        //vector_variables[index_variable]->flag_fixed = 1;
                    }
                    else if(strcmp(prop->actual->name, "lower") == 0){
                        vector_variables[index_variable]->lower = prop->actual->value->num;
                    }
                    else if(strcmp(prop->actual->name, "upper") == 0){
                        vector_variables[index_variable]->upper = prop->actual->value->num;
                    }
                    prop = prop->next;
                }
            }
        index_variable++;
        }
        //En caso de existir una colision entre variables o parametros en la hash table, se guardan en listas enlazadas (external linking)
        sym->next = hash_table_variables[index1];
        hash_table_variables[index1] = sym;
    }
    else{
        fprintf(stderr,"Ya existe una variable %s en el tipo %s\n", name, type);
        exit(1);
    }
}

//Funcion que pone el valor (VALUE) dentro del struct symbol (en campo 'value') de la variable que corresponde y parámetros
void update_value(struct lista *list, struct num *value){

    int index1 = hash(list->name);
    identificador *sym = hash_table_variables[index1];
    while(sym != NULL && strcmp(sym->name, list->name) != 0){
        sym = sym->next;
    }
        if (sym == NULL){
            fprintf(stderr, "Error: La variable %s no ha sido declarada.\n", list->name);
            exit(1);
        }
    if(strcmp(sym->type, "RealParameter") == 0)
    {
        int index3 = sym->indice_parametro;
        vector_parametros[index3]->value = value->num;
    }else{
        int index2 = sym->indice_variable;
        vector_variables[index2]->value = value->num;
        vector_variables[index2]->flag_fixed = 1;
    }
}


//Caso variable (un nombre) en la ecuación. Devuelve el nombre que aparece en la ecuacion en la forma de un struct ast, porque si no, NAME y 'expresion' son de distinto tipo y por ello incompatibles en bison
struct ast *var_in_eq(char *name){

    struct name *var = malloc(sizeof(struct name));
        if(var == NULL){
            fprintf(stderr, "Error en funcion var_in_eq");
            exit(1);
        }

    // Busca la variable en la hash table y una vez encontrada busca el campo "indice vector", que le indica la posicion en la tabla de variables.
    // De esta forma, cuando se recorra el AST durante la ejecución, la variable le dirija a la posicion de la tabla de variables con los valores (value, upper y lower) 
    int index1 = hash(name);
    identificador *sym = hash_table_variables[index1];
    while(sym != NULL &&  strcmp(sym->name, name) != 0){
        sym = sym->next;
    }
        if (sym == NULL) {
            fprintf(stderr, "Error: la variable %s no ha sido declarada.\n", name);
            exit(1);
        }
    if(strcmp(sym->type, "RealParameter") == 0){
        var->nodetype = 'p'; //parámetro
        var->name = name;

        int index3 = sym->indice_parametro;
        var->indice_vector = index3;
    }else{
        var->nodetype= 'v'; //variable
        var->name = name;

        int index2 = sym->indice_variable;
        var->indice_vector  = index2;
        vector_variables[index2]->flag_active = 1;

        int encontrada = buscar_ecuacion(vector_variables[index2], indice_ec);
        if(encontrada == 0){

            vector_variables[index2]->ecuaciones[vector_variables[index2]->num_ec] = malloc(sizeof(ecuacion));
            vector_variables[index2]->ecuaciones[vector_variables[index2]->num_ec]->indice= indice_ec;
            vector_variables[index2]->num_ec++;
        }
    }
    return (struct ast*)var; //se devuelve el struct name, anidado por el struct ast
}

int buscar_ecuacion(symbol *var, int indice_ec) {
    for (int i = 0; i < var->num_ec; i++) {
        if (var->ecuaciones[i]->indice == indice_ec) {
            return 1; 
        }
    }
    return 0;
}

//Caso número en ecuación. Devuelve el número (si apareciera) que aparece en la ecuacion en la forma de un struct ast porque si no NUM y 'expresion' son de disitnto tipo y por ello incompatibles en bison
struct ast *num_in_eq(double value){
    //printf("He leido un numero\n");
    struct num *a = malloc(sizeof(struct num));
        if(a == NULL){
            fprintf(stderr, "Error en funcion num_in_eq");
            exit(1);
        }
    a->nodetype= 'c'; //constante
    a->num= value;
    return (struct ast*)a; //se devuelve el struct num, 'casteado' por el struct ast
}


//Caso funcion en ecuacion (sin, cos, exp ...). Crea un AST, pero para el caso de funciones (cos, sin...), porque la funcion anterior no puede recibir como argumetno un string
struct ast *fun_in_eq(char *func_name, struct ast *r){

    struct ast *funcion = malloc(sizeof(struct ast));
        if(funcion == NULL){
            fprintf(stderr, "Error en funcion fun_in_eq");
            exit(1);
        }
    funcion->nodetype = 'f'; //funcion
    funcion->func = func_name;
    funcion->l = NULL; //el struct ast por defecto tiene los campos izquierda (l) y derecha (right (r)), solo necesitas uno de ellos. Se escoge de forma arbitraria 'r'
    funcion->r = r;
    return funcion;
}

//Crea los ASTs de las ecuaciones
struct ast *new_ast(struct ast *l, int nodetype, struct ast *r){ //aqui le estas enviando un struct ast pero nada lo define con el nombre de cada variable

    struct ast *ast = malloc(sizeof(struct ast));
        if(ast == NULL){
            fprintf(stderr, "Error en funcion new_ast");
            exit(1);
        }
    ast->nodetype = nodetype;
    ast->l= l;
    ast->r=r;
    return ast;
}


//Función para guardar en un vector de structs(equations) todas las ecuaciones
void guardar_ec(struct ast *ecuacion){

        if (num_equations >= MAX_ECS) {
            fprintf(stderr, "Se excedió el número máximo de ecuaciones\n");
            exit(1);
        }
    struct ast *glob_ast = malloc(sizeof(struct ast));
    if (glob_ast == NULL) {
        fprintf(stderr, "No hay espacio para glob_ast\n");
        exit(1);
    }

    glob_ast->l = ecuacion->l;
    glob_ast->r = ecuacion->r;
    glob_ast->nodetype = ecuacion->nodetype;

    vector_equations[num_equations++] = glob_ast;
}



 //////// FUNCIONES PARA EL CÁLCULO DE LA SOLUCIÓN

 //Función que forma el vector incógnitas con el que se trabaja de forma auxiliar con kinsol
void create_vec_incog(){
    for(int i = 0; i < index_variable; i++){
        if(vector_variables[i]->flag_fixed != 1 && vector_variables[i]->flag_active == 1){
            vector_incognitas[num_incognitas] = malloc(sizeof(struct var_incognita));
                if (vector_incognitas[num_incognitas] == NULL) {
                    fprintf(stderr, "Error: no se pudo asignar memoria para vector_incognitas[%d]\n", num_incognitas);
                    exit(1);
                }
            vector_incognitas[num_incognitas]->name = vector_variables[i]->name;
            vector_incognitas[num_incognitas]->value = vector_variables[i]->value;
            vector_incognitas[num_incognitas]->upper = vector_variables[i]->upper;
            vector_incognitas[num_incognitas]->lower = vector_variables[i]->lower;
            vector_incognitas[num_incognitas]->indice_vector = i;
            
                vector_incognitas[num_incognitas]->num_ec=0;
            for(int j = 0; j < vector_variables[i]->num_ec; j++){
                vector_incognitas[num_incognitas]->ecuaciones[j] = malloc(sizeof(ecuacion));
                vector_incognitas[num_incognitas]->ecuaciones[j]->indice = vector_variables[i]->ecuaciones[j]->indice; 
                vector_incognitas[num_incognitas]->num_ec++;
            }
            num_incognitas++;
        }
    }
}

 //Se establece la primera suposición de la solución, es decir los valores, por defecto de las variables_incognita (Dadas por el valor value de la propiedad correspondiente)
 void set_initial_guess(N_Vector u1)
{
  sunrealtype* udata;
  udata = N_VGetArrayPointer(u1);
    printf("numero de incognitas: %d\n", num_incognitas);
  // rellena los valores iniciales de las variables
  for(int i = 0; i < num_incognitas; i++){
    udata[i]=vector_incognitas[i]->value;
    //printf("Suposicion inicial para %s (%d) = %f\n", vector_incognitas[i]->name, i+1, vector_incognitas[i]->value);
  }
 ;
  // rellena los valores iniciales para los limites (upper y lower)
  int j=0;
  for(int i = num_incognitas; i < (num_incognitas)*3; i = i + 2){
    udata[i] = vector_incognitas[i-num_incognitas-j]->value - vector_incognitas[i-num_incognitas-j]->lower;
    udata[i+1] = vector_incognitas[i-num_incognitas-j]->value - vector_incognitas[i-num_incognitas-j]->upper;
    j++;
  }
  
}


 int func(N_Vector u, N_Vector fval, void* user_data) {
    n_iter++;
    sunrealtype* udata;
    udata = N_VGetArrayPointer(u);

    //Como se tienen que actualizar los valores en la hash_table, no se puede directamente guardar así sym->value = NV_ITH_S (u,i), porque la incognita puede encontrarse en la posiciion de la celda 5, variable 8
    //por ello se guarda en el vector_incognita, que lleva también el nombre de la variable a la que pertenece. así se puede editar en la hash table para que fucione en la siguiente iteración en evaluate()
    for (int i = 0; i < num_incognitas; i++) {
        vector_incognitas[i]->value = udata[i];
        int index2 = vector_incognitas[i]->indice_vector;
        vector_variables[index2]->value = vector_incognitas[i]->value;
    }

    //Evalua las ecuaciones pero con los nuevos valores de las variables incógita (comineza una nueva iteración)
    //Recibe un vector con los residuos en la función "enviar_a_evaluar"
    double *residuo_vector = enviar_a_evaluar();

    
    int j=0;
    for(int i = num_incognitas; i < num_incognitas*3; i = i+2){
        
        residuo_vector[i]= udata[i] - udata[i-num_incognitas-j] + vector_incognitas[i-num_incognitas-j]->lower;
        residuo_vector[i+1]= udata[i+1] - udata[i-num_incognitas-j] + vector_incognitas[i-num_incognitas-j]->upper;
        j++;
    }
    

    // copia los residuos al vector de salida
    for (int i = 0; i < 3*num_incognitas; i++) {
        //printf("RESIDUO DE LA ECUACION %d: %f\n\n", i + 1, residuo_vector[i]);
        NV_Ith_S(fval, i) = residuo_vector[i];
    }

    free(residuo_vector);
    return 0; 
}


//Llamada individual a evaluate() de cada ecuación
double* enviar_a_evaluar(){

    double* residuo_vector = malloc(3* num_incognitas * sizeof(double));
        if (residuo_vector == NULL) {
            fprintf(stderr, "No se pudo asignar memoria para los residuos.\n");
            return NULL;
        }
    for (int i = 0; i < num_equations; i++) {
        struct ast *eq = vector_equations[i];

        double val_izq = evaluate(eq->l);
        double val_der = evaluate(eq->r);

        double residuo = val_izq - val_der;
        residuo_vector[i] = residuo;
    }
    return residuo_vector;
}

//Inspirada en uno de los ejemplos de flex-bison: evaluación de cada ecuación/ no se llama desde el principal si no desde enviar_a_evaluar()
double evaluate(struct ast *nodo){
    switch(nodo->nodetype){
        case 'c': return ((struct num *)nodo)->num; //Caso constante
        case 'v':{ //Caso variable
                return vector_variables[((struct name*)nodo)->indice_vector]->value;  
        fprintf(stderr, "Variable %s no encontrada en el vector de variables\n", ((struct name*)nodo)->name);
        exit(1);
        }
        case 'p': return vector_parametros[((struct name*)nodo)->indice_vector]->value; //Caso parámetro      
        case '+': return evaluate(nodo->l) + evaluate(nodo->r);break;
        case '-': return evaluate(nodo->l) - evaluate(nodo->r);break;
        case '*': return evaluate(nodo->l) * evaluate(nodo->r);break;
        case '/': return evaluate(nodo->l) / evaluate(nodo->r);break;
        case '^': return pow(evaluate(nodo->l), evaluate(nodo->r)); break;
        case 'u': return -evaluate(nodo->r); break;
        case 'f': { //Caso función
            double val = evaluate(nodo->r);
            if (strcmp(nodo->func, "sin") == 0) return sin(val);
            else if (strcmp(nodo->func, "cos") == 0) return cos(val);
            else if (strcmp(nodo->func, "tan") == 0) return tan(val);
            else if (strcmp(nodo->func, "sinh") == 0) return sinh(val);
            else if (strcmp(nodo->func, "cosh") == 0) return cosh(val);
            else if (strcmp(nodo->func, "tanh") == 0) return tanh(val);
            else if (strcmp(nodo->func, "asin") == 0) return asin(val);
            else if (strcmp(nodo->func, "acos") == 0) return acos(val);
            else if (strcmp(nodo->func, "atan") == 0) return atan(val);
            else if (strcmp(nodo->func, "sqrt") == 0) return sqrt(val);
            else if (strcmp(nodo->func, "exp") == 0) return exp(val);
            else if (strcmp(nodo->func, "loge") == 0) return log(val);
            else if (strcmp(nodo->func, "logd") == 0) return log10(val);
            else if (strcmp(nodo->func, "abs") == 0) return fabs(val);
            else {
                fprintf(stderr, "Funcion %s no reconocida\n", nodo->func);
                exit(1);
            }
        }
        default: printf("Error de evaluacion en un nodo: %c\n", nodo->nodetype);
    }
}


int jac(N_Vector u, N_Vector f, SUNMatrix J, void* user_data, N_Vector tmp1, N_Vector tmp2)
{
    
//Jij=(F(u+sigma)-F(u))/sigma
// con sigma = raiz(U)*max(escalado, u)
 
    int fil = num_equations + 2 *num_incognitas; //numero de funciones
    int col = num_incognitas * 3;  //numero de variables
    double U = DBL_EPSILON;         

    //se crean vectores con la misma dimensión
    N_Vector f_jac_mod = N_VClone(f);
    N_Vector u_mod = N_VClone(u);

    //punteros para poder cambiar sus valores 
    sunrealtype *u_data = N_VGetArrayPointer(u);
    sunrealtype *u_mod_data = N_VGetArrayPointer(u_mod);
    sunrealtype *f_jac_data = N_VGetArrayPointer(f);
    sunrealtype *f_jac_mod_data = N_VGetArrayPointer(f_jac_mod);
    sunrealtype *scale_data = N_VGetArrayPointer(scale);

    for (int j = 0; j < col; j++) {
        // Copiar u en u_mod
        memcpy(u_mod_data, u_data, col * sizeof(double));
        double sigma = sqrt(U) * fmax(fabs(u_data[j]), scale_data[j]); 
        u_mod_data[j] = u_mod_data[j] + sigma;

        //EVALUACIÓN 2: //F(u+sigma)
        func(u_mod, f_jac_mod, user_data); 

        //CÁLCULO DE LAS COLUMNAS DEL JACOBIANO
        for (int i = 0; i < fil; i++) {
            SM_ELEMENT_D(J, i, j) = (f_jac_mod_data[i] - f_jac_data[i]) / sigma;
        }
    }

    N_VDestroy(f_jac_mod);
    N_VDestroy(u_mod);

    return 0;

}




////////// FUNCIONES PARA IMPRIMIR LOS DISTINTOS VALORES DE LOS DISTINTOS VECTORES

// Imprime los nombre de las variables, parametros (hash_table_variables) y propiedades (hash_table_propiedades)
void print_table(){

    printf("TABLA DE VARIABLES\n");

    for(int i=0; i < table_size;i++ ){
        if(hash_table_variables[i]==NULL){
            printf("\t%i\t----\n", i);
        }
        else{
            printf("\t%i\t", i);
            identificador *tmp = hash_table_variables[i];
            while(tmp != NULL){
                printf("%s --",tmp ->name);
                tmp = tmp ->next;
            }
            printf("\n");
        }
    }
    printf("Final de tabla\n"); 
    
    printf("TABLA PARA PROPIEDADES\n");

    for(int i=0; i < table_size; i++){
        if(hash_table_propiedades[i] == NULL){
            printf("\t%i\t----\n", i);
        }
        else{
            printf("\t%i\t", i);
            propiedades *tmp = hash_table_propiedades[i];
            printf("%s --, VALUE:%f, LOWER:%f, UPPER:%f",tmp ->type, tmp->value, tmp->lower, tmp->upper);
            printf("\n");
        }
    }
    printf("Final de tabla\n");
}    

//imprime todos los valores (value, upper y lower) de todas las variables

void imprime_vector_variables(){
    for(int i=0; i < index_variable; i++){
        printf("Los valores de la variable %d - %s, son value: %.14f, upper: %f, lower: %f\n", i, vector_variables[i]->name, vector_variables[i]->value, vector_variables[i]->upper, vector_variables[i]->lower);
    }
}

void imprime_vector_parametros(){
    for(int i=0; i < index_parameter; i++){
        printf("El valor del parametro %d - %s, es value: %f\n", i, vector_parametros[i]->name, vector_parametros[i]->value);
    }
}


void imprime_ecs_en_var(){
    int cont = 0;
    for (int i = 0; i < num_incognitas; i++) {
        printf("Columna %d - Variable: %s\n", i, vector_incognitas[i]->name);
        
        char buffer[1024] = ""; 
        char temp[32];

        for (int j = 0; j < vector_incognitas[i]->num_ec; j++) {

            snprintf(temp, sizeof(temp), "%d ", vector_incognitas[i]->ecuaciones[j]->indice);
            strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
            cont++;
        }
        printf("Filas: %s\n", buffer);
    }
    printf("Elementos no nulos en jacobiano: %d\n", cont);
    printf("Elementos no nulos en jacobiano teniendo en cuenta restricciones: %d\n", cont + num_incognitas*2);
}


