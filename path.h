GList* get_path_matches(const gchar*, const gchar*);
int count_chars(char, const char*);
char** split_str(const char*, char*, int);
char* replace_home(char*);
int str_startswith(char*, const char*);
int in_array(char**, char*, int);
gint compar_size (gpointer, gpointer);
gint compar_alpha (gpointer, gpointer);
void free_string_array(char**, int);
