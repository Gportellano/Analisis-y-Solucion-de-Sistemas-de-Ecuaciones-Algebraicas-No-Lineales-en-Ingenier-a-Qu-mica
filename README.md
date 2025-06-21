# Analisis-y-Solucion-de-Sistemas-de-Ecuaciones-Algebraicas-No-Lineales-en-Ingenieria-Quimica

Los sistemas de ecuaciones algebraicas no lineales son una problem´atica t´ıpica dentro del contexto
de la ingenier´ıa qu´ımica. La no convergencia de algunos programas solucionadores de dichos
sistemas, como Aspen Custom Modeler (ACM), frena el avance del desarrollo de modelos de
ingenier´ıa.
En este trabajo se desarrolla el c´odigo necesario para poder, a partir de un fichero de ACM,
llamar al paquete KINSOL, que pertenece a la librer´ıa matem´atica SUNDIALS. Para realizar
dicha llamada se desarrolla un compilador a partir de las herramientas Flex, Bison y c´odigo C,
que de forma conjunta permiten crear las estructuras de datos que contienen la informaci´on de
los archivos de ACM.
Adicionalmente, se introduce otra herramienta clave para la resoluci´on de estos sistemas, la
descomposici´on por bloques o descomposici´on Dulmage-Mendelsohn. Esta permite, adem´as de
la detecci´on de errores en el propio sistema, la divisi´on en subsistemas del conjunto, lo cual
agiliza el c´alculo de soluciones.
La capacidad del programa desarrollado y la de KINSOL se eval´uan al final con varios sistemas
de ecuaciones, destacando el modelo de ingenier´ıa qu´ımica de Cavett, un sistema de 187
ecuaciones. Con este problema, se ha confirmado el buen funcionamiento del programa, pero
tambi´en el amplio margen de mejora que queda para poder hacer de KINSOL una herramienta
verdaderamente eficaz en este contexto. Las dificultades encontradas, como la sensibilidad a las
condiciones iniciales o la falta de robustez de KINSOL, evidencian limitaciones actuales, aunque
abordables en el futuro.
