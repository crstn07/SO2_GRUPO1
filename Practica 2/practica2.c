#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
struct data_struct
{
    int cuenta;
    char nombre[50];
    double saldo;
};
struct data_struct usuarios[150];
int usuarios_size = 0;
// pthread_mutex_t lock;

struct thread_data
{
    cJSON *json_array;
    int inicio;
    int fin;
    int procesados;
    int cont_errores;
    char errores[150][100];
};

void print_json_object(char *json_string)
{
    cJSON *json = cJSON_Parse(json_string);
    char *formatted_json = cJSON_Print(json);
    printf("%s\n", formatted_json);
    cJSON_Delete(json);
    free(formatted_json);
}

void *carga_usuarios(void *arg)
{
    struct thread_data *data = (struct thread_data *)arg;
    cJSON *json_array = data->json_array;
    int inicio = data->inicio;
    int fin = data->fin;

    for (int i = inicio; i < fin; i++)
    {
        cJSON *item = cJSON_GetArrayItem(json_array, i);
        if (cJSON_IsObject(item))
        {
            cJSON *no_cuenta = cJSON_GetObjectItem(item, "no_cuenta");
            cJSON *nombre = cJSON_GetObjectItem(item, "nombre");
            cJSON *saldo = cJSON_GetObjectItem(item, "saldo");

            if (cJSON_IsNumber(no_cuenta) && cJSON_IsString(nombre) && cJSON_IsNumber(saldo))
            {
                int cuenta_duplicada = 0;

                for (int j = 0; j < usuarios_size; j++)
                {
                    if (usuarios[j].cuenta == no_cuenta->valueint)
                    {
                        cuenta_duplicada = 1;
                        break;
                    }
                }

                if (cuenta_duplicada)
                {
                    sprintf(data->errores[data->cont_errores], "Linea #%d (registro %d): Número de cuenta \"%d\" duplicado", 3 + i * 5, i + 1, no_cuenta->valueint);
                    data->cont_errores++;
                }
                else if (saldo->valuedouble < 0)
                {
                    sprintf(data->errores[data->cont_errores], "Linea #%d (registro %d): Saldo \"%.2f\" no puede ser menor a 0", 5 + i * 5, i + 1, saldo->valuedouble);
                    data->cont_errores++;
                }
                else
                {
                    usuarios[usuarios_size].cuenta = no_cuenta->valueint;
                    sprintf(usuarios[usuarios_size].nombre, "%s", nombre->valuestring);
                    usuarios[usuarios_size].saldo = saldo->valuedouble;
                    usuarios_size++;
                    data->procesados++;
                }
            }
            else if (!cJSON_IsNumber(no_cuenta))
            {
                sprintf(data->errores[data->cont_errores], "Linea #%d (registro %d): Número de cuenta \"%s\" no es un número entero positivo", 3 + i * 5, i + 1, no_cuenta->valuestring);
                data->cont_errores++;
            }
            else if (!cJSON_IsNumber(saldo))
            {
                sprintf(data->errores[data->cont_errores], "Linea #%d (registro %d): Saldo \"%s\" no es un número real positivo", 5 + i * 5, i + 1, saldo->valuestring);
                data->cont_errores++;
            }
        }
    }

    return NULL;
}

void leer_usuarios(char *filename, struct thread_data args[3])
{
    // Open the JSON file for reading
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error al abrir el archivo");
        return;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the entire file into a buffer
    char *buffer = (char *)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0';

    // Close the file
    fclose(file);

    // Parse the JSON data
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        perror("Error al parsear el JSON");
        return;
    }

    // Check if it's a valid JSON array
    if (!cJSON_IsArray(json))
    {
        perror("JSON no es un array");
        return;
    }

    int array_size = cJSON_GetArraySize(json);
    int segmento_size = array_size / 3;

    pthread_t threads[3];

    for (int i = 0; i < 3; i++)
    {
        args[i].json_array = json;
        args[i].inicio = i * segmento_size;
        args[i].fin = (i == 2) ? array_size : (i + 1) * segmento_size;
        args[i].procesados = 0;
        args[i].cont_errores = 0;
        pthread_create(&threads[i], NULL, carga_usuarios, &args[i]);
    }

    for (int i = 0; i < 3; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    cJSON_Delete(json);
    free(buffer);
}

char *estado_de_cuenta()
{
    // Create an empty JSON Array
    cJSON *json_array = cJSON_CreateArray();

    // Iterate through the structs
    for (int i = 0; i < usuarios_size; i++)
    {
        // Create an empty JSON Object
        cJSON *item = cJSON_CreateObject();

        // Add attributes to the object
        cJSON_AddNumberToObject(item, "no_cuenta", usuarios[i].cuenta);
        cJSON_AddStringToObject(item, "nombre", usuarios[i].nombre);
        cJSON_AddNumberToObject(item, "saldo", usuarios[i].saldo);

        // Add the object to the Array
        cJSON_AddItemToArray(json_array, item);
    }

    // Format the JSON Array
    char *formatted = cJSON_Print(json_array);
    cJSON_Delete(json_array); // Free the cJSON object
    return formatted;
}

void escribir_archivo(char *filename, char *data)
{
    // Open the file
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("Error al abrir el archivo");
        return;
    }

    // Write the data
    fwrite(data, 1, strlen(data), file);
    fclose(file);
}

void log_carga(struct thread_data args[3])
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char filename[100];
    strftime(filename, sizeof(filename), "carga_%Y_%m_%d-%H_%M_%S.log", t);

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error al escribir el archivo de log de carga");
        return;
    }

    fprintf(file, "-------------------- Carga de usuarios --------------------\n\n");
    fprintf(file, "Fecha: %04d-%02d-%02d %02d:%02d:%02d\n\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    int total_users = 0;
    fprintf(file, "Usuarios Cargados:\n");
    for (int i = 0; i < 3; i++)
    {
        fprintf(file, "Hilo #%d: %d\n", i + 1, args[i].procesados);
        total_users += args[i].procesados;
    }
    fprintf(file, "Total: %d\n\n", total_users);

    fprintf(file, "Errores:\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < args[i].cont_errores; j++)
        {
            fprintf(file, "  - %s\n", args[i].errores[j]);
        }
    }

    fclose(file);
}


void limpiar_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char *argv[])
{
    int opcion;
    char ruta_archivo[1000];
    int cuenta1;
    int cuenta2;
    double monto;

    // // Carga de usuarios al inicio de la aplicación
    // printf("Ingrese la ruta del archivo para la carga masiva de usuarios: ");
    // limpiar_buffer();
    // fgets(ruta_archivo, sizeof(ruta_archivo), stdin);
    // ruta_archivo[strcspn(ruta_archivo, "\n")] = '\0'; // Elimina el carácter de nueva línea
    struct thread_data args[3];
    // printf("Ruta: %s\n", ruta_archivo);
    leer_usuarios("usuarios.json", args);
    log_carga(args);
    printf("\n > Usuarios cargados correctamente.\n");

    // sprintf(usuarios[0].nombre, "Cristian Pereira");
    // usuarios[0].saldo = 100.22;

    do
    {
        printf("\n+------------------------+");
        printf("\n|    Menú de opciones    |");
        printf("\n+------------------------+");
        printf("\n| 1. Depósito            |");
        printf("\n| 2. Retiro              |");
        printf("\n| 3. Transacción         |");
        printf("\n| 4. Consultar Cuenta    |");
        printf("\n| 5. Cargar Operaciones  |");
        printf("\n| 6. Estado de Cuentas   |");
        printf("\n| 7. Salir.              |");
        printf("\n+------------------------+");
        printf("\n > Introduzca opcion (1-7): ");

        if (scanf("%d", &opcion) != 1)
        {
            printf("\nError: Ingrese un número entero válido.\n");
            limpiar_buffer(); // Limpia el buffer de entrada
            continue;
        }
        switch (opcion)
        {
        case 1:
            printf("\n---------- Deposito ----------");
            printf("\n > Numero de cuenta: ");
            scanf("%d", &cuenta1);
            printf(" > Monto a Transferir: ");
            scanf("%lf", &monto);
            // deposito(cuenta1, monto, 0);
            break;

        case 2:
            printf("\n---------- Retiro ----------");
            printf("\n > Numero de cuenta: ");
            scanf("%d", &cuenta1);
            printf(" > Monto a debitar: ");
            scanf("%lf", &monto);
            // retiro(cuenta1, monto, 0);
            break;

        case 3:
            printf("\n---------- Transferencia ----------");
            printf("\n > Numero de cuenta a debitar: ");
            scanf("%d", &cuenta1);
            printf("\n > Numero de cuenta a creditar: ");
            scanf("%d", &cuenta2);
            printf(" > Monto a debitar: ");
            scanf("%lf", &monto);
            // transferencia(cuenta1, cuenta2, monto, 0);
            break;

        case 4:
            printf("\n---------- Consultar Cuenta ---------- ");
            printf("\n > Numero de cuenta: ");
            scanf("%d", &cuenta1);
            // consultar_cuenta(cuenta1);
            break;
        case 5:
            printf("Ingrese la ruta del archivo de transacciones: ");
            limpiar_buffer(); // Limpia el buffer de entrada antes de leer la cadena
            fgets(ruta_archivo, sizeof(ruta_archivo), stdin);
            ruta_archivo[strcspn(ruta_archivo, "\n")] = '\0'; // Elimina el carácter de nueva línea
            // cargar_transacciones(ruta_archivo);
            break;
        case 6:
            printf("\n > Se creó el archivo de Estado de Cuentas \"estado_de_cuentas.json\"\n");
            // ESTADO DE CUENTAS
            char *new_json = estado_de_cuenta();
            escribir_archivo("estado_de_cuentas.json", new_json);
            free(new_json);
            break;
        case 7:
            printf("\n > Sistema Finalizado\n");
            exit(0);        
            break;
        default:
            printf("\n > Opcion Invalida");
            break;
        }

    } while (opcion != 8);
    return 0;
}
