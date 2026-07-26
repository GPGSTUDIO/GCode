#include <stdio.h>  
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#define MAX_VARIABLES 100
#define MAX_NAME_LENGTH 100
#define MAX_VALUE_LENGTH 10000

// Глобальный флаг для break
int break_flag = 0;
// Глобальный указатель для чтения строк
char* g_code_reader = NULL;

const unsigned char PATTERNGCODE[] = "###CODE###";
const size_t PATTERNGCODE_LENGTH = sizeof(PATTERNGCODE) - 1;

// Функция для получения следующей строки
char* get_next_line() {
    if (g_code_reader == NULL || *g_code_reader == '\0') {
        return NULL;
    }
    
    // Ищем конец строки
    char* line_end = g_code_reader;
    while (*line_end != '\0' && *line_end != '\n') {
        line_end++;
    }
    
    int line_length = line_end - g_code_reader;
    
    if (line_length == 0 && *line_end == '\0') {
        return NULL;
    }
    
    // Создаем копию строки
    char* line = malloc(line_length + 1);
    strncpy(line, g_code_reader, line_length);
    line[line_length] = '\0';
    line[strcspn(line, "\r")] = '\0';
    
    // Перемещаем указатель
    if (*line_end == '\n') {
        g_code_reader = line_end + 1;
    } else {
        g_code_reader = line_end;
    }
    
    return line;
}

int evaluate_simple_expression(const char* expr) {
    int result = 0;
    int current = 0;
    char op = '+';
    int i = 0;
    
    while (expr[i] != '\0') {
        if (expr[i] >= '0' && expr[i] <= '9') {
            current = current * 10 + (expr[i] - '0');
        } else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
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
    
    switch (op) {
        case '+': result += current; break;
        case '-': result -= current; break;
        case '*': result *= current; break;
        case '/': if (current != 0) result /= current; break;
    }
    
    return result;
}

long find_PATTERNGCODE_from_end(const unsigned char *buffer, long buffer_size) {
    for (long i = buffer_size - PATTERNGCODE_LENGTH; i >= 0; i--) {
        if (memcmp(&buffer[i], PATTERNGCODE, PATTERNGCODE_LENGTH) == 0) {
            return i;
        }
    }
    return -1;
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
        snprintf(var_name, sizeof(var_name), "__argv-%d__", i);
        set_variable(var_name, argv[i]);
    }
    
    set_variable("__argc__", "0");
    char argc_str[10];
    snprintf(argc_str, sizeof(argc_str), "%d", argc);
    set_variable("__argc__", argc_str);
}

void handle_find(char* GCODEMAIN) {
    char* content = GCODEMAIN + 5;
    content[strcspn(content, "\n")] = '\0';
    
    char* find_output_var = NULL;
    char* find_search_in = NULL;
    char* find_search_for = NULL;
    int find_arg_count = 0;
    
    char* find_current = content;
    char* find_next_space = NULL;
    
    find_next_space = strchr(find_current, ' ');
    if (find_next_space != NULL) {
        *find_next_space = '\0';
        find_output_var = strdup(find_current);
        find_current = find_next_space + 1;
        find_arg_count++;
    }
    
    if (find_arg_count == 1) {
        find_next_space = find_current;
        int in_var = 0;
        
        if (strncmp(find_current, "<VAR>", 5) == 0) {
            find_next_space = strchr(find_current + 5, ' ');
            if (find_next_space == NULL) {
                find_next_space = find_current + strlen(find_current);
            }
        }
        
        if (find_next_space != NULL && *find_next_space != '\0') {
            char old_char = *find_next_space;
            *find_next_space = '\0';
            
            if (strncmp(find_current, "<VAR>", 5) == 0) {
                Variable* var = find_variable(find_current + 5);
                if (var != NULL) {
                    find_search_in = strdup(var->value);
                } else {
                    find_search_in = strdup("");
                }
            } else {
                find_search_in = strdup(find_current);
            }
            
            *find_next_space = old_char;
            find_current = find_next_space + 1;
            find_arg_count++;
            
            if (*find_current != '\0') {
                if (strncmp(find_current, "<VAR>", 5) == 0) {
                    Variable* var = find_variable(find_current + 5);
                    if (var != NULL) {
                        find_search_for = strdup(var->value);
                    } else {
                        find_search_for = strdup("");
                    }
                } else {
                    find_search_for = strdup(find_current);
                }
                find_arg_count++;
            }
        }
    }
    
    if (find_arg_count < 3) {
        printf("Invalid syntax: find requires 3 arguments\n");
        if (find_output_var) free(find_output_var);
        if (find_search_in) free(find_search_in);
        if (find_search_for) free(find_search_for);
        return;
    }
    
    char* found_pos = strstr(find_search_in, find_search_for);
    char result[20];
    
    if (found_pos != NULL) {
        int position = found_pos - find_search_in;
        snprintf(result, sizeof(result), "%d", position);
    } else {
        snprintf(result, sizeof(result), "-1");
    }
    
    set_variable(find_output_var, result);
    
    if (find_output_var) free(find_output_var);
    if (find_search_in) free(find_search_in);
    if (find_search_for) free(find_search_for);
}

void handle_split(char* GCODEMAIN) {
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    char* split_output_var = NULL;
    char* split_string = NULL;
    char* split_delimiter = NULL;
    int split_index = 0;
    int split_max_splits = -1;
    
    char* args[5] = {NULL};
    int arg_count = 0;
    
    char* current = content;
    int in_var = 0;
    char* arg_start = current;
    
    while (*current != '\0' && arg_count < 5) {
        if (strncmp(current, "<VAR>", 5) == 0) {
            if (arg_start == current || !in_var) {
                in_var = 1;
                if (arg_start == current) {
                    arg_start = current;
                }
            }
            char* var_end = strchr(current, '>');
            if (var_end != NULL) {
                current = var_end + 1;
                if (*current == ' ') {
                    *current = '\0';
                    args[arg_count++] = arg_start;
                    current++;
                    while (*current == ' ') current++;
                    arg_start = current;
                    in_var = 0;
                } else if (*current == '\0') {
                    args[arg_count++] = arg_start;
                    in_var = 0;
                }
                continue;
            }
        } else if (*current == ' ' && !in_var) {
            *current = '\0';
            args[arg_count++] = arg_start;
            current++;
            while (*current == ' ') current++;
            arg_start = current;
            continue;
        } else if (*current == ' ' && in_var) {
            char* check_pos = current + 1;
            while (*check_pos == ' ') check_pos++;
            if (strncmp(check_pos, "<VAR>", 5) == 0 || *check_pos != '\0') {
                *current = '\0';
                args[arg_count++] = arg_start;
                current = check_pos;
                arg_start = current;
                in_var = 0;
                continue;
            }
        }
        current++;
    }
    
    if (arg_start < current && arg_count < 5) {
        args[arg_count++] = arg_start;
    }
    
    if (arg_count < 4) {
        printf("Invalid syntax: split requires at least 4 arguments\n");
        return;
    }
    
    split_output_var = strdup(args[0]);
    
    if (strncmp(args[1], "<VAR>", 5) == 0) {
        Variable* var = find_variable(args[1] + 5);
        if (var != NULL) split_string = strdup(var->value);
        else split_string = strdup("");
    } else {
        split_string = strdup(args[1]);
    }
    
    if (strncmp(args[2], "<VAR>", 5) == 0) {
        Variable* var = find_variable(args[2] + 5);
        if (var != NULL) split_delimiter = strdup(var->value);
        else split_delimiter = strdup("");
    } else {
        split_delimiter = strdup(args[2]);
    }
    
    if (strncmp(args[3], "<VAR>", 5) == 0) {
        Variable* var = find_variable(args[3] + 5);
        if (var != NULL) split_index = atoi(var->value);
    } else {
        split_index = atoi(args[3]);
    }
    
    if (arg_count >= 5 && args[4] != NULL) {
        if (strcmp(args[4], "*") == 0) {
            split_max_splits = -1;
        } else if (strncmp(args[4], "<VAR>", 5) == 0) {
            Variable* var = find_variable(args[4] + 5);
            if (var != NULL) split_max_splits = atoi(var->value);
        } else {
            split_max_splits = atoi(args[4]);
        }
    }
    
    char* split_result = NULL;
    char* temp_string = strdup(split_string);
    int delimiter_len = strlen(split_delimiter);
    
    char** tokens = NULL;
    int token_count = 0;
    int tokens_capacity = 10;
    tokens = malloc(tokens_capacity * sizeof(char*));
    
    char* search_pos = temp_string;
    int splits_done = 0;
    
    while (*search_pos != '\0') {
        char* next_delim = strstr(search_pos, split_delimiter);
        
        if (next_delim != NULL && (split_max_splits == -1 || splits_done < split_max_splits)) {
            *next_delim = '\0';
            
            if (token_count >= tokens_capacity) {
                tokens_capacity *= 2;
                tokens = realloc(tokens, tokens_capacity * sizeof(char*));
            }
            tokens[token_count] = strdup(search_pos);
            token_count++;
            
            search_pos = next_delim + delimiter_len;
            splits_done++;
        } else {
            if (token_count >= tokens_capacity) {
                tokens_capacity *= 2;
                tokens = realloc(tokens, tokens_capacity * sizeof(char*));
            }
            tokens[token_count] = strdup(search_pos);
            token_count++;
            break;
        }
    }
    
    if (split_index >= 0 && split_index < token_count) {
        split_result = strdup(tokens[split_index]);
    } else {
        split_result = strdup("");
    }
    
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);
    
    set_variable(split_output_var, split_result);
    
    if (split_output_var) free(split_output_var);
    if (split_string) free(split_string);
    if (split_delimiter) free(split_delimiter);
    if (split_result) free(split_result);
    if (temp_string) free(temp_string);
}

void handle_replace(char* GCODEMAIN) {
    char* content = GCODEMAIN + 8;
    content[strcspn(content, "\n")] = '\0';
    
    char* replace_output_var = NULL;
    char* replace_string = NULL;
    char* replace_old = NULL;
    char* replace_new = NULL;
    int replace_arg_count = 0;
    
    char* replace_current = content;
    char* replace_next_space = NULL;
    
    replace_next_space = strchr(replace_current, ' ');
    if (replace_next_space != NULL) {
        *replace_next_space = '\0';
        replace_output_var = strdup(replace_current);
        replace_current = replace_next_space + 1;
        replace_arg_count++;
    }
    
    if (replace_arg_count == 1) {
        replace_next_space = replace_current;
        
        if (strncmp(replace_current, "<VAR>", 5) == 0) {
            char* var_end = strchr(replace_current, '>');
            if (var_end != NULL) {
                replace_next_space = strchr(var_end + 1, ' ');
            }
        }
        
        if (replace_next_space != NULL && *replace_next_space != '\0') {
            char old_char = *replace_next_space;
            *replace_next_space = '\0';
            
            if (strncmp(replace_current, "<VAR>", 5) == 0) {
                Variable* var = find_variable(replace_current + 5);
                if (var != NULL) replace_string = strdup(var->value);
                else replace_string = strdup("");
            } else {
                replace_string = strdup(replace_current);
            }
            
            *replace_next_space = old_char;
            replace_current = replace_next_space + 1;
            replace_arg_count++;
        }
    }
    
    if (replace_arg_count == 2) {
        replace_next_space = replace_current;
        
        if (strncmp(replace_current, "<VAR>", 5) == 0) {
            char* var_end = strchr(replace_current, '>');
            if (var_end != NULL) {
                replace_next_space = strchr(var_end + 1, ' ');
            }
        }
        
        if (replace_next_space != NULL && *replace_next_space != '\0') {
            char old_char = *replace_next_space;
            *replace_next_space = '\0';
            
            if (strncmp(replace_current, "<VAR>", 5) == 0) {
                Variable* var = find_variable(replace_current + 5);
                if (var != NULL) replace_old = strdup(var->value);
                else replace_old = strdup("");
            } else {
                replace_old = strdup(replace_current);
            }
            
            *replace_next_space = old_char;
            replace_current = replace_next_space + 1;
            replace_arg_count++;
            
            if (*replace_current != '\0') {
                if (strncmp(replace_current, "<VAR>", 5) == 0) {
                    Variable* var = find_variable(replace_current + 5);
                    if (var != NULL) replace_new = strdup(var->value);
                    else replace_new = strdup("");
                } else {
                    replace_new = strdup(replace_current);
                }
                replace_arg_count++;
            }
        }
    }
    
    if (replace_arg_count < 4) {
        printf("Invalid syntax: replace requires 4 arguments\n");
        if (replace_output_var) free(replace_output_var);
        if (replace_string) free(replace_string);
        if (replace_old) free(replace_old);
        if (replace_new) free(replace_new);
        return;
    }
    
    char* replace_result = malloc(strlen(replace_string) + 1);
    strcpy(replace_result, replace_string);
    
    int old_len = strlen(replace_old);
    int new_len = strlen(replace_new);
    
    int count = 0;
    char* temp_pos = replace_result;
    while ((temp_pos = strstr(temp_pos, replace_old)) != NULL) {
        count++;
        temp_pos += old_len;
    }
    
    if (count > 0) {
        char* new_result = malloc(strlen(replace_result) + count * (new_len - old_len) + 1);
        char* write_pos = new_result;
        char* read_pos = replace_result;
        
        char* next_occurrence;
        while ((next_occurrence = strstr(read_pos, replace_old)) != NULL) {
            int prefix_len = next_occurrence - read_pos;
            memcpy(write_pos, read_pos, prefix_len);
            write_pos += prefix_len;
            
            memcpy(write_pos, replace_new, new_len);
            write_pos += new_len;
            
            read_pos = next_occurrence + old_len;
        }
        
        strcpy(write_pos, read_pos);
        
        free(replace_result);
        replace_result = new_result;
    }
    
    set_variable(replace_output_var, replace_result);
    
    if (replace_output_var) free(replace_output_var);
    if (replace_string) free(replace_string);
    if (replace_old) free(replace_old);
    if (replace_new) free(replace_new);
    if (replace_result) free(replace_result);
}

// Структура для DLL модулей
typedef struct {
    char* dll_name;
    HMODULE handle;
    char** function_names;
    int function_count;
} DLLModule;

// Глобальные переменные для DLL
DLLModule* dll_modules = NULL;
int dll_module_count = 0;

// Исправленная parse_function_list
char** parse_function_list(const char* func_list_str, int* count) {
    char** functions = NULL;
    int func_count = 0;
    int capacity = 10;
    
    functions = malloc(capacity * sizeof(char*));
    if (functions == NULL) {
        *count = 0;
        return NULL;
    }
    
    const unsigned char* current = (const unsigned char*)func_list_str;
    
    while (true) {
        // Пропускаем нулевые байты (разделители)
        while (*current == 0x00) {
            current++;
        }
        
        // Нашли начало имени функции
        const unsigned char* start = current;
        
        // Ищем конец имени функции (0x00 или 0xFF)
        while (*current != 0x00) {
            current++;
        }
        
        int len = current - start;
        if (len > 0) {
            if (func_count >= capacity) {
                capacity *= 2;
                functions = realloc(functions, capacity * sizeof(char*));
                if (functions == NULL) {
                    *count = 0;
                    return NULL;
                }
            }
            functions[func_count] = malloc(len + 1);
            if (functions[func_count] != NULL) {
                memcpy(functions[func_count], start, len);
                functions[func_count][len] = '\0';
                func_count++;
            }
        }
        
        // Пропускаем разделитель 0x00
        if (*current == 0x00) {
            current++;
        }
    }
    
    *count = func_count;
    return functions;
}

// add_dll_module
int add_dll_module(const char* dll_name) {
    // Проверяем, не загружена ли уже эта DLL
    for (int i = 0; i < dll_module_count; i++) {
        if (strcmp(dll_modules[i].dll_name, dll_name) == 0) {
            return 1;
        }
    }
    
    HMODULE dll_handle = LoadLibraryA(dll_name);
    
    if (dll_handle == NULL) {
        return 0;
    }
    
    // Пытаемся получить точку входа GCEntry
    typedef const char* (*GCEntryFunc)();
    GCEntryFunc gcentry_func = (GCEntryFunc)GetProcAddress(dll_handle, "GCEntry");
    
    if (gcentry_func == NULL) {
        printf("Warning: Not GCL\n", dll_name);
        FreeLibrary(dll_handle);
        return 0;
    }
    
    // Получаем список функций
    const char* func_list = gcentry_func();
    
    // Считаем количество функций и их общую длину
    int func_count = 0;
    const char* temp = func_list;
    int total_length = 0;
    
    // Сначала считаем количество функций и общую длину
    while (1) {
        while (*temp == ' ') temp++;
        if (*temp == '\0') {
            // Проверяем, не конец ли это всего списка
            // Если следующий байт тоже \0, значит конец
            // Но мы не можем это проверить без знания размера буфера
            // Поэтому просто считаем до первого \0
            break;
        }
        
        const char* start = temp;
        while (*temp != '\0' && *temp != ' ') temp++;
        
        if (temp > start) {
            func_count++;
            total_length += (temp - start);
        }
        
        if (*temp == '\0') {
            temp++; // Переходим к следующей функции после \0
        } else if (*temp == ' ') {
            temp++;
        }
    }
    
    // Теперь парсим функции
    char** functions = malloc(func_count * sizeof(char*));
    int current_func = 0;
    temp = func_list;
    
    while (current_func < func_count) {
        while (*temp == ' ') temp++;
        if (*temp == '\0') break;
        
        const char* start = temp;
        while (*temp != '\0' && *temp != ' ') temp++;
        
        int len = temp - start;
        if (len > 0) {
            functions[current_func] = malloc(len + 1);
            memcpy(functions[current_func], start, len);
            functions[current_func][len] = '\0';
            current_func++;
        }
        
        if (*temp == '\0') {
            temp++;
        } else if (*temp == ' ') {
            temp++;
        }
    }
    
    // Сохраняем информацию о модуле
    DLLModule* new_modules = realloc(dll_modules, (dll_module_count + 1) * sizeof(DLLModule));
    if (new_modules == NULL) {
        printf("Memory allocation failed\n");
        for (int i = 0; i < current_func; i++) free(functions[i]);
        free(functions);
        FreeLibrary(dll_handle);
        return 0;
    }
    dll_modules = new_modules;
    
    dll_modules[dll_module_count].dll_name = strdup(dll_name);
    dll_modules[dll_module_count].handle = dll_handle;
    dll_modules[dll_module_count].function_names = functions;
    dll_modules[dll_module_count].function_count = func_count;
    dll_module_count++;
   
    char dll_copy[MAX_PATH];
    strncpy(dll_copy, dll_name, MAX_PATH - 1);
    dll_copy[MAX_PATH - 1] = '\0';
    
    char* name = strrchr(dll_copy, '\\');
    if (name != NULL) name++; else name = dll_copy;
    char* name2 = strrchr(name, '/');
    if (name2 != NULL) name = name2 + 1;
    
    char* ext = strrchr(name, '.');
    if (ext != NULL) *ext = '\0';
    
    // Прямое копирование с \x00\xFF
    Variable* var = find_variable(name);
    if (var == NULL && variable_count < MAX_VARIABLES) {
        var = &variables[variable_count];
        strncpy(var->name, name, MAX_NAME_LENGTH - 1);
        var->name[MAX_NAME_LENGTH - 1] = '\0';
        variable_count++;
    }
    
    if (var != NULL) {
        var->value[0] = '\x00';
        var->value[1] = '\xFF';
        strcpy(var->value + 2, dll_name);
    }
    return 1;
}

// Проверка наличия функции в DLL
int dll_has_function(const char* dll_name, const char* func_name) {
    for (int i = 0; i < dll_module_count; i++) {
        if (strcmp(dll_modules[i].dll_name, dll_name) == 0) {
            for (int j = 0; j < dll_modules[i].function_count; j++) {
                if (strcmp(dll_modules[i].function_names[j], func_name) == 0) {
                    return 1;
                }
            }
            return 0;
        }
    }
    return 0;
}

void ask_gcode(char GCODEMAIN[10000]);

// Вызов функции из DLL с передачей переменных
char* call_dll_function_by_name(const char* dll_name, const char* func_name, const char* args) {
    fflush(stdout);
    
    for (int i = 0; i < dll_module_count; i++) {
        if (strcmp(dll_modules[i].dll_name, dll_name) == 0) {
            // Проверяем, есть ли такая функция в списке
            int function_exists = 0;
            for (int j = 0; j < dll_modules[i].function_count; j++) {
                if (strcmp(dll_modules[i].function_names[j], func_name) == 0) {
                    function_exists = 1;
                    break;
                }
            }
            
            if (!function_exists) {
                printf("Invalid syntax: %s\n", func_name);
                return NULL;
            }
            
            // Рассчитываем размер буфера (с завершающим нулём)
            size_t args_len = (args && *args) ? strlen(args) : 0;
            size_t buffer_size = 1; // для завершающего \0
            
            // Считаем размер для всех переменных
            for (int v = 0; v < variable_count; v++) {
                if (v > 0) {
                    buffer_size += 2; // \x00\x00 разделитель между переменными
                }
                buffer_size += strlen(variables[v].name);
                buffer_size += 2; // \x00\x01
                buffer_size += strlen(variables[v].value);
            }
            
            // Добавляем разделитель \x00\x02 и аргументы
            buffer_size += 2; // \x00\x02
            buffer_size += args_len;
            
            // Выделяем память
            char* full_args = malloc(buffer_size);
            if (full_args == NULL) {
                return NULL;
            }
            
            // Формируем строку с переменными, используя memcpy
            size_t offset = 0;
            
            for (int v = 0; v < variable_count; v++) {
                if (v > 0) {
                    // Разделитель между переменными
                    memcpy(full_args + offset, "\x00\x00", 2);
                    offset += 2;
                }
                
                // Копируем имя переменной
                size_t name_len = strlen(variables[v].name);
                memcpy(full_args + offset, variables[v].name, name_len);
                offset += name_len;
                
                // Разделитель имя/значение
                memcpy(full_args + offset, "\x00\x01", 2);
                offset += 2;
                
                // Копируем значение переменной
                size_t value_len = strlen(variables[v].value);
                memcpy(full_args + offset, variables[v].value, value_len);
                offset += value_len;
            }
            
            // Добавляем разделитель между переменными и аргументами
            memcpy(full_args + offset, "\x00\x02", 2);
            offset += 2;
            
            // Копируем аргументы
            if (args && *args) {
                memcpy(full_args + offset, args, args_len);
                offset += args_len;
            }
            
            // Завершающий нуль
            full_args[offset] = '\0';
            
            typedef char* (*DLLFunctionCharPtr)(const char*);
            typedef void (*DLLFunctionVoid)();
            typedef void (*DLLFunctionChar)(const char*);
            
            DLLFunctionCharPtr func_char_ptr = (DLLFunctionCharPtr)GetProcAddress(dll_modules[i].handle, func_name);
            if (func_char_ptr != NULL) {
                char* result = func_char_ptr(full_args);
                if (result != NULL) {
                    if (result[0] == '\x00') {
                        printf("%s", result + 1);
                    }
                    else if (result[0] == '\x01') {
                        ask_gcode(result + 1);
                    }
                    else if (result[0] == '\x02') {
                        char* var_name = result + 1;
                        // Ищем разделитель \x00 между именем и значением
                        char* null_pos = var_name;
                        while (*null_pos != '\0') null_pos++;
                        
                        // Переходим к значению
                        char* var_value = null_pos + 1;
                        
                        // Временно заменяем \x00 на \0 для set_variable
                        *null_pos = '\0';
                        set_variable(var_name, var_value);
                    }
                }
                
                free(full_args);
                return NULL;
            } else {
                DLLFunctionVoid func_void = (DLLFunctionVoid)GetProcAddress(dll_modules[i].handle, func_name);
                
                if (func_void != NULL) {
                    func_void();
                    free(full_args);
                    return NULL;
                } else {
                    DLLFunctionChar func_char = (DLLFunctionChar)GetProcAddress(dll_modules[i].handle, func_name);
                    
                    if (func_char != NULL) {
                        func_char(full_args);
                        free(full_args);
                        return NULL;
                    } else {
                        printf("Invalid syntax: %s\n", func_name);
                        free(full_args);
                        return NULL;
                    }
                }
            }
        }
    }
    return NULL;
}

// Очистка DLL модулей
void cleanup_dll_modules(void) {
    for (int i = 0; i < dll_module_count; i++) {
        if (dll_modules[i].handle != NULL) {
            FreeLibrary(dll_modules[i].handle);
        }
        if (dll_modules[i].dll_name != NULL) {
            free(dll_modules[i].dll_name);
        }
        
        if (dll_modules[i].function_names != NULL) {
            for (int j = 0; j < dll_modules[i].function_count; j++) {
                if (dll_modules[i].function_names[j] != NULL) {
                    free(dll_modules[i].function_names[j]);
                }
            }
            free(dll_modules[i].function_names);
        }
    }
    free(dll_modules);
    dll_modules = NULL;
    dll_module_count = 0;
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
} else if (strncmp(GCODEMAIN, "find ", 5) == 0) {
    handle_find(GCODEMAIN);
} else if (strncmp(GCODEMAIN, "split ", 6) == 0) {
    handle_split(GCODEMAIN);
} else if (strncmp(GCODEMAIN, "replace ", 8) == 0) {
    handle_replace(GCODEMAIN);
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
    
    char* getsymbol_var = NULL;
    int getsymbol_x = 0, getsymbol_y = 0;
    int getsymbol_arg_count = 0;
    
    char* getsymbol_current = content;
    char* getsymbol_next_space = NULL;
    
    getsymbol_next_space = strchr(getsymbol_current, ' ');
    if (getsymbol_next_space != NULL) {
        *getsymbol_next_space = '\0';
        getsymbol_var = strdup(getsymbol_current);
        getsymbol_current = getsymbol_next_space + 1;
        getsymbol_arg_count++;
    }
    
    if (getsymbol_arg_count == 1) {
        getsymbol_next_space = strchr(getsymbol_current, ' ');
        if (getsymbol_next_space != NULL) {
            *getsymbol_next_space = '\0';
            
            if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                Variable* x_var = find_variable(getsymbol_current + 5);
                if (x_var != NULL) getsymbol_x = atoi(x_var->value);
            } else {
                getsymbol_x = atoi(getsymbol_current);
            }
            
            getsymbol_current = getsymbol_next_space + 1;
            getsymbol_arg_count++;
            
            if (*getsymbol_current != '\0') {
                if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                    Variable* y_var = find_variable(getsymbol_current + 5);
                    if (y_var != NULL) getsymbol_y = atoi(y_var->value);
                } else {
                    getsymbol_y = atoi(getsymbol_current);
                }
                getsymbol_arg_count++;
            }
        } else {
            if (strncmp(getsymbol_current, "<VAR>", 5) == 0) {
                Variable* x_var = find_variable(getsymbol_current + 5);
                if (x_var != NULL) getsymbol_x = atoi(x_var->value);
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
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {getsymbol_x - 1, getsymbol_y - 1};
    DWORD chars_read = 0;
    CHAR_INFO char_info;
    
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
    extern int break_flag;
    
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    char* while_condition = strdup(content);
    
    while (1) {
        if (strlen(while_condition) > 0) {
            ask_gcode(while_condition);
        }
        
        if (break_flag) {
            break_flag = 0;
            break;
        }
        
        Sleep(10);
    }
    
    free(while_condition);
}
else if (strcmp(GCODEMAIN, "break") == 0) {
    extern int break_flag;
    break_flag = 1;
} else if (strncmp(GCODEMAIN, "start ", 6) == 0) {
    char* content = GCODEMAIN + 6;
    content[strcspn(content, "\n")] = '\0';
    
    char* command = NULL;
    
    if (strncmp(content, "<VAR>", 5) == 0) {
        Variable* cmd_var = find_variable(content + 5);
        if (cmd_var != NULL) {
            command = strdup(cmd_var->value);
        } else {
            printf("Variable not found: %s\n", content + 5);
            return;
        }
    } else {
        command = strdup(content);
    }
    
    if (command == NULL) {
        printf("Invalid syntax: invalid command\n");
        return;
    }
    
    system(command);
    
    free(command);
} else if (strncmp(GCODEMAIN, "hex ", 4) == 0) {
    char* content = GCODEMAIN + 4;
    content[strcspn(content, "\n")] = '\0';
    
    char* hex_mode = NULL;
    char* hex_input = NULL;
    char* hex_result_var = NULL;
    int hex_arg_count = 0;
    
    char* hex_current = content;
    char* hex_next_space = NULL;
    
    hex_next_space = strchr(hex_current, ' ');
    if (hex_next_space != NULL) {
        *hex_next_space = '\0';
        hex_mode = strdup(hex_current);
        hex_current = hex_next_space + 1;
        hex_arg_count++;
    }
    
    if (hex_arg_count == 1) {
        hex_next_space = strchr(hex_current, ' ');
        if (hex_next_space != NULL) {
            *hex_next_space = '\0';
            
            if (strncmp(hex_current, "<VAR>", 5) == 0) {
                Variable* input_var = find_variable(hex_current + 5);
                if (input_var != NULL) hex_input = strdup(input_var->value);
            } else {
                hex_input = strdup(hex_current);
            }
            
            hex_current = hex_next_space + 1;
            hex_arg_count++;
            
            if (*hex_current != '\0') {
                hex_result_var = strdup(hex_current);
                hex_arg_count++;
            }
        } else {
            if (strncmp(hex_current, "<VAR>", 5) == 0) {
                Variable* input_var = find_variable(hex_current + 5);
                if (input_var != NULL) hex_input = strdup(input_var->value);
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
    
    if (strcmp(hex_mode, "1") == 0) {
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
    
    if (hex_mode) free(hex_mode);
    if (hex_input) free(hex_input);
    if (hex_result_var) free(hex_result_var);
} else if (strncmp(GCODEMAIN, "delete ", 7) == 0) {
    char* content = GCODEMAIN + 7;
    content[strcspn(content, "\n")] = '\0';
    
    char* delete_input = NULL;
    char* delete_count_str = NULL;
    char* delete_mode_str = NULL;
    char* delete_output_var = NULL;
    int delete_arg_count = 0;
    
    char* delete_current = content;
    char* delete_next_space = NULL;
    
    delete_next_space = strchr(delete_current, ' ');
    if (delete_next_space != NULL) {
        *delete_next_space = '\0';
        
        if (strncmp(delete_current, "<VAR>", 5) == 0) {
            Variable* input_var = find_variable(delete_current + 5);
            if (input_var != NULL) delete_input = strdup(input_var->value);
        } else {
            delete_input = strdup(delete_current);
        }
        
        delete_current = delete_next_space + 1;
        delete_arg_count++;
    }
    
    if (delete_arg_count == 1) {
        delete_next_space = strchr(delete_current, ' ');
        if (delete_next_space != NULL) {
            *delete_next_space = '\0';
            delete_count_str = strdup(delete_current);
            delete_current = delete_next_space + 1;
            delete_arg_count++;
        }
    }
    
    if (delete_arg_count == 2) {
        delete_next_space = strchr(delete_current, ' ');
        if (delete_next_space != NULL) {
            *delete_next_space = '\0';
            delete_mode_str = strdup(delete_current);
            delete_current = delete_next_space + 1;
            delete_arg_count++;
            
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
    
    int delete_count = atoi(delete_count_str);
    int delete_mode = atoi(delete_mode_str);
    
    char* result = NULL;
    int input_len = strlen(delete_input);
    
    if (delete_mode == 1) {
        if (delete_count >= input_len) {
            result = strdup("");
        } else {
            result = strdup(delete_input + delete_count);
        }
    }
    else if (delete_mode == 2) {
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
        result = strdup(delete_input);
    }
    
    set_variable(delete_output_var, result);
    
    free(result);
    if (delete_input) free(delete_input);
    if (delete_count_str) free(delete_count_str);
    if (delete_mode_str) free(delete_mode_str);
    if (delete_output_var) free(delete_output_var);
} else if (strncmp(GCODEMAIN, "open ", 5) == 0) {
    char* content = GCODEMAIN + 5;
    content[strcspn(content, "\n")] = '\0';
    
    char* open_filename = NULL;
    char* open_mode = NULL;
    char* open_param = NULL;
    int open_arg_count = 0;
    
    char* open_current = content;
    char* open_next_space = NULL;
    
    open_next_space = strchr(open_current, ' ');
    if (open_next_space != NULL) {
        *open_next_space = '\0';
        
        if (strncmp(open_current, "<VAR>", 5) == 0) {
            Variable* file_var = find_variable(open_current + 5);
            if (file_var != NULL) open_filename = strdup(file_var->value);
        } else {
            open_filename = strdup(open_current);
        }
        
        open_current = open_next_space + 1;
        open_arg_count++;
    }
    
    if (open_arg_count == 1) {
        open_next_space = strchr(open_current, ' ');
        if (open_next_space != NULL) {
            *open_next_space = '\0';
            open_mode = strdup(open_current);
            open_current = open_next_space + 1;
            open_arg_count++;
            
            if (*open_current != '\0') {
                if (strncmp(open_current, "<VAR>", 5) == 0) {
                    Variable* param_var = find_variable(open_current + 5);
                    if (param_var != NULL) open_param = strdup(param_var->value);
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
    
    FILE* file = NULL;
    
    if (strcmp(open_mode, "read") == 0 || strcmp(open_mode, "readn") == 0 || 
        strcmp(open_mode, "readb") == 0 || strcmp(open_mode, "readbn") == 0) {
        if (strcmp(open_mode, "read") == 0) file = fopen(open_filename, "r");
        else if (strcmp(open_mode, "readn") == 0) file = fopen(open_filename, "r");
        else if (strcmp(open_mode, "readb") == 0) file = fopen(open_filename, "rb");
        else if (strcmp(open_mode, "readbn") == 0) file = fopen(open_filename, "rb");
        
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
                
                char* hex_buffer = malloc(file_size * 2 + 1);
                for (int i = 0; i < file_size; i++) {
                    sprintf(hex_buffer + i * 2, "%02x", (unsigned char)buffer[i]);
                }
                set_variable(open_param, hex_buffer);
                free(buffer);
                free(hex_buffer);
            }
            else if (strcmp(open_mode, "readbn") == 0) {
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                char* buffer = malloc(file_size + 1);
                fread(buffer, 1, file_size, file);
                buffer[file_size] = '\0';
                fclose(file);
                
                char* hex_buffer = malloc(file_size * 3 + 1);
                hex_buffer[0] = '\0';
                for (int i = 0; i < file_size; i++) {
                    if (i > 0 && i % 16 == 0) {
                        sprintf(hex_buffer + strlen(hex_buffer), "\\n%02x", (unsigned char)buffer[i]);
                    } else {
                        sprintf(hex_buffer + strlen(hex_buffer), "%02x ", (unsigned char)buffer[i]);
                    }
                }
                set_variable(open_param, hex_buffer);
                free(buffer);
                free(hex_buffer);
            }
        } else {
            set_variable(open_param, "None");
        }
    }
    else if (strcmp(open_mode, "write") == 0 || strcmp(open_mode, "writen") == 0 || 
             strcmp(open_mode, "writeb") == 0 || strcmp(open_mode, "writebn") == 0) {
        if (strcmp(open_mode, "write") == 0) file = fopen(open_filename, "w");
        else if (strcmp(open_mode, "writen") == 0) file = fopen(open_filename, "w");
        else if (strcmp(open_mode, "writeb") == 0) file = fopen(open_filename, "wb");
        else if (strcmp(open_mode, "writebn") == 0) file = fopen(open_filename, "wb");
        
        if (file && open_param) {
            if (strcmp(open_mode, "write") == 0) {
                fwrite(open_param, 1, strlen(open_param), file);
                fclose(file);
            }
            else if (strcmp(open_mode, "writen") == 0) {
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
    
    if (open_filename) free(open_filename);
    if (open_mode) free(open_mode);
    if (open_param) free(open_param);
} else if (strncmp(GCODEMAIN, "setrange ", 9) == 0) {
    char* content = GCODEMAIN + 9;
    content[strcspn(content, "\n")] = '\0';
    
    char* setrange_var_name = NULL;
    int setrange_from = 0, setrange_to = 0;
    char* setrange_repeat_string = NULL;
    int setrange_arg_count = 0;
    
    char* setrange_current = content;
    char* setrange_next_space = NULL;
    
    setrange_next_space = strchr(setrange_current, ' ');
    if (setrange_next_space != NULL) {
        *setrange_next_space = '\0';
        setrange_var_name = setrange_current;
        setrange_current = setrange_next_space + 1;
        setrange_arg_count++;
    }
    
    if (setrange_arg_count == 1) {
        setrange_next_space = strchr(setrange_current, ' ');
        if (setrange_next_space != NULL) {
            *setrange_next_space = '\0';
            
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_from_var = find_variable(setrange_current + 5);
                if (setrange_from_var != NULL) setrange_from = atoi(setrange_from_var->value);
            } else {
                setrange_from = atoi(setrange_current);
            }
            
            setrange_current = setrange_next_space + 1;
            setrange_arg_count++;
        }
    }
    
    if (setrange_arg_count == 2) {
        setrange_next_space = strchr(setrange_current, ' ');
        if (setrange_next_space != NULL) {
            *setrange_next_space = '\0';
            
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_to_var = find_variable(setrange_current + 5);
                if (setrange_to_var != NULL) setrange_to = atoi(setrange_to_var->value);
            } else {
                setrange_to = atoi(setrange_current);
            }
            
            setrange_current = setrange_next_space + 1;
            setrange_arg_count++;
            
            if (*setrange_current != '\0') {
                if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                    Variable* setrange_str_var = find_variable(setrange_current + 5);
                    if (setrange_str_var != NULL) setrange_repeat_string = strdup(setrange_str_var->value);
                } else {
                    setrange_repeat_string = strdup(setrange_current);
                }
                setrange_arg_count++;
            }
        } else {
            if (strncmp(setrange_current, "<VAR>", 5) == 0) {
                Variable* setrange_to_var = find_variable(setrange_current + 5);
                if (setrange_to_var != NULL) setrange_to = atoi(setrange_to_var->value);
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
        for (int setrange_i = setrange_from; setrange_i <= setrange_to; setrange_i++) {
            char setrange_num_str[16];
            sprintf(setrange_num_str, "%d", setrange_i);
            strcat(setrange_result, setrange_num_str);
        }
    } else {
        int setrange_repeat_count = setrange_to - setrange_from + 1;
        for (int setrange_i = 0; setrange_i < setrange_repeat_count; setrange_i++) {
            strcat(setrange_result, setrange_repeat_string);
        }
        free(setrange_repeat_string);
    }
    
    set_variable(setrange_var_name, setrange_result);
} else if (strncmp(GCODEMAIN, "setmore ", 8) == 0) {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH * 4];
    int nested_setmore_count = 1;
    
    if (sscanf(GCODEMAIN + 8, "%99s", name) == 1) {
        const char* text_start = GCODEMAIN + 8;
        while (*text_start && *text_start != ' ') text_start++;
        while (*text_start && *text_start == ' ') text_start++;
        
        value[0] = '\0';
        
        // Проверяем, есть ли <END> в этой же строке
        char* end_pos_check = strstr(text_start, "<END>");
        char* setmore_pos_check = strstr(text_start, "setmore ");
        
        if (end_pos_check != NULL && (setmore_pos_check == NULL || setmore_pos_check > end_pos_check)) {
            // Всё в одной строке
            strncpy(value, text_start, end_pos_check - text_start);
            value[end_pos_check - text_start] = '\0';
        } else {
            // Многострочный режим
            if (strlen(text_start) > 0) {
                strcpy(value, text_start);
                strcat(value, "\n");
            }
            
            // Читаем следующие строки через get_next_line()
            char* next_line;
            while (nested_setmore_count > 0 && (next_line = get_next_line()) != NULL) {
                if (strncmp(next_line, "setmore ", 8) == 0) {
                    nested_setmore_count++;
                } else if (strcmp(next_line, "<END>") == 0) {
                    nested_setmore_count--;
                    free(next_line);
                    if (nested_setmore_count == 0) break;
                    if (strlen(value) + 6 < sizeof(value)) {
                        strcat(value, "<END>\n");
                    }
                    continue;
                }
                
                if (strlen(value) + strlen(next_line) + 2 < sizeof(value)) {
                    strcat(value, next_line);
                    strcat(value, "\n");
                }
                free(next_line);
            }
            
            // Удаляем последний \n
            int len = strlen(value);
            if (len > 0 && value[len - 1] == '\n') {
                value[len - 1] = '\0';
            }
        }
        
        // Обрабатываем подстановки переменных
        char result[MAX_VALUE_LENGTH * 4];
        result[0] = '\0';
        const char* current_pos = value;
        
        while (*current_pos) {
            if (*current_pos == '{') {
                const char* end_brace = strchr(current_pos + 1, '}');
                if (end_brace != NULL) {
                    int var_name_len = end_brace - (current_pos + 1);
                    char var_name[MAX_NAME_LENGTH];
                    
                    if (var_name_len < sizeof(var_name)) {
                        strncpy(var_name, current_pos + 1, var_name_len);
                        var_name[var_name_len] = '\0';
                        
                        Variable* var = find_variable(var_name);
                        if (var != NULL) {
                            strcat(result, var->value);
                        } else {
                            strncat(result, current_pos, end_brace - current_pos + 1);
                        }
                        
                        current_pos = end_brace + 1;
                        continue;
                    }
                }
            }
            
            int remaining_space = sizeof(result) - strlen(result) - 1;
            if (remaining_space > 0) {
                strncat(result, current_pos, 1);
            }
            current_pos++;
        }
        
        set_variable(name, result);
    } else {
        printf("Invalid syntax: setmore <name> <text>\n");
    }
} else if (strncmp(GCODEMAIN, "set ", 4) == 0) {
    char name[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
    if (sscanf(GCODEMAIN + 4, "%99s %9999[^\n]", name, value) == 2) {
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
    
    if (sscanf(GCODEMAIN + 7, "%99s %9999s %9999s", name, arg1, arg2) == 3) {
        const char* value1;
        const char* value2;
        
        if (strncmp(arg1, "<VAR>", 5) == 0) {
            Variable* var1 = find_variable(arg1 + 5);
            if (var1 != NULL) value1 = var1->value;
            else value1 = "";
        } else {
            value1 = arg1;
        }
        
        if (strncmp(arg2, "<VAR>", 5) == 0) {
            Variable* var2 = find_variable(arg2 + 5);
            if (var2 != NULL) value2 = var2->value;
            else value2 = "";
        } else {
            value2 = arg2;
        }
        
        char result_str[MAX_VALUE_LENGTH * 2];
        snprintf(result_str, sizeof(result_str), "%s%s", value1, value2);
        set_variable(name, result_str);
    } else {
        printf("Invalid syntax: setadd <name> <value1_or_VAR> <value2_or_VAR>\n");
    }
} else if (strncmp(GCODEMAIN, "setmath ", 8) == 0) {
    char name[MAX_NAME_LENGTH];
    char input[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 8, "%99s %9999[^\n]", name, input) == 2) {
        int numeric_value = 0;
        char* expression_to_evaluate = NULL;
        
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
            expression_to_evaluate = input;
        }
        
        numeric_value = evaluate_simple_expression(expression_to_evaluate);
        
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
} else if (strncmp(GCODEMAIN, "import ", 7) == 0) {
    char* content = GCODEMAIN + 7;
    content[strcspn(content, "\r\n")] = '\0';
    
    char* dll_name = NULL;
    
    if (strncmp(content, "<VAR>", 5) == 0) {
        Variable* var = find_variable(content + 5);
        if (var != NULL) {
            dll_name = strdup(var->value);
        } else {
            printf("Invalid syntax: %s\n", content + 5);
            return;
        }
    } else {
        dll_name = strdup(content);
    }
    
    if (dll_name != NULL) {
        add_dll_module(dll_name);
        free(dll_name);
    }
}	else if (strncmp(GCODEMAIN, "execute ", 8) == 0) {
    char args[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 8, "%99[^\n]", args) == 1) {
        int unl_mode = 0;
        char* command = args;
        
        if (strncmp(args, "UNL ", 4) == 0) {
            unl_mode = 1;
            command = args + 4;
        }
        
        if (strncmp(command, "<VAR>", 5) == 0) {
            Variable* var = find_variable(command + 5);
            if (var != NULL) {
                char* code_to_execute = var->value;
                char processed_command[MAX_VALUE_LENGTH];
                
                if (unl_mode) {
                    int j = 0;
                    for (int i = 0; code_to_execute[i] != '\0' && j < MAX_VALUE_LENGTH - 1; i++) {
                        if (code_to_execute[i] == '\\' && code_to_execute[i+1] == 'n') {
                            processed_command[j++] = '\n';
                            i++;
                        } else {
                            processed_command[j++] = code_to_execute[i];
                        }
                    }
                    processed_command[j] = '\0';
                    code_to_execute = processed_command;
                } else {
                    strncpy(processed_command, code_to_execute, MAX_VALUE_LENGTH - 1);
                    processed_command[MAX_VALUE_LENGTH - 1] = '\0';
                    code_to_execute = processed_command;
                }
                
                char *line_start = code_to_execute;
                char *line_end;
                
                while (*line_start != '\0') {
                    line_end = line_start;
                    while (*line_end != '\0' && *line_end != '\n') {
                        line_end++;
                    }
                    
                    int line_length = line_end - line_start;
                    if (line_length > 0) {
                        char* line = malloc(line_length + 1);
                        strncpy(line, line_start, line_length);
                        line[line_length] = '\0';
                        
                        if (strlen(line) > 0) {
                            ask_gcode(line);
                        }
                        free(line);
                    }
                    
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
            if (unl_mode) {
                char processed_command[MAX_VALUE_LENGTH];
                int j = 0;
                for (int i = 0; command[i] != '\0' && j < MAX_VALUE_LENGTH - 1; i++) {
                    if (command[i] == '\\' && command[i+1] == 'n') {
                        processed_command[j++] = '\n';
                        i++;
                    } else {
                        processed_command[j++] = command[i];
                    }
                }
                processed_command[j] = '\0';
                
                char *line_start = processed_command;
                char *line_end;
                
                while (*line_start != '\0') {
                    line_end = line_start;
                    while (*line_end != '\0' && *line_end != '\n') {
                        line_end++;
                    }
                    
                    int line_length = line_end - line_start;
                    if (line_length > 0) {
                        char* line = malloc(line_length + 1);
                        strncpy(line, line_start, line_length);
                        line[line_length] = '\0';
                        
                        if (strlen(line) > 0) {
                            ask_gcode(line);
                        }
                        free(line);
                    }
                    
                    if (*line_end == '\n') {
                        line_start = line_end + 1;
                    } else {
                        line_start = line_end;
                    }
                }
            } else {
                ask_gcode(command);
            }
        }
    } else {
        printf("Invalid syntax: execute [UNL] <command_or_VAR>\n");
    }
} else if (strncmp(GCODEMAIN, "getinput", 8) == 0) {
    char* args = GCODEMAIN + 8;
    while (*args == ' ') args++;

    if (*args == '\0') {
        char input[MAX_VALUE_LENGTH];
        fgets(input, sizeof(input), stdin);
    } else {
        char first_arg[MAX_VALUE_LENGTH] = {0};
        char second_arg[MAX_NAME_LENGTH] = {0};
        
        sscanf(args, "%s %s", first_arg, second_arg);
        
        char prompt_text[MAX_VALUE_LENGTH] = {0};
        char save_var_name[MAX_NAME_LENGTH] = {0};
        int should_save = 0;
        
        if (second_arg[0] != '\0') {
            should_save = 1;
            strcpy(save_var_name, second_arg);
            
            if (strncmp(first_arg, "<VAR>", 5) == 0) {
                Variable* var = find_variable(first_arg + 5);
                if (var != NULL) strcpy(prompt_text, var->value);
            } else {
                strcpy(prompt_text, first_arg);
            }
        } else {
            if (strncmp(first_arg, "<VAR>", 5) == 0) {
                Variable* var = find_variable(first_arg + 5);
                if (var != NULL) strcpy(prompt_text, var->value);
            } else {
                strcpy(prompt_text, first_arg);
            }
        }
        
        if (prompt_text[0] != '\0') {
            printf("%s", prompt_text);
            fflush(stdout);
        }
        
        char input[MAX_VALUE_LENGTH];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            
            if (should_save && save_var_name[0] != '\0') {
                set_variable(save_var_name, input);
            }
        }
    }
} else if (strncmp(GCODEMAIN, "if ", 3) == 0) {
    char buffer[MAX_VALUE_LENGTH];
    strcpy(buffer, GCODEMAIN + 3);
    
    char* op_pos = NULL;
    char operator[4] = "";
    
    if ((op_pos = strstr(buffer, " ==")) != NULL) strcpy(operator, "==");
    else if ((op_pos = strstr(buffer, " /=")) != NULL) strcpy(operator, "/=");
    else if ((op_pos = strstr(buffer, " =.")) != NULL) strcpy(operator, "=.");
    else if ((op_pos = strstr(buffer, " /.")) != NULL) strcpy(operator, "/.");
    else if ((op_pos = strstr(buffer, " .=")) != NULL) strcpy(operator, ".=");
    else if ((op_pos = strstr(buffer, " ./")) != NULL) strcpy(operator, "./");
    else if ((op_pos = strstr(buffer, " ..")) != NULL) strcpy(operator, "..");
    else if ((op_pos = strstr(buffer, " //")) != NULL) strcpy(operator, "//");
    
    if (op_pos != NULL) {
        *op_pos = '\0';
        char* left_side = buffer;
        char* rest = op_pos + strlen(operator) + 1;
        
        while (*rest == ' ') rest++;
        
        char* right_end = strchr(rest, ' ');
        if (right_end != NULL) {
            *right_end = '\0';
            char* right_side = rest;
            char* command = right_end + 1;
            
            char left_value[MAX_VALUE_LENGTH];
            if (strncmp(left_side, "<VAR>", 5) == 0) {
                Variable* left_var = find_variable(left_side + 5);
                if (left_var != NULL) strcpy(left_value, left_var->value);
                else strcpy(left_value, "");
            } else {
                strcpy(left_value, left_side);
            }
            
            char right_value[MAX_VALUE_LENGTH];
            if (strncmp(right_side, "<VAR>", 5) == 0) {
                Variable* right_var = find_variable(right_side + 5);
                if (right_var != NULL) strcpy(right_value, right_var->value);
                else strcpy(right_value, "");
            } else {
                strcpy(right_value, right_side);
            }
            
            int condition_met = 0;
            
            if (strcmp(operator, "==") == 0) condition_met = (strcmp(left_value, right_value) == 0);
            else if (strcmp(operator, "/=") == 0) condition_met = (strcmp(left_value, right_value) != 0);
            else if (strcmp(operator, "=.") == 0) condition_met = (strncmp(left_value, right_value, strlen(right_value)) == 0);
            else if (strcmp(operator, "/.") == 0) condition_met = (strncmp(left_value, right_value, strlen(right_value)) != 0);
            else if (strcmp(operator, ".=") == 0) {
                int left_len = strlen(left_value);
                int right_len = strlen(right_value);
                condition_met = (left_len >= right_len) ? (strcmp(left_value + left_len - right_len, right_value) == 0) : 0;
            }
            else if (strcmp(operator, "./") == 0) {
                int left_len = strlen(left_value);
                int right_len = strlen(right_value);
                condition_met = (left_len >= right_len) ? (strcmp(left_value + left_len - right_len, right_value) != 0) : 1;
            }
            else if (strcmp(operator, "..") == 0) condition_met = (strstr(left_value, right_value) != NULL);
            else if (strcmp(operator, "//") == 0) condition_met = (strstr(left_value, right_value) == NULL);
            
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
    char command[MAX_VALUE_LENGTH];
    
    if (sscanf(GCODEMAIN + 4, "%99s in %99s %99[^\n]", var_name, in_text, command) == 3) {
        char iterable_value[MAX_VALUE_LENGTH];
        if (strncmp(in_text, "<VAR>", 5) == 0) {
            Variable* iter_var = find_variable(in_text + 5);
            if (iter_var != NULL) strcpy(iterable_value, iter_var->value);
            else strcpy(iterable_value, "");
        } else {
            strcpy(iterable_value, in_text);
        }
        
        for (int i = 0; iterable_value[i] != '\0'; i++) {
            char current_char[2] = {iterable_value[i], '\0'};
            set_variable(var_name, current_char);
            ask_gcode(command);
        }
    } else {
        printf("Invalid for syntax: for <var> in <value> <command>\n");
    }
} else if (strncmp(GCODEMAIN, "setget ", 7) == 0) {
    char set_var[MAX_NAME_LENGTH];
    char get_var[MAX_NAME_LENGTH];
    char temp_var[MAX_NAME_LENGTH];
    
    if (sscanf(GCODEMAIN + 7, "%99s %99s", set_var, get_var) == 2) {
        if (strncmp(get_var, "<VAR>", 5) == 0) {
            char* indirect_var_name = get_var + 5;
            Variable* indirect_var = find_variable(indirect_var_name);
            
            if (indirect_var != NULL) {
                strncpy(temp_var, indirect_var->value, MAX_NAME_LENGTH - 1);
                temp_var[MAX_NAME_LENGTH - 1] = '\0';
                
                Variable* source_var = find_variable(temp_var);
                if (source_var != NULL) {
                    set_variable(set_var, source_var->value);
                } else {
                    printf("Variable not found: %s (from %s)\n", temp_var, indirect_var_name);
                }
            } else {
                printf("Indirect variable not found: %s\n", indirect_var_name);
            }
        } else {
            Variable* source_var = find_variable(get_var);
            if (source_var != NULL) {
                set_variable(set_var, source_var->value);
            } else {
                printf("Variable not found: %s\n", get_var);
            }
        }
    } else {
        printf("Invalid syntax: setget <varname_set> <varname_get>\n");
    }
} else {
    // Вызов пользовательской функции или DLL функции
    char command_copy[MAX_VALUE_LENGTH];
    strncpy(command_copy, GCODEMAIN, MAX_VALUE_LENGTH - 1);
    command_copy[MAX_VALUE_LENGTH - 1] = '\0';
    command_copy[strcspn(command_copy, "\r\n")] = '\0';
    
    // Копируем имя команды до пробела
    char cmd_name_copy[MAX_NAME_LENGTH];
    char* first_space = strchr(command_copy, ' ');
    
    if (first_space != NULL) {
        int name_len = first_space - command_copy;
        strncpy(cmd_name_copy, command_copy, name_len);
        cmd_name_copy[name_len] = '\0';
    } else {
        strcpy(cmd_name_copy, command_copy);
    }
    
    char* args = NULL;
    if (first_space != NULL) {
        args = first_space + 1;
    } else {
        args = "";
    }
    
    // Ищем точку в ИМЕНИ (а не в command_copy, которую мы модифицируем)
    char* dot_pos = strchr(cmd_name_copy, '.');
    char* var_name = cmd_name_copy;
    char* func_name = NULL;
    
    if (dot_pos != NULL) {
        *dot_pos = '\0';
        func_name = dot_pos + 1;
    }
    
    // Ищем переменную
    Variable* cmd_var = find_variable(var_name);
    if (cmd_var != NULL) {
        // Проверяем, не является ли значение переменной маркером DLL
        if (strncmp(cmd_var->value, "\x00\xFF", 2) == 0) {
            // Это DLL вызов
            char* dll_name = cmd_var->value + 2;
            
            if (func_name != NULL && strlen(func_name) > 0) {
                // Вызов с точкой: test.print args
                call_dll_function_by_name(dll_name, func_name, args);
            } else {
                // Вызов без точки: test args -> вызываем init
                if (dll_has_function(dll_name, "init")) {
                    call_dll_function_by_name(dll_name, "init", args);
                } else {
                    printf("Invalid syntax: %s\n", var_name);
                }
            }
            return;
        }
        
        // Обычное выполнение пользовательской функции
        if (strlen(args) > 0) {
            char* argv_func[10] = {0};
            int argc_func = 0;
            
            char args_copy[MAX_VALUE_LENGTH];
            strncpy(args_copy, args, MAX_VALUE_LENGTH - 1);
            args_copy[MAX_VALUE_LENGTH - 1] = '\0';
            
            char* current = args_copy;
            int in_arg = 0;
            
            while (*current != '\0' && argc_func < 10) {
                if (*current != ' ') {
                    if (!in_arg) {
                        argv_func[argc_func] = current;
                        argc_func++;
                        in_arg = 1;
                    }
                } else {
                    if (in_arg) {
                        *current = '\0';
                        in_arg = 0;
                    }
                }
                current++;
            }
            
            char argc_str[10];
            snprintf(argc_str, sizeof(argc_str), "%d", argc_func);
            set_variable("__argc-func__", argc_str);
            set_variable("__argv-*-func__", args);
            for (int i = 0; i < argc_func; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i + 1);
                set_variable(var_name, argv_func[i]);
            }
            
            for (int i = argc_func; i < 10; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i + 1);
                set_variable(var_name, "");
            }
        } else {
            set_variable("__argc-func__", "0");
            set_variable("__argv-*-func__", "");
            for (int i = 1; i <= 10; i++) {
                char var_name[20];
                snprintf(var_name, sizeof(var_name), "__argv-%d-func__", i);
                set_variable(var_name, "");
            }
        }
        
        // Сохраняем текущий указатель чтения
        char* saved_reader = g_code_reader;
        
        // Устанавливаем новый указатель на код функции
        g_code_reader = cmd_var->value;
        
        // Выполняем функцию построчно
        char* line;
        while ((line = get_next_line()) != NULL) {
            if (strlen(line) > 0) {
                ask_gcode(line);
            }
            free(line);
        }
        
        // Восстанавливаем указатель чтения
        g_code_reader = saved_reader;
    } else {
        printf("Invalid syntax: %s\n", var_name);
        return;
    }
}
}

int main(int argc, char *argv[]) {
    set_variable("__nothing__", "");
    set_variable("__None__", "\0");
    set_variable("__newline__", "\n");
    set_variable("__resetline__", "\r");
    set_variable("__space__", " ");
    
    char exe_path[MAX_PATH];
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
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char size_str[50];
    snprintf(size_str, sizeof(size_str), "%d", file_size);
    set_variable("__size__", size_str);

    unsigned char *buffer = (unsigned char*)malloc(file_size);
    if (buffer == NULL) {
        perror("Memory error");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return 1;
    }
    
    long position = find_PATTERNGCODE_from_end(buffer, file_size);
    char* code = buffer + 12 + position;
    
    char GCODEMAIN[10000];
    if (code == NULL || code[0] == '\0' || strcmp(code, "None") == 0) {
        if (argc > 1 && strncmp(argv[1], "gcode://", 8) == 0) {
            set_variable("__opened-as__","url");
            char *download_url = argv[1] + 8;
            char decoded_url[1024];
            int j = 0;
            for (int i = 0; download_url[i] != '\0' && j < sizeof(decoded_url) - 1; i++) {
                if (download_url[i] == '%') {
                    if (download_url[i+1] == '2' && download_url[i+2] == 'F') {
                        decoded_url[j++] = '/';
                        i += 2;
                    } else if (download_url[i+1] == '3' && download_url[i+2] == 'A') {
                        decoded_url[j++] = ':';
                        i += 2;
                    } else {
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
            char command[1024];
            snprintf(command, sizeof(command), "curl -s \"%s\"", decoded_url);
            
            FILE *pipe = popen(command, "r");
            if (pipe) {
                char pipe_buffer[10000];
                while (fgets(pipe_buffer, sizeof(pipe_buffer), pipe)) {
                    pipe_buffer[strcspn(pipe_buffer, "\r\n")] = '\0';
                    if (strlen(pipe_buffer) > 0) {
                        ask_gcode(pipe_buffer);
                    }
                }
                pclose(pipe);
            }
        } else if (argc > 1) {
            set_variable("__opened-as__","argv");
            initialize_args_variables(argc, argv);
            FILE *input_file = fopen(argv[1], "r");
            if (input_file == NULL) {
                perror("Error opening file");
                free(buffer);
                fclose(file);
                return 1;
            }
            
            char *code_arg = NULL;
            size_t buffer_size = 0;
            size_t code_length = 0;
            int c;

            while ((c = fgetc(input_file)) != EOF) {
                if (code_length >= buffer_size) {
                    buffer_size = buffer_size == 0 ? 128 : buffer_size * 2;
                    code_arg = realloc(code_arg, buffer_size);
                    if (code_arg == NULL) {
                        fclose(input_file);
                        free(buffer);
                        fclose(file);
                        return 1;
                    }
                }
                code_arg[code_length++] = (char)c;
            }

            if (code_arg != NULL) {
                code_arg = realloc(code_arg, code_length + 1);
                code_arg[code_length] = '\0';
            }

            fclose(input_file);

            g_code_reader = code_arg;
            
            char* line;
            while ((line = get_next_line()) != NULL) {
                if (strlen(line) > 0) {
                    ask_gcode(line);
                }
                free(line);
            }

            free(code_arg);
        } else {
            printf("GCode By GPGStudio.\n\n"); 
            set_variable("__opened-as__","console");
            while (1) {
                printf("GCode> ");
                fgets(GCODEMAIN, sizeof(GCODEMAIN), stdin);
                GCODEMAIN[strcspn(GCODEMAIN, "\n")] = '\0';
                if (strlen(GCODEMAIN) > 0) {
                    if (strncmp(GCODEMAIN, "setmore ", 8) == 0) {
                        char name[MAX_NAME_LENGTH];
                        char value[MAX_VALUE_LENGTH * 4] = "";
                        int nesting = 1;
                        
                        sscanf(GCODEMAIN + 8, "%99s", name);
                        
                        while (nesting > 0) {
                            printf("... ");
                            fgets(GCODEMAIN, sizeof(GCODEMAIN), stdin);
                            GCODEMAIN[strcspn(GCODEMAIN, "\n")] = '\0';
                            
                            if (strncmp(GCODEMAIN, "setmore ", 8) == 0) nesting++;
                            else if (strcmp(GCODEMAIN, "<END>") == 0) {
                                nesting--;
                                if (nesting == 0) break;
                            }
                            
                            if (nesting > 0) {
                                if (strlen(value) > 0) strcat(value, "\n");
                                strcat(value, GCODEMAIN);
                            }
                        }
                        
                        char result[MAX_VALUE_LENGTH * 4];
                        result[0] = '\0';
                        const char* cp = value;
                        while (*cp) {
                            if (*cp == '{') {
                                const char* eb = strchr(cp + 1, '}');
                                if (eb != NULL) {
                                    int vnl = eb - (cp + 1);
                                    char vn[MAX_NAME_LENGTH];
                                    if (vnl < sizeof(vn)) {
                                        strncpy(vn, cp + 1, vnl);
                                        vn[vnl] = '\0';
                                        Variable* v = find_variable(vn);
                                        if (v != NULL) strcat(result, v->value);
                                        cp = eb + 1;
                                        continue;
                                    }
                                }
                            }
                            int rs = sizeof(result) - strlen(result) - 1;
                            if (rs > 0) strncat(result, cp, 1);
                            cp++;
                        }
                        set_variable(name, result);
                    } else {
                        ask_gcode(GCODEMAIN);
                    }
                }
            }
        }
    } else {
        set_variable("__opened-as__","compiled");
        initialize_args_variables(argc, argv);
        
        g_code_reader = code;
        
        char* line;
        while ((line = get_next_line()) != NULL) {
            if (strlen(line) > 0) {
                ask_gcode(line);
            }
            free(line);
        }
    }
    cleanup_dll_modules();
    free(buffer);
    fclose(file);
    return 0;
}
