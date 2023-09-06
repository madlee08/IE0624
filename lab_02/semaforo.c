typedef enum estados {paso_vehiculo, esperar, parpadear_vehiculo, detener_vehiculos, paso_peaton, parpadear_peaton, detener_peaton} estados;

void main() {
    estados estado = paso_vehiculo;

    switch (estado) {
        case estado: {insertar codigo} break;
        
        case paso_peaton:
            if (diez_segundos) {
                estado = parpadear_peaton;
            }
        break;

        case parpadear_peaton:
            if (tres_segundos) {
                estado = detener_peaton;
            }
        break;

        case detener_peaton:
            if (un_segundo) {
                estado = paso_vehiculo;
            }
        break;
    }
}