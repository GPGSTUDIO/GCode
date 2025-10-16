#include <stdio.h>  
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#define MAX_VARIABLES 100
#define MAX_NAME_LENGTH 100
#define MAX_VALUE_LENGTH 10000

// Глобальный флаг для break
int break_flag = 0;

const unsigned char PATTERNGCODE[] = "###CODE###";
const size_t PATTERNGCODE_LENGTH = sizeof(PATTERNGCODE) - 1;

int evaluate_simple_expression(const char* expr) {
    int result = 0;
    int current = 0;
    char op = '+';
    int i = 0;
    
    while (expr[i] != '\0') {
        if (expr[i] >= '0' && expr[i] <= '9') {
            current = current * 10 + (expr[i] - '0');
        } else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            // Применяем предыдущую операцию
            switch (op) {
                case '+': result += current; break;
                case '-': result -= current; break;
                case '*': result *= current; break;
                case '/': if (current != 0) result /= current; break;
            }
            op = expr[i];
            current = 0;
        }
        i++;
    }
    
    // Применяем последнюю операцию
    switch (op) {
        case '+': result += current; break;
        case '-': result -= current; break;
        case '*': result *= current; break;
        case '/': if (current != 0) result /= current; break;
    }
    
    return result;
}

long find_PATTERNGCODE_from_end(const unsigned char *buffer, long buffer_size) {
    // Начинаем поиск с конца файла
    for (long i = buffer_size - PATTERNGCODE_LENGTH; i >= 0; i--) {
        if (memcmp(&buffer[i], PATTERNGCODE, PATTERNGCODE_LENGTH) == 0) {
            return i; // возвращаем позицию первого найденного с конца
        }
    }
    return -1; // не найдено
}

typedef struct {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} Variable;

Variable variables[MAX_VARIABLES];
int variable_count = 0;

Variable* find_variable(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}

void set_variable(const char* name, const char* value) {
    Variable* var = find_variable(name);
    
    if (var != NULL) {
        strncpy(var->value, value, MAX_VALUE_LENGTH - 1);
        var->value[MAX_VALUE_LENGTH - 1] = '\0';
    } else if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, MAX_NAME_LENGTH - 1);
        variables[variable_count].name[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(variables[variable_count].value, value, MAX_VALUE_LENGTH - 1);
        variables[variable_count].value[MAX_VALUE_LENGTH - 1] = '\0';
        variable_count++;
    } else {
        printf("Invalid syntax: Too many variables!\n");
    }
}

void initialize_args_variables(int argc, char** argv) {
    char var_name[20];
    
    for (int i = 0; i < argc && i < 10; i++) {
        snprintf(var_name, sizeof(var_name), "__ARGV-%d__", i);
        set_variable(var_name, argv[i]);
    }
    
    // Также создаем переменную с общим количеством аргументов
    set_variable("__ARGC__", "0"); // Будет строкой, нужно преобразовать
    // Преобразуем число в строку
    char argc_str[10];
    snprintf(argc_str, sizeof(argc_str), "%d", argc);
    set_variable("__ARGC__", argc_str);
}

void ask_gcode(char GCODEMAIN[10000]) {
if (strncmp(GCODEMAIN, "print ", 6) == 0) {
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    if (strncmp(content, "<FUNC>", 6) == 0) {
        char* expression = content + 6;
        int result = 0;
        
        if (strncmp(expression, "<VAR>", 5) == 0) {
            const char* var_name = expression + 5;
            Variable* var = find_variable(var_name);
            
            if (var != NULL) {
                result = evaluate_simple_expression(var->value);
            } else {
                printf("Variable not found: %s", var_name);
            }
        } else {
            result = evaluate_simple_expression(expression);
        }
        
        printf("%d", result);
    }
    else if (strncmp(content, "<VAR>", 5) == 0) {
        Variable* var = find_variable(content + 5);
        if (var != NULL) {
            printf("%s", var->value);
        } else {
            printf("Variable not found: %s", content + 5);
        }
    }
    else {
        printf("%s", content);
    }
    printf("\n");
} else if (strncmp(GCODEMAIN, "print_raw ", 10) == 0) {
    char* content = GCODEMAIN + 10;
    content[strcspn(content, "\n")] = '\0';
    
    if (strncmp(content, "<FUNC>", 10) == 0) {
        char* expression = content + 10;
        int result = 0;
        
        if (strncmp(expression, "<VAR>", 5) == 0) {
            const char* var_name = expression + 5;
            Variable* var = find_variable(var_name);
            
            if (var != NULL) {
                result = evaluate_simple_expression(var->value);
            } else {
                printf("Variable not found: %s", var_name);
            }
        } else {
            result = evaluate_simple_expression(expression);
        }
        
        printf("%d", result);
    } else if (strncmp(content, "<VAR>", 5) == 0) {
        Variable* var = find_variable(content + 5);
        if (var != NULL) {
            printf("%s", var->value);
        } else {
            printf("Variable not found: %s", content + 5);
        }
    }
    else {
        printf("%s", content);
    }
} else if (strncmp(GCODEMAIN, "getsymbol ", 10) == 0) {
    char* content = GCODEMAIN + 10;
    content[strcspn(content, "\n")] = '\0';
    
    // Парсим аргументы: getsymbol <variable> <x> <y>
    char* getsymbol_var = NULL;
    int getsymbol_x = 0, getsymbol_y = 0;
    int getsymbol_arg_count = 0;
    
    char* getsymbol_current = content;
    char* getsymbol_next_space = NULL;
    
    // Парсим имя переменной
    getsymbol_next_space = strchr(getsymbol_current, ' ');
    if (getsymbol_next_space != NULL) {
        *getsymbol_next_space = '\0';
        getsymbol_var = strdup(getsymbol_current);
        getsymbol_current = getsymbol_next_space + 1;
        getsymbol_arg_count++;
    }
    
    // Парсим координату X
    if (getsymbol_arg_count == 1) {
        getsymbol_next_space = strchr(getsymbol_current, ' ');
        if (getsymbol_next_space != NULL) {
            *getsymbol_next_space = '\0';
            
            // Проверяем, является ли X переменной
            if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                Variable* x_var = find_variable(getsymbol_current + 5);
                if (x_var != NULL) {
                    getsymbol_x = atoi(x_var->value);
                }
            } else {
                getsymbol_x = atoi(getsymbol_current);
            }
            
            getsymbol_current = getsymbol_next_space + 1;
            getsymbol_arg_count++;
            
            // Парсим координату Y
            if (*getsymbol_current != '\0') {
                // Проверяем, является ли Y переменной
                if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                    Variable* y_var = find_variable(getsymbol_current + 5);
                    if (y_var != NULL) {
                        getsymbol_y = atoi(y_var->value);
                    }
                } else {
                    getsymbol_y = atoi(getsymbol_current);
                }
                getsymbol_arg_count++;
            }
        } else {
            // Нет пробела после X
            if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                Variable* x_var = find_variable(getsymbol_current + 5);
                if (x_var != NULL) {
                    getsymbol_x = atoi(x_var->value);
                }
            } else {
                getsymbol_x = atoi(getsymbol_current);
            }
            getsymbol_arg_count++;
        }
    }
    
    if (getsymbol_arg_count < 3) {
        printf("Invalid syntax: getsymbol requires 3 arguments\n");
        if (getsymbol_var) free(getsymbol_var);
        return;
    }
    
    // Для Windows используем Console API
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = {getsymbol_x - 1, getsymbol_y - 1}; // Координаты начинаются с 0
        DWORD chars_read = 0;
        CHAR_INFO char_info;
        
        // Читаем один символ
        if (ReadConsoleOutputCharacterA(hConsole, &char_info.Char.AsciiChar, 1, coord, &chars_read)) {
            if (chars_read == 1) {
                char result[2] = {char_info.Char.AsciiChar, '\0'};
                set_variable(getsymbol_var, result);
            } else {
                set_variable(getsymbol_var, " ");
            }
        } else {
            set_variable(getsymbol_var, " ");
        }
    
    free(getsymbol_var);
} else if (strncmp(GCODEMAIN, "while ", 6) == 0) {
    // Объявляем extern для доступа к глобальной переменной
    extern int break_flag;
    
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    char* while_condition = strdup(content);
    
    // Бесконечный цикл while
    while (1) {
        if (strlen(while_condition) > 0) {
            ask_gcode(while_condition);
        }
        
        // Проверяем флаг прерывания
        if (break_flag) {
            break_flag = 0; // Сбрасываем флаг
            break;
        }
        
        Sleep(10);
    }
    
    free(while_condition);
}
else if (strcmp(GCODEMAIN, "break") == 0) {
    // Устанавливаем флаг прерывания цикла
    extern int break_flag;
    break_flag = 1;
} else if (strncmp(GCODEMAIN, "start ", 6) == 0) {
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    // Получаем команду (может быть переменной или прямым текстом)
    char* command = NULL;
    
    if (strncmp(content, "<VAR>", 5) == 0) {
        // Получаем команду из переменной
        Variable* cmd_var = find_variable(content + 5);
        if (cmd_var != NULL) {
            command = strdup(cmd_var->value);
        } else {
            printf("Variable not found: %s\n", content + 5);
            return;
        }
    } else {
        // Прямая команда
        command = strdup(content);
    }
    
    if (command == NULL) {
        printf("Invalid syntax: invalid command\n");
        return;
    }
    
    // Выполняем системную команду
    // Для Windows
    system(command);
    
    free(command);
} else if (strncmp(GCODEMAIN, "hex ", 4) == 0) {
    char* content = GCODEMAIN + 4;
    content[strcspn(content, "\n")] = '\0';
    
    // Парсим аргументы: hex <mode> <input> <result_var>
    char* hex_mode = NULL;
    char* hex_input = NULL;
    char* hex_result_var = NULL;
    int hex_arg_count = 0;
    
    char* hex_current = content;
    char* hex_next_space = NULL;
    
    // Парсим режим (1 - text to hex, 2 - hex to text)
    hex_next_space = strchr(hex_current, ' ');
    if (hex_next_space != NULL) {
        *hex_next_space = '\0';
        hex_mode = strdup(hex_current);
        hex_current = hex_next_space + 1;
        hex_arg_count++;
    }
    
    // Парсим входные данные
    if (hex_arg_count == 1) {
        hex_next_space = strchr(hex_current, ' ');
        if (hex_next_space != NULL) {
            *hex_next_space = '\0';
            
            // Проверяем, является ли вход переменной
            if (strncmp(hex_current, "<VAR>", 5) == 0) {
                Variable* input_var = find_variable(hex_current + 5);
                if (input_var != NULL) {
                    hex_input = strdup(input_var->value);
                }
            } else {
                hex_input = strdup(hex_current);
            }
            
            hex_current = hex_next_space + 1;
            hex_arg_count++;
            
            // Парсим переменную результата
            if (*hex_current != '\0') {
                hex_result_var = strdup(hex_current);
                hex_arg_count++;
            }
        } else {
            // Нет пробела после input
            if (strncmp(hex_current, "<VAR>", 5) == 0) {
                Variable* input_var = find_variable(hex_current + 5);
                if (input_var != NULL) {
                    hex_input = strdup(input_var->value);
                }
            } else {
                hex_input = strdup(hex_current);
            }
            hex_arg_count++;
        }
    }
    
    if (hex_arg_count < 3) {
        printf("Invalid syntax: hex requires 3 arguments\n");
        if (hex_mode) free(hex_mode);
        if (hex_input) free(hex_input);
        if (hex_result_var) free(hex_result_var);
        return;
    }
    
    // Обработка конвертации
    if (strcmp(hex_mode, "1") == 0) {
        // Text to hex: конвертируем текст в hex
        size_t input_len = strlen(hex_input);
        char* hex_result = malloc(input_len * 2 + 1);
        
        for (size_t i = 0; i < input_len; i++) {
            sprintf(hex_result + i * 2, "%02x", (unsigned char)hex_input[i]);
        }
        hex_result[input_len * 2] = '\0';
        
        set_variable(hex_result_var, hex_result);
        free(hex_result);
    }
    else if (strcmp(hex_mode, "2") == 0) {
        // Hex to text: конвертируем hex в текст
        size_t input_len = strlen(hex_input);
        if (input_len % 2 != 0) {
            printf("Invalid syntax: hex input length must be even\n");
            set_variable(hex_result_var, "None");
        } else {
            size_t result_len = input_len / 2;
            char* text_result = malloc(result_len + 1);
            
            for (size_t i = 0; i < result_len; i++) {
                char hex_byte[3] = {hex_input[i * 2], hex_input[i * 2 + 1], '\0'};
                text_result[i] = (char)strtol(hex_byte, NULL, 16);
            }
            text_result[result_len] = '\0';
            
            set_variable(hex_result_var, text_result);
            free(text_result);
        }
    }
    else {
        printf("Invalid syntax: unknown hex mode %s (use 1 or 2)\n", hex_mode);
        set_variable(hex_result_var, "None");
    }
    
    // Очищаем память
    if (hex_mode) free(hex_mode);
    if (hex_input) free(hex_input);
    if (hex_result_var) free(hex_result_var);
} else if (strncmp(GCODEMAIN, "delete ", 7) == 0) {
    char* content = GCODEMAIN + 7;
    content[strcspn(content, "\n")] = '\0';
    
    // Парсим аргументы: delete <input> <count> <mode> <output_var>
    char* delete_input = NULL;
    char* delete_count_str = NULL;
    char* delete_mode_str = NULL;
    char* delete_output_var = NULL;
    int delete_arg_count = 0;
    
    char* delete_current = content;
    char* delete_next_space = NULL;
    
    // Парсим входную строку
    delete_next_space = strchr(delete_current, ' ');
    if (delete_next_space != NULL) {
        *delete_next_space = '\0';
        
        // Проверяем, является ли input переменной
        if (strncmp(delete_current, "<VAR>", 5) == 0) {
            Variable* input_var = find_variable(delete_current + 5);
            if (input_var != NULL) {
                delete_input = strdup(input_var->value);
            }
        } else {
            delete_input = strdup(delete_current);
        }
        
        delete_current = delete_next_space + 1;
        delete_arg_count++;
    }
    
    // Парсим count
    if (delete_arg_count == 1) {
        delete_next_space = strchr(delete_current, ' ');
        if (delete_next_space != NULL) {
            *delete_next_space = '\0';
            delete_count_str = strdup(delete_current);
            delete_current = delete_next_space + 1;
            delete_arg_count++;
        }
    }
    
    // Парсим mode
    if (delete_arg_count == 2) {
        delete_next_space = strchr(delete_current, ' ');
        if (delete_next_space != NULL) {
            *delete_next_space = '\0';
            delete_mode_str = strdup(delete_current);
            delete_current = delete_next_space + 1;
            delete_arg_count++;
            
            // Парсим output variable
            if (*delete_current != '\0') {
                delete_output_var = strdup(delete_current);
                delete_arg_count++;
            }
        }
    }
    
    if (delete_arg_count < 4) {
        printf("Invalid syntax: delete requires 4 arguments\n");
        if (delete_input) free(delete_input);
        if (delete_count_str) free(delete_count_str);
        if (delete_mode_str) free(delete_mode_str);
        if (delete_output_var) free(delete_output_var);
        return;
    }
    
    // Конвертируем count и mode в числа
    int delete_count = atoi(delete_count_str);
    int delete_mode = atoi(delete_mode_str);
    
    // Выполняем операцию delete
    char* result = NULL;
    int input_len = strlen(delete_input);
    
    if (delete_mode == 1) {
        // Режим 1: удалить с начала
        if (delete_count >= input_len) {
            result = strdup("");
        } else {
            result = strdup(delete_input + delete_count);
        }
    }
    else if (delete_mode == 2) {
        // Режим 2: удалить с конца  
        if (delete_count >= input_len) {
            result = strdup("");
        } else {
            result = malloc(input_len - delete_count + 1);
            strncpy(result, delete_input, input_len - delete_count);
            result[input_len - delete_count] = '\0';
        }
    }
    else {
        printf("Invalid syntax: invalid mode %d (use 1 or 2)\n", delete_mode);
        result = strdup(delete_input); // Возвращаем оригинал при ошибке
    }
    
    // Сохраняем результат
    set_variable(delete_output_var, result);
    
    // Очищаем память
    free(result);
    if (delete_input) free(delete_input);
    if (delete_count_str) free(delete_count_str);
    if (delete_mode_str) free(delete_mode_str);
    if (delete_output_var) free(delete_output_var);
} else if (strncmp(GCODEMAIN, "open ", 5) == 0) {
    char* content = GCODEMAIN + 5;
    content[strcspn(content, "\n")] = '\0';
    
    // Парсим аргументы: open <filename> <mode> <param>
    char* open_filename = NULL;
    char* open_mode = NULL;
    char* open_param = NULL;
    int open_arg_count = 0;
    
    char* open_current = content;
    char* open_next_space = NULL;
    
    // Парсим имя файла
    open_next_space = strchr(open_current, ' ');
    if (open_next_space != NULL) {
        *open_next_space = '\0';
        
        // Проверяем, является ли имя файла переменной
        if (strncmp(open_current, "<VAR>", 5) == 0) {
            Variable* file_var = find_variable(open_current + 5);
            if (file_var != NULL) {
                open_filename = strdup(file_var->value);
            }
        } else {
            open_filename = strdup(open_current);
        }
        
        open_current = open_next_space + 1;
        open_arg_count++;
    }
    
    // Парсим режим
    if (open_arg_count == 1) {
        open_next_space = strchr(open_current, ' ');
        if (open_next_space != NULL) {
            *open_next_space = '\0';
            open_mode = strdup(open_current);
            open_current = open_next_space + 1;
            open_arg_count++;
            
            // Парсим параметр
            if (*open_current != '\0') {
                // Проверяем, является ли параметр переменной
                if (strncmp(open_current, "<VAR>", 5) == 0) {
                    Variable* param_var = find_variable(open_current + 5);
                    if (param_var != NULL) {
                        open_param = strdup(param_var->value);
                    }
                } else {
                    open_param = strdup(open_current);
                }
                open_arg_count++;
            }
        } else {
            open_mode = strdup(open_current);
            open_arg_count++;
        }
    }
    
    if (open_arg_count < 2) {
        printf("Invalid syntax: open requires at least 2 arguments\n");
        if (open_filename) free(open_filename);
        if (open_mode) free(open_mode);
        return;
    }
    
    // Обработка различных режимов
    FILE* file = NULL;
    
    if (strcmp(open_mode, "read") == 0 || strcmp(open_mode, "readn") == 0 || 
        strcmp(open_mode, "readb") == 0 || strcmp(open_mode, "readbn") == 0) {
        // Режимы чтения
        if (strcmp(open_mode, "read") == 0) {
            file = fopen(open_filename, "r");
        } else if (strcmp(open_mode, "readn") == 0) {
            file = fopen(open_filename, "r");
        } else if (strcmp(open_mode, "readb") == 0) {
            file = fopen(open_filename, "rb");
        } else if (strcmp(open_mode, "readbn") == 0) {
            file = fopen(open_filename, "rb");
        }
        
        if (file) {
            if (strcmp(open_mode, "read") == 0) {
                char buffer[4096] = {0};
                size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
                buffer[bytes_read] = '\0';
                fclose(file);
                set_variable(open_param, buffer);
            }
            else if (strcmp(open_mode, "readn") == 0) {
                char buffer[4096] = {0};
                size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
                buffer[bytes_read] = '\0';
                fclose(file);
                
                // Заменяем реальные \n на текстовые \n
                char* pos = buffer;
                while ((pos = strstr(pos, "\n")) != NULL) {
                    memmove(pos + 2, pos + 1, strlen(pos));
                    memcpy(pos, "\\n", 2);
                    pos += 2;
                }
                set_variable(open_param, buffer);
            }
            else if (strcmp(open_mode, "readb") == 0) {
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                char* buffer = malloc(file_size + 1);
                fread(buffer, 1, file_size, file);
                buffer[file_size] = '\0';
                fclose(file);
                
                // Конвертируем бинарные данные в hex строку
                char hex_buffer[file_size * 2 + 1];
                for (int i = 0; i < file_size; i++) {
                    sprintf(hex_buffer + i * 2, "%02x", (unsigned char)buffer[i]);
                }
                set_variable(open_param, hex_buffer);
                free(buffer);
            }
            else if (strcmp(open_mode, "readbn") == 0) {
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                char* buffer = malloc(file_size + 1);
                fread(buffer, 1, file_size, file);
                buffer[file_size] = '\0';
                fclose(file);
                
                // Конвертируем в hex с \n для читаемости
                char hex_buffer[file_size * 3 + 1];
                for (int i = 0; i < file_size; i++) {
                    if (i > 0 && i % 16 == 0) {
                        sprintf(hex_buffer + i * 3 - 1, "\\n%02x", (unsigned char)buffer[i]);
                    } else {
                        sprintf(hex_buffer + i * 3, "%02x ", (unsigned char)buffer[i]);
                    }
                }
                set_variable(open_param, hex_buffer);
                free(buffer);
            }
        } else {
            // Файл не найден - записываем "None"
            set_variable(open_param, "None");
        }
    }
    else if (strcmp(open_mode, "write") == 0 || strcmp(open_mode, "writen") == 0 || 
             strcmp(open_mode, "writeb") == 0 || strcmp(open_mode, "writebn") == 0) {
        // Режимы записи
        if (strcmp(open_mode, "write") == 0) {
            file = fopen(open_filename, "w");
        } else if (strcmp(open_mode, "writen") == 0) {
            file = fopen(open_filename, "w");
        } else if (strcmp(open_mode, "writeb") == 0) {
            file = fopen(open_filename, "wb");
        } else if (strcmp(open_mode, "writebn") == 0) {
            file = fopen(open_filename, "wb");
        }
        
        if (file && open_param) {
            if (strcmp(open_mode, "write") == 0) {
                fwrite(open_param, 1, strlen(open_param), file);
                fclose(file);
            }
            else if (strcmp(open_mode, "writen") == 0) {
                // Заменяем текстовые \n на реальные \n
                char* temp_param = strdup(open_param);
                char* pos = temp_param;
                while ((pos = strstr(pos, "\\n")) != NULL) {
                    memmove(pos + 1, pos + 2, strlen(pos + 1));
                    *pos = '\n';
                    pos += 1;
                }
                fwrite(temp_param, 1, strlen(temp_param), file);
                fclose(file);
                free(temp_param);
            }
            else if (strcmp(open_mode, "writeb") == 0) {
                // Конвертируем hex строку обратно в бинарные данные
                size_t len = strlen(open_param);
                if (len % 2 == 0) {
                    char* bin_buffer = malloc(len / 2);
                    for (size_t i = 0; i < len; i += 2) {
                        char hex_byte[3] = {open_param[i], open_param[i + 1], '\0'};
                        bin_buffer[i / 2] = (char)strtol(hex_byte, NULL, 16);
                    }
                    fwrite(bin_buffer, 1, len / 2, file);
                    free(bin_buffer);
                }
                fclose(file);
            }
            else if (strcmp(open_mode, "writebn") == 0) {
                // Удаляем \n из hex строки и конвертируем
                char* clean_hex = strdup(open_param);
                char* pos = clean_hex;
                while ((pos = strstr(pos, "\\n")) != NULL) {
                    memmove(pos, pos + 2, strlen(pos + 1));
                }
                
                size_t len = strlen(clean_hex);
                if (len % 2 == 0) {
                    char* bin_buffer = malloc(len / 2);
                    for (size_t i = 0; i < len; i += 2) {
                        char hex_byte[3] = {clean_hex[i], clean_hex[i + 1], '\0'};
                        bin_buffer[i / 2] = (char)strtol(hex_byte, NULL, 16);
                    }
                    fwrite(bin_buffer, 1, len / 2, file);
                    free(bin_buffer);
                }
                free(clean_hex);
                fclose(file);
            }
        } else {
            printf("Invalid syntax: cannot open file %s for writing\n", open_filename);
        }
    }
    else {
        printf("Invalid syntax: unknown mode %s\n", open_mode);
    }
    
    // Очищаем память
    if (open_filename) free(open_filename);
    if (open_mode) free(open_mode);
    if (open_param) free(open_param);
} else if (strncmp(GCODEMAIN, "setrange ", 9) == 0) {
    char* content = GCODEMAIN + 9;
    content[strcspn(content, "\n")] = '\0';
    
    // Ручной парсинг аргументов: setrange <var_name> <from> <to> [<string>]
    char* setrange_var_name = NULL;
    int setrange_from = 0, setrange_to = 0;
    char* setrange_repeat_string = NULL;
    int setrange_arg_count = 0;
    
    char* setrange_current = content;
    char* setrange_next_space = NULL;
    
    // Парсим первый аргумент (имя переменной)
    setrange_next_space = strchr(setrange_current, ' ');
    if (setrange_next_space != NULL) {
        *setrange_next_space = '\0';
        setrange_var_name = setrange_current;
        setrange_current = setrange_next_space + 1;
        setrange_arg_count++;
    }
    
    // Парсим второй аргумент (from)
    if (setrange_arg_count == 1) {
        setrange_next_space = strchr(setrange_current, ' ');
        if (setrange_next_space != NULL) {
            *setrange_next_space = '\0';
            
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_from_var = find_variable(setrange_current + 5);
                if (setrange_from_var != NULL) {
                    setrange_from = atoi(setrange_from_var->value);
                }
            } else {
                setrange_from = atoi(setrange_current);
            }
            
            setrange_current = setrange_next_space + 1;
            setrange_arg_count++;
        }
    }
    
    // Парсим третий аргумент (to)
    if (setrange_arg_count == 2) {
        setrange_next_space = strchr(setrange_current, ' ');
        if (setrange_next_space != NULL) {
            *setrange_next_space = '\0';
            
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_to_var = find_variable(setrange_current + 5);
                if (setrange_to_var != NULL) {
                    setrange_to = atoi(setrange_to_var->value);
                }
            } else {
                setrange_to = atoi(setrange_current);
            }
            
            setrange_current = setrange_next_space + 1;
            setrange_arg_count++;
            
            // Четвертый аргумент (строка для повторения)
            if (*setrange_current != '\0') {
                if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                    Variable* setrange_str_var = find_variable(setrange_current + 5);
                    if (setrange_str_var != NULL) {
                        setrange_repeat_string = strdup(setrange_str_var->value);
                    }
                } else {
                    setrange_repeat_string = strdup(setrange_current);
                }
                setrange_arg_count++;
            }
        } else {
            // Нет пробела после to - обрабатываем to
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_to_var = find_variable(setrange_current + 5);
                if (setrange_to_var != NULL) {
                    setrange_to = atoi(setrange_to_var->value);
                }
            } else {
                setrange_to = atoi(setrange_current);
            }
            setrange_arg_count++;
        }
    }
    
    if (setrange_arg_count < 3) {
        printf("Invalid syntax: setrange requires at least 3 arguments\n");
        if (setrange_repeat_string) free(setrange_repeat_string);
        return;
    }
    
    char setrange_result[1024] = {0};
    
    if (setrange_arg_count == 3) {
        // Режим числовой последовательности
        for (int setrange_i = setrange_from; setrange_i <= setrange_to; setrange_i++) {
            char setrange_num_str[16];
            sprintf(setrange_num_str, "%d", setrange_i);
            strcat(setrange_result, setrange_num_str);
        }
    } else {
        // Режим повторения строки
        int setrange_repeat_count = setrange_to - setrange_from + 1;
        for (int setrange_i = 0; setrange_i < setrange_repeat_count; setrange_i++) {
            strcat(setrange_result, setrange_repeat_string);
        }
        free(setrange_repeat_string);
    }
    
    // Записываем результат в переменную
    set_variable(setrange_var_name, setrange_result);
} else if (strncmp(GCODEMAIN, "setmore ", 8) == 0) {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH * 4];
    char line_buffer[MAX_VALUE_LENGTH];
    
    // Считываем имя переменной
    if (sscanf(GCODEMAIN + 8, "%19s", name) == 1) {
        // Пропускаем имя и пробел после него
        const char* text_start = GCODEMAIN + 8;
        while (*text_start && *text_start != ' ') {
            text_start++;
        }
        while (*text_start && *text_start == ' ') {
            text_start++;
        }
        
        // Начинаем сборку значения
        value[0] = '\0';
        
        // Проверяем, есть ли <END> в первой строке
        if (strstr(text_start, "<END>") != NULL) {
            // <END> в той же строке - копируем только текст до <END>
            strncpy(value, text_start, MAX_VALUE_LENGTH * 4 - 1);
            value[MAX_VALUE_LENGTH * 4 - 1] = '\0';
            
            char* end_pos = strstr(value, "<END>");
            if (end_pos != NULL) {
                *end_pos = '\0';
            }
        } else {
            // <END> в следующих строках - начинаем с текущего текста
            strncpy(value, text_start, MAX_VALUE_LENGTH * 4 - 1);
            value[MAX_VALUE_LENGTH * 4 - 1] = '\0';
            
            // Проверяем режим выполнения через переменную __opened-as__
            int is_console = 0;
            Variable* opened_as = find_variable("__opened-as__");
            if (opened_as != NULL && strcmp(opened_as->value, "console") == 0) {
                is_console = 1;
            }
            
            if (is_console) {
                // Консольный режим - запрашиваем ввод у пользователя
                // printf("Enter multi-line text (end with <END> on separate line):\n");
                //if (value[0] != '\0') {
                //    strcat(value, "\n");
                //}
                
                while (fgets(line_buffer, sizeof(line_buffer), stdin) != NULL) {
                    // Удаляем символ новой строки
                    line_buffer[strcspn(line_buffer, "\r\n")] = '\0';
                    
                    // Проверяем, не является ли эта строка <END>
                    if (strcmp(line_buffer, "<END>") == 0) {
                        break;
                    }
                    
                    // Добавляем новую строку к значению
                    if (strlen(value) + strlen(line_buffer) + 2 < sizeof(value)) {
                        if (value[0] != '\0' && value[strlen(value)-1] != '\n') {
                            strcat(value, "\n");
                        }
                        strcat(value, line_buffer);
                    } else {
                        printf("Invalid syntax: Variable value too long\n");
                        break;
                    }
                }
            } else {
                // Файловый режим - читаем через strtok
                char* next_line;
                if (value[0] != '\0') {
                    strcat(value, "\n");
                }
                
                while ((next_line = strtok(NULL, "\n")) != NULL) {
                    // Проверяем, не является ли эта строка <END>
                    if (strcmp(next_line, "<END>") == 0) {
                        break;
                    }
                    
                    // Добавляем новую строку к значению
                    if (strlen(value) + strlen(next_line) + 2 < sizeof(value)) {
                        strcat(value, next_line);
                        strcat(value, "\n");
                    } else {
                        printf("Invalid syntax: Variable value too long\n");
                        break;
                    }
                }
                
                // Удаляем последний лишний перенос строки
                int len = strlen(value);
                if (len > 0 && value[len - 1] == '\n') {
                    value[len - 1] = '\0';
                }
            }
        }
        
        // Обрабатываем подстановки переменных в формате {varname}
        char result[MAX_VALUE_LENGTH * 4];
        result[0] = '\0';
        const char* current_pos = value;
        
        while (*current_pos) {
            if (*current_pos == '{') {
                const char* end_brace = strchr(current_pos + 1, '}');
                if (end_brace != NULL) {
                    // Извлекаем имя переменной между {}
                    int var_name_len = end_brace - (current_pos + 1);
                    char var_name[MAX_NAME_LENGTH];
                    
                    if (var_name_len < sizeof(var_name)) {
                        strncpy(var_name, current_pos + 1, var_name_len);
                        var_name[var_name_len] = '\0';
                        
                        // Ищем переменную
                        Variable* var = find_variable(var_name);
                        if (var != NULL) {
                            strcat(result, var->value);
                        } else {
                            // Если переменная не найдена, оставляем как есть
                            strncat(result, current_pos, end_brace - current_pos + 1);
                        }
                        
                        current_pos = end_brace + 1;
                        continue;
                    }
                }
            }
            
            // Копируем обычный символ
            int remaining_space = sizeof(result) - strlen(result) - 1;
            if (remaining_space > 0) {
                strncat(result, current_pos, 1);
            }
            current_pos++;
        }
        
        // Устанавливаем переменную с обработанным значением
        set_variable(name, result);
        
    } else {
        printf("Invalid syntax: setmore <name> <text>\n");
    }
}	else if (strncmp(GCODEMAIN, "set ", 4) == 0) {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
    if (sscanf(GCODEMAIN + 4, "%19s %9999[^\n]", name, value) == 2) {
        if (strncmp(value, "<FUNC>", 6) == 0) {
            char* expression = value + 6;
            int result = 0;
            
            if (strncmp(expression, "<VAR>", 5) == 0) {
                const char* var_name = expression + 5;
                Variable* var = find_variable(var_name);
                
                if (var != NULL) {
                    result = evaluate_simple_expression(var->value);
                    char result_str[MAX_VALUE_LENGTH];
                    snprintf(result_str, sizeof(result_str), "%d", result);
                    set_variable(name, result_str);
                } else {
                    printf("Variable not found: %s\n", var_name);
                }
            } else {
                result = evaluate_simple_expression(expression);
                char result_str[MAX_VALUE_LENGTH];
                snprintf(result_str, sizeof(result_str), "%d", result);
                set_variable(name, result_str);
            }
        } 
        else if (strncmp(value, "<VAR>", 5) == 0) {
            const char* var_name = value + 5;
            Variable* var = find_variable(var_name);
            
            if (var != NULL) {
                set_variable(name, var->value);
            } else {
                printf("Variable not found: %s\n", var_name);
            }
        } 
        else {
            set_variable(name, value);
        }
    } else {
        printf("Invalid syntax: set <name> <value>\n");
    }
} else if (strncmp(GCODEMAIN, "setadd ", 7) == 0) {
			char name[MAX_NAME_LENGTH];
			char arg1[MAX_VALUE_LENGTH];
			char arg2[MAX_VALUE_LENGTH];
			
			if (sscanf(GCODEMAIN + 7, "%19s %9999s %9999s", name, arg1, arg2) == 3) {
				const char* value1;
				const char* value2;
				char temp1[MAX_VALUE_LENGTH];
				char temp2[MAX_VALUE_LENGTH];
				
				// Обработка первого аргумента
				if (strncmp(arg1, "<VAR>", 5) == 0) {
					Variable* var1 = find_variable(arg1 + 5);
					if (var1 != NULL) {
						value1 = var1->value;
					} else {
						printf("Variable not found: %s\n", arg1 + 5);
						value1 = "";
					}
				} else {
					value1 = arg1;
				}
				
				// Обработка второго аргумента
				if (strncmp(arg2, "<VAR>", 5) == 0) {
					Variable* var2 = find_variable(arg2 + 5);
					if (var2 != NULL) {
						value2 = var2->value;
					} else {
						printf("Variable not found: %s\n", arg2 + 5);
						value2 = "";
					}
				} else {
					value2 = arg2;
				}
				
				// Конкатенация строк
				char result_str[MAX_VALUE_LENGTH * 2];
				snprintf(result_str, sizeof(result_str), "%s%s", value1, value2);
				set_variable(name, result_str);
				
			} else {
				printf("Invalid syntax: setadd <name> <value1_or_VAR> <value2_or_VAR>\n");
			}
		} else if (strncmp(GCODEMAIN, "setmath ", 8) == 0) {
			char name[MAX_NAME_LENGTH];
			char input[MAX_VALUE_LENGTH];
			
			if (sscanf(GCODEMAIN + 8, "%19s %9999[^\n]", name, input) == 2) {
				int numeric_value = 0;
				char* expression_to_evaluate = NULL;
				
				// Если начинается с <VAR>, берем значение переменной
				if (strncmp(input, "<VAR>", 5) == 0) {
					const char* var_name = input + 5;
					Variable* var = find_variable(var_name);
					
					if (var != NULL) {
						expression_to_evaluate = var->value;
					} else {
						printf("Variable not found: %s\n", var_name);
						expression_to_evaluate = "0";
					}
				} else {
					// Если не начинается с <VAR>, используем напрямую
					expression_to_evaluate = input;
				}
				
				// Простая функция для вычисления выражений
				numeric_value = evaluate_simple_expression(expression_to_evaluate);
				
				// Создаем новую переменную с результатом
				char result_str[10000];
				snprintf(result_str, sizeof(result_str), "%d", numeric_value);
				set_variable(name, result_str);
				
			} else {
				printf("Invalid syntax: setmath <name> <expression_or_VAR>\n");
			}
} else if (strncmp(GCODEMAIN, "include ", 8) == 0) {
    char filename[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 8, "%99[^\n]", filename) == 1) {
        char* actual_filename = filename;
        
        if (strncmp(filename, "<VAR>", 5) == 0) {
            Variable* file_var = find_variable(filename + 5);
            if (file_var != NULL) {
                actual_filename = file_var->value;
            } else {
                printf("Variable not found: %s\n", filename + 5);
                return;
            }
        }
      
        
        FILE* lib_file = fopen(actual_filename, "r");
        if (lib_file == NULL) {
            printf("Library not found: %s\n", actual_filename);
            return;
        }
        
        char line[MAX_VALUE_LENGTH];
        while (fgets(line, sizeof(line), lib_file)) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) > 0) {
                ask_gcode(line);
            }
        }
        
        fclose(lib_file);
    }
} else if (strncmp(GCODEMAIN, "execute ", 8) == 0) {
    char args[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 8, "%99[^\n]", args) == 1) {
        // Проверяем на UNL режим
        int unl_mode = 0;
        char* command = args;
        
        if (strncmp(args, "UNL ", 4) == 0) {
            unl_mode = 1;
            command = args + 4;
        }
        
        // Проверяем, начинается ли с <VAR>
        if (strncmp(command, "<VAR>", 5) == 0) {
            // Получаем значение переменной
            Variable* var = find_variable(command + 5);
            if (var != NULL) {
                char* code_to_execute = var->value;
                char processed_command[MAX_VALUE_LENGTH];
                
                if (unl_mode) {
                    // UNL РЕЖИМ: разделение по текстовому "\n"
                    int j = 0;
                    for (int i = 0; code_to_execute[i] != '\0' && j < MAX_VALUE_LENGTH - 1; i++) {
                        if (code_to_execute[i] == '\\' && code_to_execute[i+1] == 'n') {
                            processed_command[j++] = '\n';
                            i++; // Пропускаем следующий символ
                        } else {
                            processed_command[j++] = code_to_execute[i];
                        }
                    }
                    processed_command[j] = '\0';
                    code_to_execute = processed_command;
                } else {
                    // ОБЫЧНЫЙ РЕЖИМ: копируем как есть
                    strncpy(processed_command, code_to_execute, MAX_VALUE_LENGTH - 1);
                    processed_command[MAX_VALUE_LENGTH - 1] = '\0';
                    code_to_execute = processed_command;
                }
                
                // Выполняем построчно
                char *line_start = code_to_execute;
                char *line_end;
                
                while (*line_start != '\0') {
                    // Находим конец строки
                    line_end = line_start;
                    while (*line_end != '\0' && *line_end != '\n') {
                        line_end++;
                    }
                    
                    // Временная копия строки
                    int line_length = line_end - line_start;
                    if (line_length > 0) {
                        char line[line_length + 1];
                        strncpy(line, line_start, line_length);
                        line[line_length] = '\0';
                        
                        // Выполняем команду
                        if (strlen(line) > 0) {
                            ask_gcode(line);
                        }
                    }
                    
                    // Переходим к следующей строке
                    if (*line_end == '\n') {
                        line_start = line_end + 1;
                    } else {
                        line_start = line_end;
                    }
                }
            } else {
                printf("Variable not found: %s\n", command + 5);
            }
        } else {
            // Прямое выполнение команды (без <VAR>)
            if (unl_mode) {
                // UNL РЕЖИМ для прямого текста: разделение по "\n"
                char processed_command[MAX_VALUE_LENGTH];
                int j = 0;
                for (int i = 0; command[i] != '\0' && j < MAX_VALUE_LENGTH - 1; i++) {
                    if (command[i] == '\\' && command[i+1] == 'n') {
                        processed_command[j++] = '\n';
                        i++; // Пропускаем следующий символ
                    } else {
                        processed_command[j++] = command[i];
                    }
                }
                processed_command[j] = '\0';
                
                // Выполняем построчно
                char *line_start = processed_command;
                char *line_end;
                
                while (*line_start != '\0') {
                    line_end = line_start;
                    while (*line_end != '\0' && *line_end != '\n') {
                        line_end++;
                    }
                    
                    int line_length = line_end - line_start;
                    if (line_length > 0) {
                        char line[line_length + 1];
                        strncpy(line, line_start, line_length);
                        line[line_length] = '\0';
                        
                        if (strlen(line) > 0) {
                            ask_gcode(line);
                        }
                    }
                    
                    if (*line_end == '\n') {
                        line_start = line_end + 1;
                    } else {
                        line_start = line_end;
                    }
                }
            } else {
                // ОБЫЧНЫЙ РЕЖИМ: выполняем как одну команду
                ask_gcode(command);
            }
        }
    } else {
        printf("Invalid syntax: execute [UNL] <command_or_VAR>\n");
    }
} else if (strncmp(GCODEMAIN, "getinput", 8) == 0) {
    char* args = GCODEMAIN + 8;
    while (*args == ' ') args++; // Пропускаем пробелы

    if (*args == '\0') {
        // Просто getinput - ждем ввод
        char input[MAX_VALUE_LENGTH];
        fgets(input, sizeof(input), stdin);
    } else {
        // Разбираем аргументы
        char first_arg[MAX_VALUE_LENGTH] = {0};
        char second_arg[MAX_NAME_LENGTH] = {0};
        
        // Парсим первый аргумент
        sscanf(args, "%s %s", first_arg, second_arg);
        
        // Определяем приглашение и переменную для сохранения
        char prompt_text[MAX_VALUE_LENGTH] = {0};
        char save_var_name[MAX_NAME_LENGTH] = {0};
        int should_save = 0;
        
        if (second_arg[0] != '\0') {
            // Есть два аргумента: приглашение и переменная
            should_save = 1;
            strcpy(save_var_name, second_arg);
            
            if (strncmp(first_arg, "<VAR>", 5) == 0) {
                // Приглашение из переменной
                Variable* var = find_variable(first_arg + 5);
                if (var != NULL) {
                    strcpy(prompt_text, var->value);
                }
            } else {
                // Прямое приглашение
                strcpy(prompt_text, first_arg);
            }
        } else {
            // Только один аргумент - только приглашение
            if (strncmp(first_arg, "<VAR>", 5) == 0) {
                Variable* var = find_variable(first_arg + 5);
                if (var != NULL) {
                    strcpy(prompt_text, var->value);
                }
            } else {
                strcpy(prompt_text, first_arg);
            }
        }
        
        // Выводим приглашение
        if (prompt_text[0] != '\0') {
            printf("%s", prompt_text);
            fflush(stdout);
        }
        
        // Читаем ввод
        char input[MAX_VALUE_LENGTH];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            
            // Сохраняем если нужно
            if (should_save && save_var_name[0] != '\0') {
                set_variable(save_var_name, input);
                //printf("Saved to variable: %s\n", save_var_name);
            }
        }
    }
} else if (strncmp(GCODEMAIN, "if ", 3) == 0) {
    char buffer[MAX_VALUE_LENGTH];
    strcpy(buffer, GCODEMAIN + 3);
    
    // Ищем операторы
    char* op_pos = NULL;
    char operator[4] = "";
    
    if ((op_pos = strstr(buffer, " ==")) != NULL) {
        strcpy(operator, "==");
    } else if ((op_pos = strstr(buffer, " /=")) != NULL) {
        strcpy(operator, "/=");
    } else if ((op_pos = strstr(buffer, " =.")) != NULL) {
        strcpy(operator, "=.");
    } else if ((op_pos = strstr(buffer, " /.")) != NULL) {
        strcpy(operator, "/.");
    } else if ((op_pos = strstr(buffer, " .=")) != NULL) {
        strcpy(operator, ".=");
    } else if ((op_pos = strstr(buffer, " ./")) != NULL) {
        strcpy(operator, "./");
    } else if ((op_pos = strstr(buffer, " ..")) != NULL) {
        strcpy(operator, "..");
    } else if ((op_pos = strstr(buffer, " //")) != NULL) {
        strcpy(operator, "//");
    }
    
    if (op_pos != NULL) {
        *op_pos = '\0'; // Разделяем на левую часть и остальное
        char* left_side = buffer;
        char* rest = op_pos + strlen(operator) + 1; // Пропускаем оператор с пробелом
        
        // Пропускаем пробелы в rest
        while (*rest == ' ') rest++;
        
        // Ищем конец правой части (первый пробел после значения)
        char* right_end = strchr(rest, ' ');
        if (right_end != NULL) {
            *right_end = '\0'; // Разделяем правую часть и команду
            char* right_side = rest;
            char* command = right_end + 1;
            
            // Получаем левое значение
            char left_value[MAX_VALUE_LENGTH];
            if (strncmp(left_side, "<VAR>", 5) == 0) {
                Variable* left_var = find_variable(left_side + 5);
                if (left_var != NULL) strcpy(left_value, left_var->value);
                else strcpy(left_value, "");
            } else {
                strcpy(left_value, left_side);
            }
            
            // Получаем правое значение
            char right_value[MAX_VALUE_LENGTH];
            if (strncmp(right_side, "<VAR>", 5) == 0) {
                Variable* right_var = find_variable(right_side + 5);
                if (right_var != NULL) strcpy(right_value, right_var->value);
                else strcpy(right_value, "");
            } else {
                strcpy(right_value, right_side);
            }
            
            // Проверяем условие
            int condition_met = 0;
            
            if (strcmp(operator, "==") == 0) {
                condition_met = (strcmp(left_value, right_value) == 0);
            } else if (strcmp(operator, "/=") == 0) {
                condition_met = (strcmp(left_value, right_value) != 0);
            } else if (strcmp(operator, "=.") == 0) {
                condition_met = (strncmp(left_value, right_value, strlen(right_value)) == 0);
            } else if (strcmp(operator, "/.") == 0) {
                condition_met = (strncmp(left_value, right_value, strlen(right_value)) != 0);
            } else if (strcmp(operator, ".=") == 0) {
                int left_len = strlen(left_value);
                int right_len = strlen(right_value);
                if (left_len >= right_len) {
                    condition_met = (strcmp(left_value + left_len - right_len, right_value) == 0);
                } else {
                    condition_met = 0;
                }
            } else if (strcmp(operator, "./") == 0) {
                int left_len = strlen(left_value);
                int right_len = strlen(right_value);
                if (left_len >= right_len) {
                    condition_met = (strcmp(left_value + left_len - right_len, right_value) != 0);
                } else {
                    condition_met = 1;
                }
            } else if (strcmp(operator, "..") == 0) {
                condition_met = (strstr(left_value, right_value) != NULL);
            } else if (strcmp(operator, "//") == 0) {
                condition_met = (strstr(left_value, right_value) == NULL);
            }
            
            if (condition_met) ask_gcode(command);
            
        } else {
            printf("Invalid if syntax: missing command\n");
        }
    } else {
        printf("Invalid if syntax: missing operator\n");
    }
} else if (strncmp(GCODEMAIN, "for ", 4) == 0) {
    char var_name[MAX_NAME_LENGTH];
    char in_text[MAX_VALUE_LENGTH];
    char iterable[MAX_VALUE_LENGTH];
    char command[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 4, "%19s in %99s %99[^\n]", var_name, in_text, command) == 3) {
        // Получаем итерируемое значение
        char iterable_value[MAX_VALUE_LENGTH];
        if (strncmp(in_text, "<VAR>", 5) == 0) {
            Variable* iter_var = find_variable(in_text + 5);
            if (iter_var != NULL) {
                strcpy(iterable_value, iter_var->value);
            } else {
                strcpy(iterable_value, "");
            }
        } else {
            strcpy(iterable_value, in_text);
        }
        
        // Итерируем по каждому символу
        for (int i = 0; iterable_value[i] != '\0'; i++) {
            // Создаем временную переменную с текущим символом
            char current_char[2] = {iterable_value[i], '\0'};
            set_variable(var_name, current_char);
            
            // Выполняем команду
            ask_gcode(command);
        }
        
    } else {
        printf("Invalid for syntax: for <var> in <value> <command>\n");
    }
} else if (strncmp(GCODEMAIN, "setget ", 7) == 0) {
    char set_var[MAX_NAME_LENGTH];
    char get_var[MAX_NAME_LENGTH];
    
    if (sscanf(GCODEMAIN + 7, "%99s %99s", set_var, get_var) == 2) {
        Variable* source_var = find_variable(get_var);
        if (source_var != NULL) {
            set_variable(set_var, source_var->value);
        } else {
            printf("Variable not found: %s\n", get_var);
        }
    } else {
        printf("Invalid syntax: setget <varname_set> <varname_get>\n");
    }
} else {
    // Создаем копию для безопасной работы и обрезаем \n
    char command_copy[MAX_VALUE_LENGTH];
    strncpy(command_copy, GCODEMAIN, MAX_VALUE_LENGTH - 1);
    command_copy[MAX_VALUE_LENGTH - 1] = '\0';
    
    // Обрезаем символ новой строки в конце
    command_copy[strcspn(command_copy, "\r\n")] = '\0';
    
    // Ищем первое слово (имя команды)
    char* cmd_name = command_copy;
    char* first_space = strchr(command_copy, ' ');
    
    // Вычисляем аргументы ДО обрезки command_copy
    char* args = NULL;
    if (first_space != NULL) {
        args = first_space + 1;  // Аргументы начинаются после пробела
        *first_space = '\0'; // Обрезаем после первого слова
    } else {
        args = "";  // Нет аргументов
    }
    
    // Проверяем, существует ли переменная с таким именем
    Variable* cmd_var = find_variable(cmd_name);
    
    if (cmd_var != NULL) {
        // Устанавливаем аргументы функции
        if (strlen(args) > 0) {
            // Создаем копию для безопасного парсинга
            char args_copy[MAX_VALUE_LENGTH];
            strncpy(args_copy, args, MAX_VALUE_LENGTH - 1);
            args_copy[MAX_VALUE_LENGTH - 1] = '\0';
            
            // Парсим аргументы вручную (без strtok)
            char* argv_func[10] = {0};
            int argc_func = 0;
            
            char* current = args_copy;
            int in_arg = 0;
            
            while (*current != '\0' && argc_func < 10) {
                if (*current != ' ') {
                    if (!in_arg) {
                        // Начало нового аргумента
                        argv_func[argc_func] = current;
                        argc_func++;
                        in_arg = 1;
                    }
                } else {
                    if (in_arg) {
                        // Конец аргумента
                        *current = '\0';
                        in_arg = 0;
                    }
                }
                current++;
            }
            
            // Устанавливаем переменные аргументов
            char argc_str[10];
            snprintf(argc_str, sizeof(argc_str), "%d", argc_func);
            set_variable("__argc-func__", argc_str);
            
            for (int i = 0; i < argc_func; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i + 1);
                set_variable(var_name, argv_func[i]);
            }
            
            // Очищаем оставшиеся слоты
            for (int i = argc_func; i < 10; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i + 1);
                set_variable(var_name, "");
            }
        } else {
            // Нет аргументов
            set_variable("__argc-func__", "0");
            for (int i = 1; i <= 10; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i);
                set_variable(var_name, "");
            }
        }
        
        // Выполняем код функции построчно
        char* command = cmd_var->value;
        char *line_start = command;
        char *line_end;
        
        while (*line_start != '\0') {
            line_end = line_start;
            while (*line_end != '\0' && *line_end != '\n') {
                line_end++;
            }
            
            int line_length = line_end - line_start;
            if (line_length > 0) {
                char line[line_length + 1];
                strncpy(line, line_start, line_length);
                line[line_length] = '\0';
                
                if (strlen(line) > 0) {
                    ask_gcode(line);
                }
            }
            
            if (*line_end == '\n') {
                line_start = line_end + 1;
            } else {
                line_start = line_end;
            }
        }
        
    } else {
        // Переменной нет - ошибка
        printf("Invalid syntax: %s\n", cmd_name);
        return;
    }
}
}

int main(int argc, char *argv[]) {  
		set_variable("__nothing__", "");
		set_variable("__None__", "\0");
		set_variable("__newline__", "\n");
		set_variable("__resetline__", "\r");
		// Открываем
		char exe_path[MAX_PATH];
		
		// Получаем полный путь к исполняемому файлу
		GetModuleFileName(NULL, exe_path, MAX_PATH);
		set_variable("__path__", exe_path);
		char* last_slash = strrchr(exe_path, '\\');
		if (last_slash != NULL) {
			set_variable("__name__", last_slash + 1);
		} else {
			set_variable("__name__", exe_path);
		}
		FILE *file = fopen(exe_path, "rb");
		if (file == NULL) {
			perror("Error opening file");
			return 1;
		}
		// Определяем размер файла
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char size_str[50];
		snprintf(size_str, sizeof(size_str), "%d", file_size);
		set_variable("__size__", size_str);

		// Выделяем память для данных
		unsigned char *buffer = (unsigned char*)malloc(file_size);
		if (buffer == NULL) {
			perror("Memory error");
			fclose(file);
			return 1;
		}

		// Читаем файл
		size_t bytes_read = fread(buffer, 1, file_size, file);
		if (bytes_read != file_size) {
			perror("Error reading file");
			free(buffer);
			fclose(file);
			return 1;
		}
		
		// Поиск мета данных
		long position = find_PATTERNGCODE_from_end(buffer, file_size);
		
		char* code = buffer+12+position;
		
		char GCODEMAIN[10000];
		if (code == NULL || code[0] == '\0' || strcmp(code, "None") == 0) {
		if (argc > 1 && strncmp(argv[1], "gcode://", 8) == 0) {
			set_variable("__opened-as__","url");
			char *download_url = argv[1] + 8;
			// Декодируем URI (особенно для символов / и :)
			char decoded_url[1024];
			int j = 0;
			for (int i = 0; download_url[i] != '\0' && j < sizeof(decoded_url) - 1; i++) {
				if (download_url[i] == '%') {
					// Декодируем %2F -> / и %3A -> :
					if (download_url[i+1] == '2' && download_url[i+2] == 'F') {
						decoded_url[j++] = '/';
						i += 2;
					} else if (download_url[i+1] == '3' && download_url[i+2] == 'A') {
						decoded_url[j++] = ':';
						i += 2;
					} else {
						// Оставляем как есть, если не распознано
						decoded_url[j++] = download_url[i];
					}
				} else {
					decoded_url[j++] = download_url[i];
				}
			}
			decoded_url[j] = '\0';
			if (strncmp(decoded_url, "file///", 7) == 0) {
				memmove(decoded_url + 6, decoded_url + 5, strlen(decoded_url + 5) + 1);
				memcpy(decoded_url, "file://", 7);
			} else if (strncmp(decoded_url, "http//", 6) == 0) {
				memmove(decoded_url + 6, decoded_url + 5, strlen(decoded_url + 5) + 1);
				memcpy(decoded_url, "http://", 7);
			} else if (strncmp(decoded_url, "https//", 7) == 0) {
				memmove(decoded_url + 7, decoded_url + 6, strlen(decoded_url + 6) + 1);
				memcpy(decoded_url, "https://", 8);
			}
			set_variable("__url__",decoded_url);
			// Создаем команду для curl
			char command[1024];
			snprintf(command, sizeof(command), "curl -s \"%s\"", decoded_url);
			
			// Выполняем curl и читаем вывод
			FILE *pipe = popen(command, "r");
			if (pipe) {
				char buffer[10000];
				while (fgets(buffer, sizeof(buffer), pipe)) {
					// Убираем символы новой строки
					buffer[strcspn(buffer, "\r\n")] = '\0';
					if (strlen(buffer) > 0) {
						ask_gcode(buffer);
					}
				}
				pclose(pipe);
			}
		} else { if (argc > 1 && !strncmp(argv[1], "gcode://", 8) == 0) {
			set_variable("__opened-as__","argv");
			initialize_args_variables(argc, argv);
			// Открываем файл для чтения
			FILE *file = fopen(argv[1], "r");
			if (file == NULL) {
				perror("Error opening file");
				return 1;
			}
			// Переменная для хранения кода
			char *code_arg = NULL;
			// Начальный размер буфера
			size_t buffer_size = 0;
			// Текущая позиция в буфере
			size_t code_length = 0;
			int c;

			// Читаем файл посимвольно
			while ((c = fgetc(file)) != EOF) {
				// Увеличиваем буфер при необходимости
				if (code_length >= buffer_size) {
					buffer_size = buffer_size == 0 ? 128 : buffer_size * 2;
					code_arg = realloc(code_arg, buffer_size);
					if (code_arg == NULL) {
						fclose(file);
						fprintf(stderr, "Memory allocation failed\n");
						return 1;
					}
				}
				// Добавляем символ в буфер
				code_arg[code_length++] = (char)c;
			}

			// Добавляем нулевой терминатор
			if (code_arg != NULL) {
				code_arg = realloc(code_arg, code_length + 1);
				code_arg[code_length] = '\0';
			}

			fclose(file);

			// Теперь code_arg содержит содержимое файла
			char *linearg = strtok(code_arg, "\n");
			while (linearg != NULL) {
				// Выполняем команду
				ask_gcode(linearg);
				
				// Получаем следующую строку
				linearg = strtok(NULL, "\n");
			}

			// Не забываем освободить память
			free(code_arg);
        return 1;
		} else {
			//Вывести приветсвие
			printf("GCode By GPGStudio.\n\n"); 
			set_variable("__opened-as__","console");
			while (1) {
				printf("GCode> ");
				//Запросить ввод
				fgets(GCODEMAIN, sizeof(GCODEMAIN), stdin);
				//Спросить язык програмирования
				ask_gcode(GCODEMAIN);
			}
		}
		}} else {
		
		set_variable("__opened-as__","compiled");
		initialize_args_variables(argc, argv);
		char *line = strtok(code, "\n"); // Разделяем код на строки
		
		while (line != NULL) {
			// Выполняем команду
			line[strcspn(line, "\r")] = '\0';
			ask_gcode(line);
			
			// Получаем следующую строку
			line = strtok(NULL, "\n");
		}
		}
		// Закрываем файл
		free(buffer);
		fclose(file);
	return 0;  
	}
