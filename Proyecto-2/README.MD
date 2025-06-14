Para ejecutar esta vara:

Instale

gcc y make

Ejecute este comando en el directorio raíz:

make

Puede correr las pruebas así:

make test-blocks

Las pruebas deberían crear el directorio test_blocks si no existe e inicializar el gestor de bloques con 10 bloques de prueba.

Pruebas de Asignación:
Asignación exitosa de bloques
Verificación del estado de los bloques (libre/ocupado)
Reutilización de bloques liberados
Pruebas de Escritura/Lectura:
Escritura de datos de prueba en bloques
Lectura y verificación de los datos escritos
Actualización de datos en bloques existentes
Pruebas de Límites:
Manejo correcto cuando no hay bloques libres

Para limpiar los archivos generados:

make clean

Eso elimina:

Archivos objeto (.o)
Ejecutables de prueba
Directorio de pruebas test_blocks/

Notas Importantes
El sistema usa archivos .dat para simular bloques de almacenamiento.
Los bloques se almacenan en el directorio test_blocks/.
El tamaño de bloque predeterminado es de 4096 bytes.
El sistema está diseñado para manejar hasta 10 bloques en modo de prueba.