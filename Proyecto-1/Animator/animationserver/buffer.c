// ============= SERVIDOR - Envío Confiable =============

// Función para envío confiable con reintentos
int reliable_send(int socket, const char* data, size_t len) {
    size_t total_sent = 0;
    int attempts = 0;
    const int max_attempts = 3;
    
    while(total_sent < len && attempts < max_attempts) {
        int sent = send(socket, data + total_sent, len - total_sent, MSG_DONTWAIT);
        
        if(sent < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // Buffer lleno, esperar un poco
                usleep(1000); // 1ms
                attempts++;
                continue;
            } else {
                // Error real de conexión
                return -1;
            }
        } else if(sent == 0) {
            // Conexión cerrada
            return -1;
        }
        
        total_sent += sent;
        attempts = 0; // Reset attempts on successful send
    }
    
    return (total_sent == len) ? 0 : -1;
}

// CORRECCIÓN: Envío por lotes para reducir overhead
void send_print_batched(Canvas *canvas) {
    pthread_mutex_lock(&canvas_send_mutex);
    
    // Crear buffer por monitor para envío en lotes
    char monitor_buffers[MAX_MONITORS][MAX_MONITORS][4096];
    int buffer_lengths[MAX_MONITORS][MAX_MONITORS];
    
    // Inicializar buffers
    for(int mi = 0; mi < canvas->monitors_height; mi++) {
        for(int mj = 0; mj < canvas->monitors_width; mj++) {
            monitor_buffers[mi][mj][0] = '\0';
            buffer_lengths[mi][mj] = 0;
        }
    }
    
    // Construir lotes de comandos por monitor
    for(int i = 0; i < canvas->height; i++) {
        for(int j = 0; j < canvas->width; j++) {
            int monitor_row = i / MAX_MONITOR_HEIGHT;
            int monitor_col = j / MAX_MONITOR_WIDTH;
            
            if(monitor_row >= canvas->monitors_height || 
               monitor_col >= canvas->monitors_width ||
               canvas->monitors[monitor_row][monitor_col] == NULL) {
                continue;
            }
            
            int local_row = (i % MAX_MONITOR_HEIGHT) + 1;
            int local_col = (j % MAX_MONITOR_WIDTH) + 1;
            
            char command[32];
            int cmd_len = sprintf(command, "PRINT:%d,%d,%c;", 
                                local_row, local_col, canvas->canvas_drawing[i][j]);
            
            // Verificar espacio en buffer
            if(buffer_lengths[monitor_row][monitor_col] + cmd_len < 4000) {
                strcat(monitor_buffers[monitor_row][monitor_col], command);
                buffer_lengths[monitor_row][monitor_col] += cmd_len;
            } else {
                // Buffer lleno, enviar y resetear
                Monitor *monitor = canvas->monitors[monitor_row][monitor_col];
                if(reliable_send(monitor->socket, 
                                monitor_buffers[monitor_row][monitor_col], 
                                buffer_lengths[monitor_row][monitor_col]) < 0) {
                    printf("Error enviando lote a monitor %d\n", monitor->id);
                }
                
                // Resetear buffer y agregar comando actual
                strcpy(monitor_buffers[monitor_row][monitor_col], command);
                buffer_lengths[monitor_row][monitor_col] = cmd_len;
            }
        }
    }
    
    // Enviar buffers restantes
    for(int mi = 0; mi < canvas->monitors_height; mi++) {
        for(int mj = 0; mj < canvas->monitors_width; mj++) {
            if(canvas->monitors[mi][mj] != NULL && buffer_lengths[mi][mj] > 0) {
                Monitor *monitor = canvas->monitors[mi][mj];
                if(reliable_send(monitor->socket, 
                                monitor_buffers[mi][mj], 
                                buffer_lengths[mi][mj]) < 0) {
                    printf("Error enviando lote final a monitor %d\n", monitor->id);
                }
            }
        }
    }
    
    pthread_mutex_unlock(&canvas_send_mutex);
}

// CORRECCIÓN: Move object optimizado
void move_object_patched(Object* obj, Canvas* canvas) {
    int move_flag = 1;
    
    while(move_flag) {
        int step_x = 0, step_y = 0;

        // Calcular paso
        if(obj->destined_x > obj->x) step_x = 1;
        else if(obj->destined_x < obj->x) step_x = -1;
        
        if(obj->destined_y > obj->y) step_y = 1;
        else if(obj->destined_y < obj->y) step_y = -1;

        // Modificar canvas con protección
        pthread_mutex_lock(&canvas_send_mutex);
        
        erase_object(obj, canvas);
        
        if(step_x != 0) obj->x += step_x;
        if(step_y != 0) obj->y += step_y;
        
        if(obj->rotations > 0) rotate(obj, obj->rotations);
        
        draw_object(obj, canvas);
        
        pthread_mutex_unlock(&canvas_send_mutex);
        
        // Enviar frame completo en lotes
        send_print_batched(canvas);
        
        move_flag = (obj->destined_x != obj->x) || (obj->destined_y != obj->y);
        
        // Frame rate control - 20 FPS
        usleep(50000); // 50ms = 20 FPS
    }
}

// ============= CLIENTE - Recepción Confiable =============

#define RECV_BUFFER_SIZE 8192
#define ACCUMULATOR_SIZE 16384

void animation_cicle(int client_fd) {
    char recv_buffer[RECV_BUFFER_SIZE];
    char accumulator[ACCUMULATOR_SIZE] = {0};
    int accumulator_len = 0;
    
    // Configurar socket como no-bloqueante para mejor control
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Limpiar pantalla
    printf("\033[2J\033[H");
    fflush(stdout);
    
    fd_set read_fds;
    struct timeval timeout;
    
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        
        timeout.tv_sec = 1;  // 1 segundo timeout
        timeout.tv_usec = 0;
        
        int activity = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if(activity < 0) {
            perror("select error");
            break;
        } else if(activity == 0) {
            // Timeout - continuar esperando
            continue;
        }
        
        if(FD_ISSET(client_fd, &read_fds)) {
            int valread = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE - 1, 0);
            
            if(valread <= 0) {
                if(valread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue; // No hay datos, continuar
                }
                printf("Conexión perdida\n");
                break;
            }
            
            recv_buffer[valread] = '\0';
            
            // Verificar espacio en accumulator
            if(accumulator_len + valread >= ACCUMULATOR_SIZE - 1) {
                printf("Buffer overflow - reseteando\n");
                accumulator_len = 0;
                accumulator[0] = '\0';
            }
            
            // Agregar datos recibidos
            strcat(accumulator, recv_buffer);
            accumulator_len += valread;
            
            // Procesar comandos completos
            char *cmd_start = accumulator;
            char *cmd_end;
            int commands_processed = 0;
            
            while((cmd_end = strchr(cmd_start, ';')) != NULL) {
                *cmd_end = '\0';
                
                int row, col;
                char c;
                
                if(sscanf(cmd_start, "PRINT:%d,%d,%c", &row, &col, &c) == 3) {
                    if(row > 0 && col > 0 && row <= 50 && col <= 200) {
                        printf("\033[%d;%dH%c", row, col, c);
                        commands_processed++;
                    }
                } else if(strncmp(cmd_start, "END", 3) == 0) {
                    printf("\nAnimación terminada\n");
                    fflush(stdout);
                    return;
                }
                
                cmd_start = cmd_end + 1;
            }
            
            // Flush después de procesar lote
            if(commands_processed > 0) {
                fflush(stdout);
            }
            
            // Mover datos restantes al inicio del buffer
            if(*cmd_start) {
                accumulator_len = strlen(cmd_start);
                memmove(accumulator, cmd_start, accumulator_len + 1);
            } else {
                accumulator_len = 0;
                accumulator[0] = '\0';
            }
        }
    }
    
    printf("\nConexión terminada\n");
}

// No olvides agregar estos headers al cliente:
// #include <fcntl.h>
// #include <sys/select.h>
// #include <errno.h>