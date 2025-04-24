# Compilar el servidor
gcc -o bin/prethread-WebServer src/prethread_webserver.c -lpthread

# Compilar el cliente
gcc -o bin/HTTPclient src/http_client.c -lcurl

# Ejecutar el servidor
./bin/prethread-WebServer -n 5 -w /var/www/html -p 8080

# Ejecutar el cliente
./bin/HTTPclient -h http://localhost:8080

# Ejecutar el script de stress
python3 src/stress.py -n 10 HTTPclient -h http://localhost:8080