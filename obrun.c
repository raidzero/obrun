/*
	Copyright (C) 2012-2013  DolphinCommode

	This is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	obrun is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <pthread.h>

#define DEBUG 0

// returns true if esc is pressed
static gboolean check_escape(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_KEY_Escape) {
    gtk_main_quit();
    return TRUE;
  }
  return FALSE;
}

static int in_file(char* filename, char* s)
{
	char line[256];
	int present = 1;
	FILE* fp = fopen(filename, "r");

	if (fp != NULL)
	{
		rewind(fp); // start from beginning

		while (fgets(line, sizeof(line), fp) != NULL)
		{
			line[strlen(line)-1] = '\0'; // kill newline
			present = strcmp(line, s);
			#if DEBUG == 1
				printf("Checking %s for %s: %d\n", line, s, present);
			#endif
			if (present == 0) break;
		}
		fclose(fp);
	}
	
	return (present == 0);
}

int main(void) 
{
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

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL); // listen for X button
	g_signal_connect(window, "key_press_event", G_CALLBACK(check_escape), NULL); // listen for esc
	g_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "activate", G_CALLBACK(gtk_main_quit), NULL); // listen for dropdown changes

	gtk_widget_show_all(window);
	gtk_main();

	gchar *orig_str = g_strdup_printf("%s", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
	gchar *exec_str = g_strdup_printf("%s&", gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));

	
	if (strcmp(orig_str, "(null)") == 0 || strcmp(orig_str, "") == 0) // user didnt enter anything
	{
		return 0;
	}
	
	int result = system(exec_str);
	
	int already_present = in_file(histFile, orig_str); // have we already recorded this?
	
	#if DEBUG
		printf("already_present: %d\n", already_present);
	#endif
	if (result == 0 && !already_present)
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

	return 0;
}
