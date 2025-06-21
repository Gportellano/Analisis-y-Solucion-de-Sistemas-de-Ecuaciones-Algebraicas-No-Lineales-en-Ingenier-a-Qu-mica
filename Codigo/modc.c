/****************************************************************/
// MODANA_0.c
// Model Analyzer main program 
/****************************************************************/  

#include <stdio.h>
#include <stdlib.h>
//#include <stdarg.h> /*en este momento no se neceista esta librería solo cuando se haga lo de argumentos variables en funciones*/

#include <kinsol/kinsol.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_types.h>   /* defs. of sunrealtype, sunindextype */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix       */

#include "hmod.h"

#define FTOL SUN_RCONST(1.e-5) /* function tolerance */
#define STOL SUN_RCONST(1.e-5) /* step tolerance     */
#define ONE    SUN_RCONST(1.0)
#define ZERO   SUN_RCONST(0.0) 

void yyerror(char*);

extern FILE* yyin;
extern int yyparse();
extern char* yytext;

extern int lineno;
extern char linebuf[];
extern int tokenpos;
N_Vector scale;

int main(int argc,char* argv[])
{
    // extern int yydebug;  yydebug = 1;  

    if (argc > 1) /*he cambiado argv por argc*/
    {
        FILE* fich;
        fich = fopen(argv[1], "r");
        if (!fich) 
        {
            fprintf(stderr, "ERROR: No se puede abrir %s\n", argv[1]);
            exit(1);
        }
        yyin = fich;
    }
    init_hash_table();
    yyparse();
    printf("POSICION 0: el yyparse ha termiando\n");

    create_vec_incog();
    
    printf("POSICION 1\n");

    SUNContext sunctx;
    sunrealtype fnormtol, scsteptol;
    int glstr, mset, mxiter, retval;
    SUNMatrix J;
    SUNLinearSolver LS;
    N_Vector u, u1, constraints;

    mset = 1;
    mxiter = 1000;
    fnormtol  = FTOL;
    scsteptol = STOL;
    SUNContext_Create(SUN_COMM_NULL, &sunctx);

    printf("POSICION 2\n");

    int n_var = num_incognitas*3; // nºvariables + 2*nºvariables (limite superior e inferior)
    int n_ec = num_equations+2*num_incognitas;
    printf("Numero de ecuaciones declaradas: %ld\n", num_equations);
    printf("Numero de ecuaciones a resolver:%d ecuaciones + 2 * %d incognitas = %d\n", num_equations, num_incognitas, n_ec);

    u = N_VNew_Serial(n_var, sunctx);
    u1 = N_VNew_Serial(n_var, sunctx);
    scale = N_VNew_Serial(n_var, sunctx);
    constraints = N_VNew_Serial(n_var, sunctx);

    N_VConst(ONE, scale); //sin escalado
    N_VConst(ZERO, constraints); //sin restricciones
    
    /*
    for(int i = num_equations; i < 3*num_equations; i = i+2){
        NV_Ith_S(constraints, i) = ONE;
        NV_Ith_S(constraints, i+1) = -ONE;
    }
    */

    printf("POSICION 3: set initial guess\n");

    //imprime_vector_variables();

    set_initial_guess(u1);

    N_VScale(ONE, u1, u);

    /*
    sunrealtype* u_data = N_VGetArrayPointer(u);
    sunrealtype* scale_data = N_VGetArrayPointer(scale);

    if (u_data == NULL || scale_data == NULL) {
    fprintf(stderr, "Error obteniendo punteros internos\n");
    return 1;
    }
    for (int i = 0; i < n; i++) {
    scale_data[i] = fabs(1/u_data[i]);

        // Para evitar escalas cero o demasiado pequeñas, un mínimo:
        if (scale_data[i] < 1e-6) {
            scale_data[i] = 1e-6;
            }
    }
    for(int i = 0; i<n; i++){
        printf("s[%d] = %f\n", i, scale_data[i]);
    }
    */
    /*
    sunrealtype* udata = N_VGetArrayPointer(u);
    for (int i = 0; i < 3*num_equations; i++) {
    printf("u[%d] = %f\n", i, udata[i]);
    }
    */

    printf("POSICION 4\n");

    void *kmem = KINCreate(sunctx);
    if (kmem == NULL) {
        fprintf(stderr, "KINCreate failed.\n");
        return 1;
    }

    printf("POSICION 5\n");
   
    //KINSetUserData(kin_mem, NULL);   
    KINSetConstraints(kmem, constraints);    
    KINSetFuncNormTol(kmem, fnormtol);
    KINSetScaledStepTol(kmem, scsteptol);
    KINSetMaxSetupCalls(kmem, mset);
    //KINSetNumMaxIters(kmem, mxiter);

    printf("POSICION 6\n");

    if (KINInit(kmem, func, u) != KIN_SUCCESS) {
        fprintf(stderr, "KINInit error.\n");
        return 1;
    }

    printf("POSICION 7\n");

    J = SUNDenseMatrix(n_ec, n_var, sunctx); 
    LS = SUNLinSol_Dense(u, J, sunctx); 
    KINSetLinearSolver(kmem, LS, J);
    //KINSetJacFn(kmem, jac);

    printf("POSICION 8\n");

    int flag = KINSol(kmem, u, KIN_LINESEARCH, scale, scale);  
    

    printf("POSICION 9\n");
    FILE *info;
    info = fopen("informe.txt", "w+");

    FILE *cavett;
    cavett = fopen("sol_cavett", "w+");  
    
    if (flag == KIN_SUCCESS) {
        printf("Solucion encontrada:\n"); 
        printf("Numero de iteraciones %d\n", n_iter);
        for (long int i = 0; i < num_incognitas; i++) {
            fprintf(cavett, "x[%d] = %s = %g\n", i, vector_incognitas[i]->name, NV_Ith_S(u, i));
        }
    } else {
        printf("KINSOL falla en = %d\n", flag);
    }
    
    retval = KINPrintAllStats(kmem, info, SUN_OUTPUTFORMAT_TABLE);
    int lflag = SUNLinSolLastFlag(LS);
    printf("Ultima bandera en %d\n\n", lflag);
    
    /*

    FILE *jaco;
    jaco = fopen("jacob.txt", "w+");
    SUNDenseMatrix_Print(J, jaco);

    */

    //imprime_vector_variables();
    //imprime_ecs_en_var();
    //print_table(); //imprime las hash tables
    //imprime_vector_parametros();

    // Free memory
    N_VDestroy(u);
    N_VDestroy(scale);
    N_VDestroy(constraints);
    KINFree(&kmem);
    //fclose(jaco);
    fclose(cavett);
    fclose(info);
    return 0;
}

void yyerror(char* errmsg)
{
    extern int yynerrs;
    /*	fprintf(stderr, "%s\n", errmsg); */
    printf("[error %d] linea %d: %s cerca de %s\n", yynerrs + 1, lineno, errmsg, yytext);
    printf("\n%s\n", linebuf);
    printf("%*s\n", tokenpos, "^");
}
