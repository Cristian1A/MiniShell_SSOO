# MiniShell_SSOO, GIS, 2023/2024, URJC

# Autores
   ·Limaylla Ticlavilca, David Paúl (GIS)
   ·Vlad, Cristian Andrei (GIS)
## Descripción

En esta práctica se abordará el problema de implementar un programa que actúe como intérprete de mandatos. 
El minishell a implementar debe interpretar y ejecutar mandatos leyéndolos de la entrada estándar. En definitiva, debe ser capaz de:

  • Ejecutar una secuencia de uno o varios mandatos separados por el carácter ‘|’.
  
  • Permitir redirecciones:
  
    o Entrada: < fichero. Sólo puede realizarse sobre el primer mandato del pipe.
    
    o Salida: > fichero. Sólo puede realizarse sobre el último mandato del pipe.
    
    o Error: >& fichero. Sólo puede realizarse sobre el último mandato del pipe.
    
  • Permitir la ejecución en background de la secuencia de mandatos si termina con el carácter ‘&’. Para ello, el minishell debe mostrar el pid del proceso por el que espera entre corchetes, y no bloquearse por la ejecución de dicho mandato (es decir, no debe esperar a su terminación para mostrar el prompt).

A grandes rasgos, el programa tiene que hacer lo siguiente:

  • Mostrar en pantalla un prompt (los símbolos msh> seguidos de un espacio).
  
  • Leer una línea del teclado.
  
  • Analizarla utilizando la librería parser (ver apéndice).
  
  • Ejecutar todos los mandatos de la línea a la vez creando varios procesos hijo y comunicando unos con otros con las tuberías que sean necesarias, y realizando las redirecciones que sean necesarias. En caso de que no se ejecute en background, se espera a que todos los mandatos hayan finalizado para volver a mostrar el prompt y repetir el proceso.
  
Teniendo en cuenta lo siguiente:

  • Si la línea introducida no contiene ningún mandato o se ejecuta el mandato en background, se volverá a mostrar el prompt a la espera de una nueva línea.
  
  • Si alguno de los mandatos a ejecutar no existe, el programa debe mostrar el error “mandato: No se encuentra el mandato”.
  
  • Si se produce algún error al abrir cualquiera de los ficheros de las redirecciones, debe mostrarse el error “fichero: Error. Descripción del error”.
  
  • Ni el minishell ni los procesos en background deben finalizar al recibir las señal desde teclado SIGINT (Ctrl-C) mientras que los procesos que se lancen deben actuar ante ella, manteniendo la acción por defecto. 

## Objetivos parciales

• Ser capaz de reconocer y ejecutar en foreground líneas con un solo mandato y 0 o más argumentos. (0,5 puntos)

• Ser capaz de reconocer y ejecutar en foreground líneas con un solo mandato y 0 o más argumentos, redirección

de entrada estándar desde archivo y redirección de salida a archivo. (1 punto)

• Ser capaz de reconocer y ejecutar en foreground líneas con dos mandatos con sus respectivos argumentos,

enlazados con ‘|’, y posible redirección de entrada estándar desde archivo y redirección de salida a archivo. (1

punto)

• Ser capaz de reconocer y ejecutar en foreground líneas con más de dos mandatos con sus respectivos

argumentos, enlazados con ‘|’, redirección de entrada estándar desde archivo y redirección de salida a archivo.

(2,5 puntos)

• Ser capaz de ejecutar el mandato cd (0,5 puntos). Mientras que la mayoría de los mandatos son programas

del sistema, cd es un mandato interno que debe ofrecer el propio intérprete de mandatos. El mandato cd debe

permitir tanto el acceso a través de rutas absolutas como relativas, además de la posibilidad de acceder al

directorio especificado en la variable HOME si no recibe ningún argumento, escribiendo la ruta absoluta del

nuevo directorio actual de trabajo. Para el correcto cambio de directorio el mandato cd se debe ejecutar sin

pipes.

• Ser capaz de reconocer y ejecutar tanto en foreground como en background líneas con más de dos mandatos

con sus respectivos argumentos, enlazados con ‘|’, redirección de entrada estándar desde archivo y

redirección de salida a archivo. Para su correcta demostración, se deben realizar los mandatos internos jobs

y fg (2 puntos):

o jobs: Muestra la lista de procesos que se están ejecutando en segundo plano en la minishell (no es

necesario considerar aquellos procesos pausados con Ctrl-D). El formato de salida será similar al del

mandato jobs del sistema:

[1]+ Running find / -name hola | grep h &

o fg: Reanuda la ejecución del proceso en background identificado por el número obtenido en el mandato

jobs, indicando el mandato que se está ejecutando. Reorienta su entrada estándar y salidas estándar a la

terminal del usuario. Si no se le pasa ningún identificador se pasa a foreground el último mandato en

background introducido.

• Evitar que los comandos lanzados en background y el minishell mueran al enviar la señal desde el teclado

SIGINT, mientras los procesos en foreground respondan ante ella. La minishell deberá mostrar una nueva línea

y un nuevo prompt cuando reciba dicha señal (1 punto)

• Ser capaz de ejecutar el mandato exit (0,5 puntos). exit es un mandato interno que cuando se ejecuta la

minishell termina de manera ordenada y se muestra el prompt de la Shell desde dónde se ejecutó.

• Ser capaz de ejecutar el mandato umask (1 punto). La máscara del modo de creación de un fichero de usuario

(umask) se usa para determinar los permisos de un fichero cuando se crea nuevo. Por tanto, este mandato se

utiliza para establecer dichos permisos por defecto para ficheros nuevos. El mandato admite dos formatos.

Para esta práctica sólo se pide que el mandato programado acepte un número octal (por ejemplo, 0174). Los

permisos por defecto que se establezcan serán el resultado de la resta bitwise del número octal pasado como

argumento al mandato.

Nota: las puntuaciones para cada objetivo parcial son las puntuaciones máximas que se pueden obtener si se cumplen

esos objetivos.

Nota: no se debe hacer un programa separado para cada objetivo, sino un único programa genérico que cumpla con

todos los objetivos simultáneamente.

## Normas de estilo

La práctica se evaluará comprobando el correcto funcionamiento de los distintos objetivos, y valorando la simplicidad

del código, los comentarios, la óptima gestión de recursos, la gestión de errores y la calidad de la memoria. El profesor

podrá solicitar una defensa oral de la práctica si lo considerase necesario.

A la hora de codificar la solución pedida, se deberán respetar una serie de normas de estilo:

• Las variables locales de las funciones se declararán inmediatamente después de la llave de comienzo del

cuerpo de la misma. Se penalizará la declaración de variables entre sentencias ejecutables de las funciones.

• No se admitirán asignaciones iniciales a variables cuyo valor dependa del valor de otras variables. El valor

asignado en estos casos siempre deberá ser conocido en tiempo de compilación.

• Cuando se declare una variable de tipo array, su tamaño deberá ser conocido en tiempo de compilación. Si se

quiere utilizar un array de tamaño variable, deberá crearse en memoria dinámica mediante las funciones

correspondientes (malloc, calloc o realloc). La memoria dinámica solicitada deberá liberarse (free) antes de

salir del programa.

• Las operaciones sobre strings (copia, comparación, duplicación, concatenación, etc) se realizarán en lo posible

mediante las funciones indicadas en string.h (ver referencias a la biblioteca de C en el Campus Virtual).

• En general, se penalizará el uso de construcciones propias de C++.

• Al compilar el código fuente, deberá producirse el menor número posible de warnings (mejor que no se

produzca ninguno).

El incumplimiento de estas normas de estilo, así como de otras normas que puedan ser anunciadas por el profesor a

través del Campus Virtual, conllevará una penalización en la nota.

## Entrega de prácticas

La entrega de prácticas se hará a través del Campus Virtual en las fechas anunciadas en el mismo. Se debe entregar un
único archivo myshell.c con el código de toda la práctica, debidamente comentado, y una memoria de la misma en
formato PDF. La memoria debe realizarse partiendo de la plantilla WORD aportada en el aula virtual y debe incluir:

• Índice de contenidos

• Autores

• Descripción del código: incluyendo descripción de las principales funciones implementadas.

• Comentarios personales: incluyendo problemas encontrados, crítica constructiva, propuesta de mejoras y

evaluación del tiempo dedicado.

• No incluir código fuente

• NO DESCUIDE LA CALIDAD DE LA MEMORIA DE SU PRÁCTICA. Aprobar la memoria es tan imprescindible para

aprobar la práctica, como el correcto funcionamiento de la misma. Si al evaluar la memoria se considera que

no alcanza el mínimo admisible, la práctica se considerará SUSPENSA.

IMPORTANTE: Todos los archivos solicitados (fuentes y memoria) deberán empaquetarse en un único archivo

comprimido en formato ZIP llamado practica2.zip y entregarse a través del Campus Virtual pinchando en el enlace

correspondiente.

## Defensa oral de la práctica

Después de entregar la práctica, cada alumno será convocado de forma individual para realizar una defensa oral de
su práctica, en fecha y lugar indicados por el equipo docente de la asignatura.
En la defensa oral de la práctica, el alumno:

• Compilará y montará el ejecutable de su práctica desde la línea de mandatos de Linux.

• Ejecutará y demostrará el funcionamiento de su práctica, de acuerdo con las indicaciones del profesor.

• Responderá a cuestiones relativas al código fuente de su práctica.

La defensa oral de la práctica tiene influencia sobre la nota final obtenida en la misma, de forma que puede mejorar o

empeorar la nota parcial calculada a partir del cumplimiento de objetivos, las normas de estilo del código fuente y la

memoria. Si el desempeño del alumno en la defensa oral no alcanza el mínimo nivel admisible, la práctica en su

conjunto se considerará SUSPENSA, pudiéndose incluso solicitar al alumno que entregue una práctica totalmente

nueva para la siguiente convocatoria.

## Evaluación de la práctica

La nota de la práctica será individual para cada alumno, y se calculará a partir de:

• El cumplimiento de los objetivos planteados en la misma.

• La aplicación de las normas de estilo.

• La memoria.

• La defensa oral individual.

## Autoría de la práctica

La práctica se debe realizar en grupos de 2 personas como máximo.

El hecho de detectar copia en las prácticas expondrá a los alumnos a la posibilidad de una apertura de expediente
disciplinario y expulsión. En caso de detectar copia, los alumnos afectados serán suspendidos en la TOTALIDAD de la
asignatura. Una práctica será considerada copia en caso de que contenga la totalidad o una parte de la práctica de
otro alumno. Se considerará copia en caso de:

• Archivos que contengan la totalidad o fragmentos de código de otro alumno

• Memorias con la totalidad o fragmentos de frases e imágenes de otro alumno

El profesor podrá hacer uso de detectores automáticos de plagio en las prácticas, tanto en la parte referente al código

como a la memoria.

