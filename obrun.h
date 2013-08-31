void die();
void display_error_dialog(const gchar*);
static gboolean check_key_down(GtkWidget*, GdkEventKey*, gpointer);
static gboolean check_key_up(GtkWidget*, GdkEventKey*, gpointer);
static int in_file(char*, char*, int);

