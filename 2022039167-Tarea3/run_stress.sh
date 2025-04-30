# Ejecutar el script de stress
python3 src/stress.py -n 100 HTTPclient -h http://localhost:8080 GET /files/example.txt

python3 src/stress.py -n 10 HTTPclient -h http://localhost:8080 GET /files/package.deb