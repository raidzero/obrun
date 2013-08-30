#include <stdio.h> 
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "path.h"

#define DEBUG 0

GList* get_path_matches(const gchar* cmd, const gchar* path)
{
	#if DEBUG
		printf("Checking PATH (%s) for cmd (%s)...\n", path, cmd);
	#endif
	int numDirs = count_chars(':', path);
	char** path_array = split_str(path, ":", numDirs);
	
	#if DEBUG
		printf("Searching PATH for cmd (%s):\n", cmd);	
	#endif

	// where matches go
	GList* matches = NULL;

	// iterate over the path directories
	int i = 0;
	for (i=0; i<numDirs; i++)
	{
		char* pathDir = path_array[i];

		if (strcmp(pathDir, "UNDEFINED") == 0)
		{
			// dont want to operate on something that isnt there
			#if DEBUG
				printf("DIR UNDEFINED, skipping!\n");
			#endif
			continue;
		}
		if (strstr(pathDir, "~"))
		{
			pathDir = replace_home(pathDir);
		}
		#if DEBUG
			printf("Checking (%s)...\n", pathDir);
		#endif

		// look in each dir
		DIR* dir;
		struct dirent *ent;
		if ((dir = opendir(pathDir)) != NULL) // make sure it exists
		{
			#if DEBUG
				printf("%s opened successfully.\n", pathDir);
			#endif
			while ((ent = readdir(dir)) != NULL) // make sure it is accessible
			{
				#if DEBUG == 2
					printf("Checking file (%s)... ", ent->d_name);	
				#endif
				
				int submatch = str_startswith(ent->d_name, cmd);
				
				#if DEBUG == 2
					printf("submatch: %d\n", submatch);
				#endif
				// build its absolute path for stat'ing
				char fullpath[256];
				sprintf(fullpath, "%s/%s", pathDir, ent->d_name);
				#if DEBUG == 2
					printf("fullpath: %s\n", fullpath);
				#endif
				
				if (submatch)
				{
					// now see if its executable
					struct stat sb;
					if (!stat(fullpath, &sb))
					{
						// is regular file and mode & 0111 is not 0 - meaning something is left, so was 7
						#if DEBUG == 2
							printf("%s: %d\n", fullpath, sb.st_mode);
						#endif
						if (S_ISREG(sb.st_mode) && sb.st_mode & 0111)
						{
							//printf("%s ", ent->d_name);

							// add to glist
							gchar* match = g_strdup(ent->d_name);
							matches = g_list_insert_sorted(matches, match, (GCompareFunc) compar);
						}
					}
				}
				#if DEBUG == 2
					printf("%s check done.\n", fullpath);
				#endif
			}
			closedir(dir);
		}
		else
		{
			#if DEBUG
				printf("Could not open directory: %s\n", pathDir);
			#endif
		}
	}
	
	int numMatches = g_list_length(matches);
	if ( numMatches == 0)
	{
		printf("No matches for %s\n", cmd);
		return NULL;
	}
	else
	{
		// return the list, let the caller figure out what to do with it
		printf("Returning %d matches for %s.\n", numMatches, cmd);
		return matches;
	}
}

// will free a char**
void free_string_array(char** array, int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		free(array[i]);
	}
	// now free the main thing
	free(array);
}

// used for glist sorting - by length
gint compar (gpointer a, gpointer b)
{
	return (strlen((char*)a) > strlen((char*)b));
}
// returns 0 or 1 if string starts with another
int str_startswith(char* s, const char* st)
{
	#if DEBUG
		printf("str_startswith(\"%s\", \"%s\") called\n", s, st);
	#endif
	int stLen = strlen(st);
	int i = 0;

	for (i=0; i<stLen; i++)
	{
		#if DEBUG
			printf("s[%d] (%c) == st[%d] (%c)? %d\n", i, s[i], i, st[i], (s[i] == st[i]));
		#endif
		if (s[i] != st[i])
		{
			return 0;
		}
	}
	return 1;
}

// returns a string with ~ replaced with absolute home path
char* replace_home(char* s)
{
	char* username = getenv("USER");

	char homedir[256];
	sprintf(homedir, "/home/%s", username);

	char* result = malloc((strlen(s) + strlen(homedir) + 1) * sizeof(char));
	char* new = malloc(strlen(s) * sizeof(char));

	int i = 2; // in s
	int j = 0; // in new
	for (i = 2; i<strlen(s)+2; i++)
	{
		/*
		#if DEBUG
			printf("new[%d] = %c s[%d] = %c\n", i, new[i], j, s[j]);
		#endif
		*/
		new[j] = s[i];
		j++;
	}

	#if DEBUG
		printf("new: %s\n", new);
	#endif
	sprintf(result, "%s/%s", homedir, new);
	
	free(new);
	return result;
}

// returns number of characters in string
int count_chars(char c, const char* s)
{
	int sLen = strlen(s);
	int i = 0; int count = 0;
	for (i=0; i<sLen; i++)
	{
		if (s[i] == c)
			count++;
	}
		
	#if DEBUG
		printf("Found %d %c's in %s\n", count, c, s);
	#endif
	if (count == 0) count++;
	return count;
}

// returns 0 or 1 if a string is in an array
int in_array(char** haystack, char* needle, int count)
{
	#if DEBUG
		printf("in_array(haystack, \"%s\", %d) called.\n", needle, count);
	#endif
	
	if (haystack[0] == NULL || haystack == NULL || count == 0)
	{
		return 0;
	}

	int result = 0;
	if (count == 0)
	{
		#if DEBUG
			printf("strcmp(\"%s\", \"%s\")\n", haystack[0], needle);
		#endif
		return (strcmp(haystack[0], needle) == 0);
	}
	else
	{
		int i;
		for(i=0; i<count; i++)
		{
			if (haystack[i] == NULL)
			{
				return 0;
			}
			if (strcmp(haystack[i], needle) == 0)
			{
				result = 1;
				break;
			}	
		}
	}
	return result;
}

// returns an array of char*, splits string by delimiter
char** split_str(const char* s, char* delim, int count)
{
	char** result = malloc(sizeof(char*) * (count + 1));
	char* pch;
	int i = 0;
	int already_present;
	pch = strtok((char*) s, delim);
	while (pch != NULL)
	{
		already_present = in_array(result, pch, i);
		#if DEBUG
			printf("already_present: %d\n", already_present);	
		#endif

		if (!already_present)
		{
			result[i] = pch;
			#if DEBUG
				printf("result[%d] = \"%s\"\n", i, pch);
			#endif
		}
		else
		{
			result[i] = "UNDEFINED";
		}
		pch = strtok(NULL, delim);
		i++;
	}

	return result;
}
