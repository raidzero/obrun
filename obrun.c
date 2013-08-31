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

// globals
GtkWidget* window; // the main window
GtkWidget* combo; // the entry area
GtkWidget* err_dialog; // error dialog
GList* matches; // possible auto complete matches
gchar* old_entry = ""; // what was entered before
int current_match_index = 0; // the match index we are currently on
char* sort_mode; // how we will sort the autocomplete

// quits the program after destroying any given widgets
void die()
{
	gtk_main_quit();	
	exit(0);
}

// displays an error message box
void display_error_dialog(const gchar* error_str)
{
	err_dialog = gtk_message_dialog_new(GTK_WINDOW(window), 
		GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,		
		"%s", error_str);

	gtk_dialog_run(GTK_DIALOG(err_dialog));
	gtk_widget_destroy(err_dialog);
}

// performs actions for key down events (esc, tab)
static gboolean check_key_down(GtkWidget* wisget, GdkEventKey *event, gpointer data)
{
	const gchar* entry = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
	
	int numMatches = g_list_length(matches);
	gchar* match;

	switch (event->keyval)
	{
		case GDK_KEY_Escape:
			#if DEBUG
				printf("ESC pressed!\n");
			#endif
			die();		
		case GDK_KEY_Tab:
			
			#if DEBUG
				printf("Tab DOWN. Found %d matches\n", numMatches);
			#endif
			
			// if there were no matches just delete the entry text
			if (numMatches == 0)
			{
				gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "");
				break;
			}
			match = matches->data;

			// has the entry changed since hitting tab?
			if (g_strcmp0(old_entry, entry) == 0)
			{
				#if DEBUG
					printf("Entry has not changed! ");
					printf("%d < %d? %d\n", current_match_index, numMatches-1, (current_match_index < numMatches-1));
				#endif

				if (current_match_index < numMatches-1)
				{
					match = g_list_nth(matches, ++current_match_index)->data;
				}
				else // at the end, wrap around
				{	
					current_match_index = 0;
					match = g_list_nth(matches, current_match_index)->data;
				}
			}
			
			// whatever has been found, set it in the entry box
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), match);		
			gtk_entry_set_position(GTK_ENTRY(GTK_COMBO(combo)->entry), strlen(match));
			break;
	}
	
	old_entry = g_strdup(entry);
	return FALSE;
}

// performs actions for key up events (any key other than esc, tab)
static gboolean check_key_up(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	const gchar* entry = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
	const gchar* path = (gchar*) getenv("PATH");
	
	switch(event->keyval)
	{
		case GDK_KEY_Escape:
			break; // do nothing
		case GDK_KEY_Tab:
			#if DEBUG
				printf("TAB UP\n");
			#endif
			break; // nothing
		default: // other keys
			#if DEBUG
				printf("Checking matches for \"%s\"...\n", entry);
			#endif
			matches = get_path_matches(entry, g_strdup(path));
			break;
	}

	return FALSE;
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

int main(int argc, char* argv[]) 
{
	// wipe away any previous err file
	unlink("/tmp/err");
	
	char histFile[256];
	sprintf(histFile, "/home/%s/.obrun_history", getenv("USER"));	
	
	// open history file
	FILE* logFp = fopen(histFile, "r");


	gtk_init(NULL, NULL);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL); // main window
	combo = gtk_combo_new(); // editable box with dropdown menu
		
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
			g_list_free(histLines);
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
	
	g_free(newLine);

	// we're done with this file for now
	fclose(logFp);
	
	// how do we want to sort?
	sort_mode = "size";
	if (argc > 1)
	{
		if (strcmp(argv[1], "-a") == 0)
		{
			sort_mode = "alpha";
		}
	}


	gtk_window_set_title(GTK_WINDOW(window), "Run...");

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_add(GTK_CONTAINER(window), combo);

	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_widget_set_size_request(GTK_COMBO(combo)->entry, 200, 20);

	gtk_combo_disable_activate(GTK_COMBO(combo)); // don't show dropdown when enter
	
	// listen for X button
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(die), NULL); 
	
	// listen for key up events: for typing
	g_signal_connect(window, "key_release_event", G_CALLBACK(check_key_up), NULL); 

	// listen for key down events: for autocomplete, esc
	g_signal_connect(window, "key_press_event", G_CALLBACK(check_key_down), NULL);
	// listen for enter
	g_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "activate", G_CALLBACK(gtk_main_quit), NULL); 
	
	G_START: 
	gtk_widget_show_all(window);
	gtk_main();

	gchar *orig_str = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
	gchar *exec_str = g_strdup_printf("%s 2> /tmp/err &", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

	
	if (strcmp(orig_str, "(null)") == 0 || strcmp(orig_str, "") == 0) // user didnt enter anything
	{
		die();
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
		
		
		if (not_found)
		{
			char* err_msg = malloc((strlen(orig_str) + strlen(": Command not found!") + 1) * sizeof(char));
			sprintf(err_msg, "%s: Command not found!", orig_str);

			display_error_dialog(err_msg);
		
			// wipe out the entry field 
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "");
			
			free(err_msg);
			goto G_START; // don't exit, move back up to where widgets are shown
		}
		
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
	}
	
	g_free(orig_str);
	g_free(exec_str);	
	g_list_free(matches);

	die();
	return 0;
}
