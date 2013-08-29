#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>

#include "path.h"

#define DEBUG 1

// returns true if esc is pressed
static gboolean check_escape(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_KEY_Escape) 
	{
		gtk_main_quit();
		printf("ESC pressed!\n");
		return TRUE;
	}
	return FALSE;
}

// puts the shortest match in the PATH in text entry box if tab is pressed
static void check_tab(GtkComboBox *combo, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_KEY_Tab) 
	{
		printf("TAB pressed!\n");

		// get text from box
		const gchar* entry_box = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
		g_printf("entry: %s\n", entry_box);
		
		// pull the PATH
		const gchar* path = (gchar*) getenv("PATH");
		g_printf("path: %s\n", path);

		// see what the shortest PATH match is
		gchar* match = get_path_match(entry_box, g_strdup(path)); // send a copy of path since it gets mangeld
		
		// print the match
		g_print("%s\n", match);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), match);
		g_free(match);
	}
}
// returns 0 or 1 if a line is in a file
static int in_file(char* filename, char* s, int must_exist)
{
	#if DEBUG
		printf("in_file(\"%s\", \"%s\") called\n", filename, s);
	#endif
	char line[256];
	int present = 1;
	FILE* fp = fopen(filename, "r");
	
	if (must_exist) // dont accept a null file pointer, just keep trying
	{
		#if DEBUG
			printf("File must exist.\n");
		#endif
		while (fp == NULL)
		{
			#if DEBUG
				printf("waiting for file to show up...\n");
			#endif

			fp = fopen(filename, "r");
		}
		#if DEBUG
			printf("File exists! yay!\n");
		#endif
		/* wait a small amount of time because bash's redirection is slow compared to us
		 * 2500 microsecs is instant on my system, since this is for me it'll do
		 */
		 usleep(2500); 
	}
	
	if (fp != NULL)
	{
		rewind(fp); // start from beginning

		while (fgets(line, sizeof(line), fp) != NULL)
		{
			line[strlen(line)-1] = '\0'; // kill newline
			present = strcmp(line, s);
			#if DEBUG == 1
				printf("Checking LINE (%s) for MATCH (%s): %d\n", line, s, present);
			#endif
			if (present == 0) break;
		}
	}
	fclose(fp);
	
	return (present == 0);
}

int main(void) 
{
	// wipe away any previous err file
	unlink("/tmp/err");
	
	char histFile[256];
	sprintf(histFile, "/home/%s/.obrun_history", getenv("USER"));	
	
	// open history file
	FILE* logFp = fopen(histFile, "r");

	gtk_init(NULL, NULL);
	
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL); // main window
	GtkWidget* combo = gtk_combo_new(); // editable box with dropdown menu
		
	GList* histLines = NULL;
	char line[356];
	gchar* newLine = NULL;
		
	int lc = 0;	
	
	// add in the history items, if the file was found
	if (logFp != NULL)
	{
		while (fgets(line, sizeof(line), logFp) != NULL) // read a line
		{
			line[strlen(line)-1] = '\0'; // almighty null-terminator
			newLine = g_strdup(line); // g_list_append apparently cant just be run in a loop
			#if DEBUG
				printf("Adding line (%s) to lines...\n", newLine);
			#endif
			histLines = g_list_prepend(histLines, newLine);
			lc++;
		}
	
		histLines = g_list_prepend(histLines, ""); // empty to start with

		if (histLines != NULL)
		{
			#if DEBUG
				printf("%d histLines found\n", lc);
			#endif
			gtk_combo_set_popdown_strings(GTK_COMBO(combo), histLines);
		}
	}
	else
	{
		// just create it
		#if DEBUG
			printf("Creating new history file...\n");
		#endif
		logFp = fopen(histFile, "w");
	}

	// we're done with this file for now
	fclose(logFp);
	
	gtk_window_set_title(GTK_WINDOW(window), "Run...");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_add(GTK_CONTAINER(window), combo);

	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_widget_set_size_request(GTK_COMBO(combo)->entry, 200, 20);
	
	// listen for X button
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL); 
	
	// listen for esc
	g_signal_connect(window, "key_press_event", G_CALLBACK(check_escape), NULL); 

	// listen for tab
	g_signal_connect(combo, "key_press_event", G_CALLBACK(check_tab), NULL); 
	
	// listen for dropdown changes
	g_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "activate", G_CALLBACK(gtk_main_quit), NULL); 

	gtk_widget_show_all(window);
	gtk_main();

	gchar *orig_str = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
	gchar *exec_str = g_strdup_printf("%s 2> /tmp/err &", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

	
	if (strcmp(orig_str, "(null)") == 0 || strcmp(orig_str, "") == 0) // user didnt enter anything
	{
		return 0;
	}
	
	
	g_printf("Executing %s...\n", exec_str);

	int result = system(exec_str);

	// before anything else, has this comand already been recorded?
	int already_present = in_file(histFile, orig_str, 0); 
	if (!already_present)
	{
		// now lets check our error file for a bash error "command not found"
		char not_found_str[256];
		sprintf(not_found_str, "sh: %s: command not found", orig_str);
		int not_found = in_file("/tmp/err", not_found_str, 1);
		
		#if DEBUG
			printf("already_present: %d\n", already_present);
			printf("not_found: %d\n", not_found);
		#endif

		if (result == 0 && !already_present && !not_found)
		{
			FILE* logFp = fopen(histFile, "a");
			if (logFp == NULL)
			{
				logFp = fopen(histFile, "w");
			}	

			#if DEBUG
				printf("Adding %s to historyFile...\n", orig_str);
			#endif
			fprintf(logFp, "%s\n", orig_str);
			fclose(logFp);		
		}
		
		g_free(orig_str);
		g_free(exec_str);
	}
	return 0;
}
